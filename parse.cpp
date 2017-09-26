/* Complete recursive descent parser for the calculator language.
    Builds on figure 2.16.  Prints a trace of productions predicted and
    tokens matched.  Does no error recovery: prints "syntax error" and
    dies on invalid input.
    Michael L. Scott, 2008-2017.
*/

#include <iostream>
#include <cstdlib>

#include "scan.h"
#include "debug.h"

using namespace std;

/*
 * ast structs
 */
typedef struct _node {
    token type;
    char name[100];
} node;

typedef struct _bin_op {
    token op;
    node* l_child;
    node* r_child;
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
void match (token expected) {
    if (input_token == expected) {
        PREDICT("matched " << names[input_token]);
        if (input_token == t_id || input_token == t_literal) {
            PREDICT(": " << "\"" << token_image << "\"");
            //AST(token_image);
        }
        PREDICT(endl);
        input_token = scan ();
    }
    else error ();
}

void program ();
void stmt_list ();
void stmt ();
void relation ();
void expr (bin_op*);
void expr_tail(bin_op*);
void term (bin_op*);
void term_tail (bin_op*);
void factor_tail (bin_op*);
void factor (bin_op*);
void relation_op(bin_op*);
void add_op (bin_op*);
void mul_op (bin_op*);

void program () {
    AST("(program" << endl);
    switch (input_token) {
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
            AST("]" << endl << ")");
            match (t_eof);
            break;
        default: error ();
    }
}

void stmt_list () {
    switch (input_token) {
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
        /* Follow(stmt), since stmt_list is epsilon */
        case t_eof:
        case t_fi:
        case t_od:
        case t_lparen:
        case t_add:
        case t_sub:
        case t_mul:
        case t_div:
        case t_literal:
            PREDICT("predict stmt_list --> epsilon" << endl);
            break;          /*  epsilon production */
        default:
            AST("error: " << input_token << endl);
            error ();
    }
}

void stmt () {
    switch (input_token) {
        case t_id:
            PREDICT("predict stmt --> id gets expr" << endl);
            AST(":= ");
            cout << "\"";
            AST(token_image);
            cout << "\"";
            match (t_id);
            AST(" ");
            match (t_gets);
            // the bracket only show while there is more than one child
            relation ();
            break;
        case t_read:
            PREDICT("predict stmt --> read id" << endl);
            match (t_read);
            AST("read ");
            cout << "\"";
            AST(token_image);
            cout << "\"";
            match (t_id);
            break;
        case t_write:
            PREDICT("predict stmt --> write relation" << endl);
            match (t_write);
            AST("write ");
            relation ();
            break;
        case t_if:
            PREDICT("predict stmt --> if R SL fi" << endl);
            match (t_if);
            AST("if\n");
            relation ();
            AST(endl << "[ ");
            stmt_list ();
            AST("]" << endl);
            match (t_fi);
            break;
        case t_do:
            PREDICT("predict stmt --> do SL od" << endl);
            match (t_do);
            AST("do\n");
            AST("[ ");
            stmt_list ();
            AST("]" << endl);
            match (t_od);
            break;
        case t_check:
            PREDICT("predict stmt --> check R" << endl);
            match (t_check);
            AST("check ");
            relation ();
            break;
        default: error ();
    }
}

void relation() {
    bin_op* binary_op = (bin_op*) malloc(sizeof(bin_op));
    binary_op->op = t_none;
    binary_op->l_child = NULL;
    binary_op->l_child = NULL;

    switch (input_token) {
        case t_id:
        case t_literal:
        case t_lparen:
            PREDICT("predict relation --> expr expr_tail" << endl);
            expr(binary_op);
            expr_tail (binary_op);

            if (binary_op->op != t_none) {
                cout << "(" << names[binary_op->op] << " ";
                if (binary_op->l_child != NULL) {
                    if (binary_op->l_child->type == t_id) {
                        cout << "(id \"";
                        cout << binary_op->l_child->name;
                        cout << "\") ";
                    }
                    else if (binary_op->l_child->type == t_literal) {
                        cout << "(num \"";
                        cout << binary_op->l_child->name;
                        cout << "\")";
                    }
                }
                if (binary_op->r_child != NULL) {
                    if (binary_op->r_child->type == t_id) {
                        cout << "(id \"";
                        cout << binary_op->r_child->name;
                        cout << "\")";
                    }
                    else if (binary_op->r_child->type == t_literal) {
                        cout << " (num \"";
                        cout << binary_op->r_child->name;
                        cout << "\")";
                    }
                }
                cout << ")";
            } else {
                if (binary_op->l_child != NULL) {
                    if (binary_op->l_child->type == t_id) {
                        cout << "(id \"";
                        cout << binary_op->l_child->name;
                        cout << "\")";
                    }
                    else if (binary_op->l_child->type == t_literal) {
                        cout << "(num \"";
                        cout << binary_op->l_child->name;
                        cout << "\")";
                    }
                }
            }

            if (!binary_op->l_child)
                free(binary_op->l_child);
            if (!binary_op->r_child)
                free(binary_op->r_child);
            free(binary_op);

            break;
        default: error ();
    }
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
        case t_eof:
        case t_id:
        case t_read:
        case t_write:
        case t_if:
        case t_fi:
        case t_do:
        case t_od:
        case t_check:
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

void term_tail (bin_op* binary_op) {
    switch (input_token) {
        case t_add:
        case t_sub:
            PREDICT("predict term_tail --> add_op term term_tail" << endl);
            add_op (binary_op);
            term (binary_op);
            term_tail (binary_op);
            break;
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
    node* child;

    switch (input_token) {
        case t_id :
            PREDICT("predict factor --> id" << endl);

            child = (node *) malloc(sizeof(node));
            child->type = t_id;
            strcpy(child->name, token_image);

            match (t_id);

            if (binary_op->l_child == NULL)
                binary_op->l_child = child;
            else {
                binary_op->r_child = child;
            }

            break;
        case t_literal:
            PREDICT("predict factor --> literal" << endl);

            child = (node *) malloc(sizeof(node));
            child->type = t_literal;
            strcpy(child->name, token_image);

            match (t_literal);

            if (binary_op->l_child == NULL)
                binary_op->l_child = child;
            else
                binary_op->r_child = child;

            break;
        case t_lparen:
            PREDICT("predict factor --> lparen expr rparen" << endl);
            match (t_lparen);
            relation ();
            match (t_rparen);
            break;
        default: error ();
    }
}

void relation_op(bin_op* binary_op) {
    switch (input_token) {
        case t_eq:
            PREDICT("predict relation_op --> ==" << endl);
            match (t_eq);
            //AST("==");
            binary_op->op = t_eq;
            break;
        case t_noteq:
            PREDICT("predict relation_op --> <>" << endl);
            match (t_noteq);
            //AST("<>");
            binary_op->op = t_noteq;
            break;
        case t_lt:
            PREDICT("predict relation_op --> <" << endl);
            match (t_lt);
            //AST("<");
            binary_op->op = t_lt;
            break;
        case t_gt:
            PREDICT("predict relation_op --> >" << endl);
            match (t_gt);
            //AST(">");
            binary_op->op = t_gt;
            break;
        case t_lte:
            PREDICT("predict relation_op --> <=" << endl);
            match (t_lte);
            //AST("<=");
            binary_op->op = t_lte;
            break;
        case t_gte:
            PREDICT("predict relation_op --> >=" << endl);
            match (t_gte);
            //AST(">=");
            binary_op->op = t_gte;
            break;
        default: error ();
    }

    //cout << endl << "test: " << binary_op->op << endl;
}

void add_op (bin_op* binary_op) {
    switch (input_token) {
        case t_add:
            PREDICT("predict add_op --> add" << endl);
            match (t_add);
            //AST("+ ");
            binary_op->op = t_add;
            break;
        case t_sub:
            PREDICT("predict add_op --> sub" << endl);
            match (t_sub);
            //AST("- ");
            binary_op->op = t_sub;
            break;
        default: error ();
    }
}

void mul_op (bin_op* binary_op) {
    switch (input_token) {
        case t_mul:
            PREDICT("predict mul_op --> mul" << endl);
            match (t_mul);
            //AST("* ");
            binary_op->op = t_mul;
            break;
        case t_div:
            PREDICT("predict mul_op --> div" << endl);
            match (t_div);
            //AST("/ ");
            binary_op->op = t_div;
            break;
        default: error ();
    }
}

int main () {
    input_token = scan ();
    program ();
    return 0;
}
