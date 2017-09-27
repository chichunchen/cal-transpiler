/* Complete recursive descent parser for the calculator language.
    Builds on figure 2.16.  Prints a trace of productions predicted and
    tokens matched.  Does no error recovery: prints "syntax error" and
    dies on invalid input.
    Michael L. Scott, 2008-2017.
*/

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <stdio.h>

#include "scan.h"
#include "debug.h"

using namespace std;

/*
 * ast structs
 */
typedef struct _bin_op {
    token type;
    char name[100];
    struct _bin_op* l_child;
    struct _bin_op* r_child;
} bin_op;

/*
 * the order must be same as token
 */
const char* names[] = {"read", "write", "id", "literal", "gets",
                       "add", "sub", "mul", "div", "lparen", "rparen", "eof",
                       "if", "fi", "do", "od", "check",
                       "eq", "noteq", "lt", "gt", "lte", "gte" };

static token input_token;

void error () {
    cout << "syntax error" << endl;
    exit (1);
}

// 1. match token
// 2. if it's id or literal, print it
void match (token expected, bool print) {
    if (input_token == expected) {
        PREDICT("matched " << names[input_token]);
        if (input_token == t_id || input_token == t_literal) {
            PREDICT(": " << "\"" << token_image << "\"");
            if (print) AST(token_image);
        }
        PREDICT(endl);
        input_token = scan ();
    }
    else error ();
}

void program ();
void stmt_list ();
void stmt ();
bin_op* relation ();
void expr (bin_op*);
void expr_tail(bin_op*);
void term (bin_op*);
void term_tail (bin_op*);
void factor_tail (bin_op*);
void factor (bin_op*);
void relation_op(bin_op*);
void add_op (bin_op*);
void mul_op (bin_op*);

void print_relation(bin_op* root);
void free_bin_op(bin_op* root);

void program () {
    AST("(program" << endl);
    switch (input_token) {
        /* First(program) */
        case t_id:
        case t_read:
        case t_write:
        case t_if:
        case t_do:
        case t_check:
        case t_eof:
            PREDICT("predict program --> stmt_list eof" << endl);
            AST("[ ");
            stmt_list ();
            AST("]");

            // if not t_eof, delete until eof
            match (t_eof, false);

            break;
        default: error ();
    }
    AST(endl << ")");
}

void stmt_list () {
    switch (input_token) {
        /* First(stmt_list) */
        case t_id:
        case t_read:
        case t_write:
        case t_if:
        case t_do:
        case t_check:
            PREDICT("predict stmt_list --> stmt stmt_list");
            AST("(");
            stmt ();
            AST(")" << endl);
            stmt_list ();
            break;
        /* Follow(stmt_list) has (Follow(stmt) and Follow(R)) */
        case t_eof:
        case t_fi:
        case t_od:
            PREDICT("predict stmt_list --> epsilon" << endl);
            break;          /*  epsilon production */
        default:
            AST("error: " << input_token << endl);
            error ();
    }
}

void stmt () {
    bin_op* root;

    switch (input_token) {
        case t_id:
            PREDICT("predict stmt --> id gets expr" << endl);
            AST(":= ");
            AST("\"");
            match (t_id, true);
            AST("\"");

            // if not :=, then insert an :=
            match (t_gets, false);

            root = relation();
            print_relation(root);
            free_bin_op(root);

            break;
        case t_read:
            PREDICT("predict stmt --> read id" << endl);
            match (t_read, false);
            AST("read ");
            AST("\"");

            // if not id, delete token until id
            match (t_id, true);

            AST("\"");
            break;
        case t_write:
            PREDICT("predict stmt --> write relation" << endl);
            match (t_write, false);
            AST("write");

            // pass write
            root = relation();
            print_relation(root);
            free_bin_op(root);

            break;
        case t_if:
            PREDICT("predict stmt --> if R SL fi" << endl);
            match (t_if, false);
            AST("if\n");

            // pass follow(stmt_list)
            root = relation();
            print_relation(root);
            free_bin_op(root);

            AST(endl << "[ ");
            stmt_list ();
            AST("]" << endl);

            // insert or delete token til token == t_fi
            match (t_fi, false);

            break;
        case t_do:
            PREDICT("predict stmt --> do SL od" << endl);
            match (t_do, false);
            AST("do\n");

            AST("[ ");
            stmt_list ();
            AST("]" << endl);

            // insert or delete token til token == t_od
            match (t_od, false);

            break;
        case t_check:
            PREDICT("predict stmt --> check R" << endl);
            match (t_check, false);
            AST("check");

            // pass follow(r)
            root = relation();
            print_relation(root);
            free_bin_op(root);

            break;
        default: error ();
    }
}

