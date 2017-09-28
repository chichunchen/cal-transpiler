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
#include <vector>
#include <algorithm>

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

static const int first_S_[] = {t_id, t_read, t_write, t_if, t_do, t_check};
static const int follow_S_[] = {t_id, t_read, t_write, t_if, t_do, t_check, t_eof};
vector<int> first_S(first_S_, first_S_ + sizeof(first_S_) / sizeof(int));
vector<int> follow_S(follow_S_, follow_S_ + sizeof(follow_S_) / sizeof(int));

static const int first_R_[] = {t_lparen, t_id, t_literal};
static const int follow_R_[] = {t_rparen, t_id, t_read, t_write, t_if, t_do, t_check, t_fi, t_od};
static const int ro[] = {t_eq, t_noteq, t_lt, t_gt, t_lte, t_gte};
vector<int> first_R(first_R_, first_R_ + sizeof(first_R_) / sizeof(int));
vector<int> follow_R(follow_R_, follow_R_ + sizeof(follow_R_) / sizeof(int));

static const int first_E_[] = {t_lparen, t_id, t_literal};
vector<int> first_E(first_E_, first_E_ + sizeof(first_E_) / sizeof(int));

static const int follow_E_[] = {t_rparen, t_id, t_read, t_write, t_if, t_do, t_check, t_fi, t_od, t_eq, t_noteq, t_lt, t_gt, t_lte, t_gte};
vector<int> follow_E(follow_E_, follow_E_ + sizeof(follow_E_) / sizeof(int));

//vector<int> first_F = first_E;
//static const int follow_F_[] = {t_rparen, t_id, t_read, t_write, t_if, t_do, t_check, t_fi, t_od, t_eq, t_noteq, t_lt, t_gt, t_lte, t_gte, t_mul, t_div, t_add, t_sub};
//vector<int> follow_F(follow_F_, follow_F_ + sizeof(follow_F_) / sizeof(int));

enum Context {
    c_stmt_list, c_stmt, c_rel, c_expr, c_expr_tail, c_term, c_term_tail, c_factor, c_factor_tail,
    c_ro, c_ao, c_mo, c_none
};

/*
 * customized exception classes
 */
struct StatementException : public exception {
    const char * what () const throw () {
        return "Statement Exception";
    }
};

struct RelationException : public exception {
    const char * what () const throw () {
        return "Relation Exception";
    }
};

struct ExpressionException : public exception {
    const char * what () const throw () {
        return "Expression Exception";
    }
};

/*
 * the order must be same as token
 */
const char* names[] = {"read", "write", "id", "literal", "gets",
                       "add", "sub", "mul", "div", "lparen", "rparen", "eof",
                       "if", "fi", "do", "od", "check",
                       "eq", "noteq", "lt", "gt", "lte", "gte" };

static token input_token;

void error () {
    cerr << "syntax error at line: " << lineno << endl;
    exit (1);
}