// prefix tree traversal
void print_relation(bin_op* root) {
    if (root->l_child != NULL && root->r_child != NULL)
        AST(" (");

    if (root) {
        if (root->type == t_id) {
            AST("(id \"");
            AST(root->name);
            AST("\")");
        }
        else if (root->type == t_literal) {
            AST("(num \"");
            AST(root->name);
            AST("\")");
        }
        else {
            // print op
            AST(root->name);
        }
    }

    if (root->l_child) {
        AST(" ");
        print_relation(root->l_child);
    }

    if (root->r_child) {
        AST(" ");
        print_relation(root->r_child);
    }

    if (root->l_child != NULL && root->r_child != NULL)
        AST(")");
}

void free_bin_op(bin_op* root) {
    if (root->l_child)
        free_bin_op(root->l_child);
    if (root->r_child)
        free_bin_op(root->r_child);
    free(root);
}

// init with null binary_op and return filled binary_op
// get a context argument
bin_op* relation() {
    bin_op* binary_op = (bin_op*) malloc(sizeof(bin_op));
    binary_op->type = t_none;
    binary_op->l_child = NULL;
    binary_op->r_child = NULL;

    switch (input_token) {
        case t_id:
        case t_literal:
        case t_lparen:
            PREDICT("predict relation --> expr expr_tail" << endl);

            // catch
            // pass follow (expr) in context-specific
            expr(binary_op);

            // pass follow(R) in context-specific
            expr_tail (binary_op);

            break;
        default: error ();
    }
    return binary_op;
}

void expr (bin_op* binary_op) {
    switch (input_token) {
        case t_id:
        case t_literal:
        case t_lparen:
            PREDICT("predict expr --> term term_tail" << endl);

            term (binary_op);
            term_tail (binary_op);
            break;
        default: error ();
    }
}

void expr_tail(bin_op* binary_op) {
    switch (input_token) {
        case t_eq:
        case t_noteq:
        case t_lt:
        case t_gt:
        case t_lte:
        case t_gte:
            relation_op(binary_op);
            expr(binary_op);
            break;
        /* Follow(E) */
        // receive context-specific from caller, and only predict epsilon in the given set
        case t_eof:
        case t_id:
        case t_read:
        case t_write:
        case t_if:
        case t_fi:
        case t_do:
        case t_od:
        case t_check:
        case t_rparen:
            PREDICT("predict expr_tail --> epsilon" << endl);
            break;
        default:
            cerr << "error: " << input_token << endl;
            error();
    }
}

void term (bin_op* binary_op) {
    switch (input_token) {
        case t_id:
        case t_literal:
        case t_lparen:
            PREDICT("predict term --> factor factor_tail" << endl);
            factor (binary_op);
            factor_tail (binary_op);
            break;
        default: error ();
    }
}

// get context-specific sets from caller
void term_tail (bin_op* binary_op) {
    switch (input_token) {
        case t_add:
        case t_sub:
            PREDICT("predict term_tail --> add_op term term_tail" << endl);
            add_op (binary_op);
            term (binary_op);
            term_tail (binary_op);
            break;
        // Follow(E), receive context-specific from caller, and only predict epsilon in the given set
        case t_rparen:
        case t_id:
        case t_read:
        case t_write:
        case t_eof:
        case t_eq:
        case t_noteq:
        case t_gt:
        case t_lt:
        case t_gte:
        case t_lte:
        case t_if:
        case t_fi:
        case t_do:
        case t_od:
        case t_check:
            PREDICT("predict term_tail --> epsilon" << endl);
            break;          /*  epsilon production */
        default: error ();
    }
}