// 1. match token
// 2. if it's id or literal, print it
void match (token expected, bool print, Context context) {
    if (input_token == expected) {
        PREDICT("matched " << names[input_token]);
        if (input_token == t_id || input_token == t_literal) {
            PREDICT(": " << "\"" << token_image << "\"");
            if (print) AST(token_image);
        }
        PREDICT(endl);
        input_token = scan ();
    }
    else {
//        error ();
        if (context == c_stmt) {
            throw StatementException();
        }
        else if (context == c_expr) {
            throw ExpressionException();
        }
        else {  // program, stmt_list
            return;
        }
    }
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
            match (t_eof, false, c_none);
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

    try {
        switch (input_token) {
            case t_id:
                PREDICT("predict stmt --> id gets expr" << endl);
                AST(":= ");
                AST("\"");
                match (t_id, true, c_stmt);
                AST("\"");
                match (t_gets, false, c_stmt);
                // the bracket only show while there is more than one child
                print_relation(relation ());
                break;
            case t_read:
                PREDICT("predict stmt --> read id" << endl);
                match (t_read, false, c_stmt);
                AST("read ");
                AST("\"");
                match (t_id, true, c_stmt);
                AST("\"");
                break;
            case t_write:
                PREDICT("predict stmt --> write relation" << endl);
                match (t_write, false, c_stmt);
                AST("write");
                print_relation(relation ());
                break;
            case t_if:
                PREDICT("predict stmt --> if R SL fi" << endl);
                match (t_if, false, c_stmt);
                AST("if\n");
                print_relation(relation ());
                AST(endl << "[ ");
                stmt_list ();
                AST("]" << endl);
                match (t_fi, false, c_stmt);
                break;
            case t_do:
                PREDICT("predict stmt --> do SL od" << endl);
                match (t_do, false, c_stmt);
                AST("do\n");
                AST("[ ");
                stmt_list ();
                AST("]" << endl);
                match (t_od, false, c_stmt);
                break;
            case t_check:
                PREDICT("predict stmt --> check R" << endl);
                match (t_check, false, c_stmt);
                AST("check");
                print_relation(relation ());
                break;
            default:
//                error ();
                throw StatementException();
        }
    } catch (StatementException se) {
        cerr << se.what() << " " << token_image << " , line number: " << lineno << endl;

        while ((input_token = scan())) {
            // recover
            if (find(first_S.begin(), first_S.end(), input_token) != first_S.end()) {
                cerr << "first: in lineno: " << lineno << ", token: " << token_image << endl;
                stmt();
                input_token = scan();
                return;
            } else if (find(follow_S.begin(), follow_S.end(), input_token) != follow_S.end()) {
                cerr << "follow:  in lineno: " << lineno << ", token: " << token_image << endl;
                input_token = scan();
                return;
            } else {
                cerr << "discard token: " << token_image << ", error in lineno: " << lineno << endl;
                input_token = scan();
            }
        }
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

// init with null binary_op and return filled binary_op
bin_op* relation() {
    bin_op* binary_op = (bin_op*) malloc(sizeof(bin_op));
    binary_op->type = t_none;
    binary_op->l_child = NULL;
    binary_op->r_child = NULL;

    try {
        switch (input_token) {
            case t_id:
            case t_literal:
            case t_lparen:
                PREDICT("predict relation --> expr expr_tail" << endl);
                expr (binary_op);
                expr_tail (binary_op);
                break;
            default:
//            error ();
                throw RelationException();
        }
    } catch (RelationException &re) {
        cerr << re.what() << " , line number: " << lineno << endl;

        while ((input_token = scan())) {
            // recover
            if (find(first_R.begin(), first_R.end(), input_token) != first_R.end()) {
                cerr << "first: in lineno: " << lineno << ", token: " << token_image << endl;
                expr(binary_op);
//                input_token = scan();
                return binary_op;
            } else if (find(follow_R.begin(), follow_R.end(), input_token) != follow_R.end()) {
                cerr << "follow:  in lineno: " << lineno << ", token: " << token_image << endl;
//                input_token = scan();
                return binary_op;
            } else {
                cerr << "discard token: " << token_image << ", error in lineno: " << lineno << endl;
                input_token = scan();
            }
        }
    }
    return binary_op;
}

void expr (bin_op* binary_op) {
    try {
        switch (input_token) {
            case t_id:
            case t_literal:
            case t_lparen:
                PREDICT("predict expr --> term term_tail" << endl);
                term (binary_op);
                term_tail (binary_op);
                break;
            default:
                //error ();
                throw ExpressionException();
        }
    } catch (ExpressionException& ee) {
        cerr << ee.what() << ": error in line number: " << lineno << endl;

        while ((input_token = scan())) {
            // recover
            if (find(first_E.begin(), first_E.end(), input_token) != first_E.end()) {
                cerr << "first: in lineno: " << lineno << ", token: " << names[input_token] << endl;
                expr(binary_op);
//                input_token = scan();
                return;
            } else if (find(follow_E.begin(), follow_E.end(), input_token) != follow_E.end()) {
                cerr << "follow:  in lineno: " << lineno << ", token: " << names[input_token] << endl;
//                input_token = scan();
                return;
            } else {
                cerr << "discard token: " << names[input_token] << ", error in lineno: " << lineno << endl;
                input_token = scan();
            }
        }
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
        case t_rparen:
            PREDICT("predict expr_tail --> epsilon" << endl);
            break;
        default:
//            cerr << "error: " << input_token << endl;
//            error();
            throw ExpressionException();
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
        default:
//            error ();
            throw ExpressionException();
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
        default:
//            error ();
            throw ExpressionException();
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
//            cerr << "error: " << input_token << endl;
//            error ();
            throw ExpressionException();
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

            match (t_id, false, c_factor);

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

            match (t_literal, false, c_factor);

            if (binary_op->l_child == NULL)
                binary_op->l_child = child;
            else
                binary_op->r_child = child;

            break;
        case t_lparen:
            PREDICT("predict factor --> lparen expr rparen" << endl);
            match (t_lparen, false, c_factor);

            child = relation ();
            if (binary_op->l_child == NULL)
                binary_op->l_child = child;
            else
                binary_op->r_child = child;

            match (t_rparen, false, c_factor);
            break;
        default:
//            error ();
            throw ExpressionException();
    }
}

void relation_op(bin_op* binary_op) {
    switch (input_token) {
        case t_eq:
            PREDICT("predict relation_op --> ==" << endl);
            match (t_eq, false, c_ro);
            binary_op->type = t_eq;
            strcpy(binary_op->name, "==");
            break;
        case t_noteq:
            PREDICT("predict relation_op --> <>" << endl);
            match (t_noteq, false, c_ro);
            binary_op->type = t_noteq;
            strcpy(binary_op->name, ",.");
            break;
        case t_lt:
            PREDICT("predict relation_op --> <" << endl);
            match (t_lt, false, c_ro);
            binary_op->type = t_lt;
            strcpy(binary_op->name, "<");
            break;
        case t_gt:
            PREDICT("predict relation_op --> >" << endl);
            match (t_gt, false, c_ro);
            binary_op->type = t_gt;
            strcpy(binary_op->name, ">");
            break;
        case t_lte:
            PREDICT("predict relation_op --> <=" << endl);
            match (t_lte, false, c_ro);
            binary_op->type = t_lte;
            strcpy(binary_op->name, "<=");
            break;
        case t_gte:
            PREDICT("predict relation_op --> >=" << endl);
            match (t_gte, false, c_ro);
            binary_op->type = t_gte;
            strcpy(binary_op->name, ">=");
            break;
        default:
//            error ();
            throw ExpressionException();
    }

    //cout << endl << "test: " << binary_op->op << endl;
}

void add_op (bin_op* binary_op) {
    switch (input_token) {
        case t_add:
            PREDICT("predict add_op --> add" << endl);
            match (t_add, false, c_ao);
            binary_op->type = t_add;
            strcpy(binary_op->name, "+");
            break;
        case t_sub:
            PREDICT("predict add_op --> sub" << endl);
            match (t_sub, false, c_ao);
            binary_op->type = t_sub;
            strcpy(binary_op->name, "-");
            break;
        default:
//            error ();
            throw ExpressionException();
    }
}

void mul_op (bin_op* binary_op) {
    switch (input_token) {
        case t_mul:
            PREDICT("predict mul_op --> mul" << endl);
            match (t_mul, false, c_mo);
            binary_op->type = t_mul;
            strcpy(binary_op->name, "*");
            break;
        case t_div:
            PREDICT("predict mul_op --> div" << endl);
            match (t_div, false, c_mo);
            binary_op->type = t_div;
            strcpy(binary_op->name, "/");
            break;
        default:
//            error ();
            throw ExpressionException();
    }
}

int main () {
    input_token = scan ();
    program ();
    return 0;
}