// get context-specific sets from caller
void factor_tail (bin_op* binary_op) {
    switch (input_token) {
        case t_mul:
        case t_div:
            PREDICT("predict factor_tail --> mul_op factor factor_tail" << endl);
            mul_op (binary_op);
            factor (binary_op);
            factor_tail (binary_op);
            break;
        /* Follow(factor_tail) */
        // receive context-specific from caller, and only predict epsilon in the given set
        case t_add:
        case t_sub:
        case t_rparen:
        case t_id:
        case t_read:
        case t_write:
        case t_eof:
        case t_eq:
        case t_noteq:
        case t_gt:
        case t_lt:
        case t_gte:
        case t_lte:
        case t_if:
        case t_fi:
        case t_do:
        case t_od:
        case t_check:
            PREDICT("predict factor_tail --> epsilon" << endl);
            break;          /*  epsilon production */
        default:
            cerr << "error: " << input_token << endl;
            error ();
    }
}

void factor (bin_op* binary_op) {
    bin_op* child;

    switch (input_token) {
        case t_id :
            PREDICT("predict factor --> id" << endl);

            child = (bin_op *) malloc(sizeof(bin_op));
            child->type = t_id;
            strcpy(child->name, token_image);
            child->l_child = NULL;
            child->r_child = NULL;

            match (t_id, false);

            if (binary_op->l_child == NULL)
                binary_op->l_child = child;
            else {
                binary_op->r_child = child;
            }

            break;
        case t_literal:
            PREDICT("predict factor --> literal" << endl);

            child = (bin_op *) malloc(sizeof(bin_op));
            child->type = t_literal;
            strcpy(child->name, token_image);
            child->l_child = NULL;
            child->r_child = NULL;

            match (t_literal, false);

            if (binary_op->l_child == NULL)
                binary_op->l_child = child;
            else
                binary_op->r_child = child;

            break;
        case t_lparen:
            PREDICT("predict factor --> lparen expr rparen" << endl);
            match (t_lparen, false);

            child = relation ();
            if (binary_op->l_child == NULL)
                binary_op->l_child = child;
            else
                binary_op->r_child = child;

            // pass ), Follow(R) as context specific
            relation ();

            match (t_rparen, false);

            break;
        default: error ();
    }
}

void relation_op(bin_op* binary_op) {
    switch (input_token) {
        case t_eq:
            PREDICT("predict relation_op --> ==" << endl);
            match (t_eq, false);
            binary_op->type = t_eq;
            strcpy(binary_op->name, "==");
            break;
        case t_noteq:
            PREDICT("predict relation_op --> <>" << endl);
            match (t_noteq, false);
            binary_op->type = t_noteq;
            strcpy(binary_op->name, ",.");
            break;
        case t_lt:
            PREDICT("predict relation_op --> <" << endl);
            match (t_lt, false);
            binary_op->type = t_lt;
            strcpy(binary_op->name, "<");
            break;
        case t_gt:
            PREDICT("predict relation_op --> >" << endl);
            match (t_gt, false);
            binary_op->type = t_gt;
            strcpy(binary_op->name, ">");
            break;
        case t_lte:
            PREDICT("predict relation_op --> <=" << endl);
            match (t_lte, false);
            binary_op->type = t_lte;
            strcpy(binary_op->name, "<=");
            break;
        case t_gte:
            PREDICT("predict relation_op --> >=" << endl);
            match (t_gte, false);
            binary_op->type = t_gte;
            strcpy(binary_op->name, ">=");
            break;
        default: error ();
    }

    //cout << endl << "test: " << binary_op->op << endl;
}

void add_op (bin_op* binary_op) {
    switch (input_token) {
        case t_add:
            PREDICT("predict add_op --> add" << endl);
            match (t_add, false);
            binary_op->type = t_add;
            strcpy(binary_op->name, "+");
            break;
        case t_sub:
            PREDICT("predict add_op --> sub" << endl);
            match (t_sub, false);
            binary_op->type = t_sub;
            strcpy(binary_op->name, "-");
            break;
        default: error ();
    }
}

void mul_op (bin_op* binary_op) {
    switch (input_token) {
        case t_mul:
            PREDICT("predict mul_op --> mul" << endl);
            match (t_mul, false);
            binary_op->type = t_mul;
            strcpy(binary_op->name, "*");
            break;
        case t_div:
            PREDICT("predict mul_op --> div" << endl);
            match (t_div, false);
            binary_op->type = t_div;
            strcpy(binary_op->name, "/");
            break;
        default: error ();
    }
}

int main () {
    input_token = scan ();
    program ();
    return 0;
}
