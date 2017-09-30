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
#include "ast.h"
#include "debug.h"

using namespace std;

/*
 * ast structs
 */


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
                       "eq", "noteq", "lt", "gt", "lte", "gte" , "none"};

const char* print_names[] = {"read", "write", "id", "literal", "gets",
                             "+", "-", "*", "/", "lparen", "rparen", "eof",
                             "if", "fi", "do", "od", "check",
                             "==", "<>", "<", ">", "<=", ">=", "none"};

static token input_token;

void error () {
    cerr << "syntax error at line: " << lineno << endl;
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
    else {
        cerr << endl;
        cerr << "match error in line: " << lineno << " , get " << names[input_token] <<
                ", insert: " << names[expected] << endl;
        return;
    }
}

void program ();
st_list* stmt_list (st_list* stList);
st* stmt ();
bin_op* relation ();
void expr (bin_op*);
void expr_tail(bin_op*);
void term (bin_op*, token c);
void term_tail (bin_op*, token c);
void factor_tail (bin_op*, token c);
void factor (bin_op*, token c);
void relation_op(bin_op*);
void add_op (bin_op*);
void mul_op (bin_op*);

st_list* pg_sl_root;

void program () {
    pg_sl_root = (st_list*) malloc(sizeof(st_list));
    pg_sl_root->l_child = NULL;
    pg_sl_root->r_child = NULL;

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
            stmt_list (pg_sl_root);
            match (t_eof, false);
            break;
        default: error ();
    }

    print_program_ast(pg_sl_root);
}

// stList is decided on the caller
st_list* stmt_list (st_list* stList) {
    st_list* new_sl;

    switch (input_token) {
        /* First(stmt_list) */
        case t_id:
        case t_read:
        case t_write:
        case t_if:
        case t_do:
        case t_check:
            PREDICT("predict stmt_list --> stmt stmt_list");

            stList->l_child = stmt ();

            new_sl = (st_list*) malloc(sizeof(st_list));
            new_sl->l_child = NULL;
            new_sl->r_child = NULL;

            stList->r_child = new_sl;
            stList = stList->r_child;

            stmt_list (stList);
            break;
        /* Follow(stmt_list) has (Follow(stmt) and Follow(R)) */
        case t_eof:
        case t_fi:
        case t_od:
            PREDICT("predict stmt_list --> epsilon" << endl);
            break;          /*  epsilon production */
        default:
            error ();
    }
    return stList;
}

st* stmt () {
    bin_op* root;
    bin_op* rel;
    st_list* stList;
    st* statement = (st*) malloc(sizeof(st));
    st_list* sl_root;       // do and if

    try {
        switch (input_token) {
            case t_id:
                PREDICT("predict stmt --> id gets expr" << endl);
                strcpy(statement->id, token_image);
                match (t_id, true);
                match (t_gets, false);
                // the bracket only show while there is more than one child
                rel = relation();

                statement->type = t_id;
                statement->rel = rel;
                statement->sl = NULL;
                break;
            case t_read:
                PREDICT("predict stmt --> read id" << endl);
                match (t_read, false);
                strcpy(statement->id, token_image);
                match (t_id, true);

                statement->type = t_read;
                statement->sl = NULL;
                statement->rel = NULL;
                break;
            case t_write:
                PREDICT("predict stmt --> write relation" << endl);
                match (t_write, false);
                rel = relation();

                statement->type = t_write;
                statement->rel = rel;
                statement->sl = NULL;
                statement->id[0] = '\0';
                break;
            case t_if:
                PREDICT("predict stmt --> if R SL fi" << endl);
                match (t_if, false);

                rel = relation();

                sl_root = (st_list*) malloc(sizeof(st_list));
                stmt_list (sl_root);

                statement->type = t_if;
                statement->rel = rel;
                statement->sl = sl_root;
                statement->id[0] = '\0';

                match (t_fi, false);
                break;
            case t_do:
                PREDICT("predict stmt --> do SL od" << endl);
                match (t_do, false);

                sl_root = (st_list*) malloc(sizeof(st_list));
                stmt_list (sl_root);

                statement->type = t_do;
                statement->sl = sl_root;
                statement->rel = NULL;
                statement->id[0] = '\0';

                match (t_od, false);
                break;
            case t_check:
                PREDICT("predict stmt --> check R" << endl);
                match (t_check, false);

                rel = relation();

                statement->type = t_check;
                statement->rel = rel;
                statement->sl = NULL;
                statement->id[0] = '\0';

                break;
            default:
                cerr << "Deleting token: " << token_image << endl;
                throw StatementException();
        }
    } catch (StatementException se) {
        cerr << se.what() << " " << token_image << " , line number: " << lineno << endl;

        while ((input_token = scan())) {
            // recover
            if (find(first_S.begin(), first_S.end(), input_token) != first_S.end()) {
                cerr << "lineno: " << lineno << ", token: " << token_image << " in first set" << endl;
                stmt();
                input_token = scan();
                return statement;
            } else if (find(follow_S.begin(), follow_S.end(), input_token) != follow_S.end()) {
                cerr << "lineno: " << lineno << ", token: " << token_image << " in follow set" << endl;
                input_token = scan();
                return statement;
            } else {
                cerr << "deleting token: " << token_image << ", error in lineno: " << lineno << endl;
                input_token = scan();

                if (input_token == t_eof)
                    return statement;
            }
        }
    }
    return statement;
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
                //cerr << "Deleting token: " << token_image << endl;
                throw RelationException();
        }
    } catch (RelationException &re) {
        cerr << re.what() << " , line number: " << lineno << endl;

        while ((input_token = scan())) {
            // recover
            if (find(first_R.begin(), first_R.end(), input_token) != first_R.end()) {
                cerr << "lineno: " << lineno << ", token: " << token_image << " in first set" << endl;
                expr(binary_op);
                return binary_op;
            } else if (find(follow_R.begin(), follow_R.end(), input_token) != follow_R.end()) {
                cerr << "lineno: " << lineno << ", token: " << token_image << " in follow set" << endl;
                return binary_op;
            } else {
                cerr << "deleting token: " << token_image << ", error in lineno: " << lineno << endl;
                input_token = scan();

                if (input_token == t_eof)
                    return binary_op;
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
                term (binary_op, input_token);
                term_tail (binary_op, input_token);
                break;
            default:
                //cerr << "Deleting token: " << token_image << endl;
                throw ExpressionException();
        }
    } catch (ExpressionException& ee) {
        cerr << endl << ee.what() << ": error in line number: " << lineno << ", delete token: " << token_image << endl;

        while ((input_token = scan())) {
            // recover
            if (find(first_E.begin(), first_E.end(), input_token) != first_E.end()) {
                cerr << "lineno: " << lineno << ", token: " << token_image << " in first set" << endl;
                expr(binary_op);
                return;
            } else if (find(follow_E.begin(), follow_E.end(), input_token) != follow_E.end()) {
                cerr << "lineno: " << lineno << ", token: " << token_image << " in follow set" << endl;
                return;
            } else {
                cerr << "deleting token: " << token_image << ", error in lineno: " << lineno << endl;
                input_token = scan();

                if (input_token == t_eof)
                    return;
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
            //cerr << "Deleting token: " << token_image << endl;
            throw ExpressionException();
    }
}

void term (bin_op* binary_op, token context) {
    switch (input_token) {
        case t_id:
        case t_literal:
        case t_lparen:
            PREDICT("predict term --> factor factor_tail" << endl);
            factor (binary_op, context);
            factor_tail (binary_op, context);
            break;
        default:
            //cerr << "Deleting token: " << token_image << endl;
            throw ExpressionException();
    }
}

void term_tail (bin_op* binary_op, token context) {
    switch (input_token) {
        case t_add:
        case t_sub:
            PREDICT("predict term_tail --> add_op term term_tail" << endl);
            add_op (binary_op);
            term (binary_op, context);
            term_tail (binary_op, context);
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
//             if (context == t_id) {
//                 token specfic_token[] = { t_eq, t_noteq, t_gt, t_lt, t_gte, t_lte };
//                 for (int i = 0; i < 8; i++) {
//                     if (specfic_token[i] == input_token)
//                         return;
//                 }
//                 throw ExpressionException();
//             }
//             else {
                PREDICT("predict term_tail --> epsilon" << endl);
                break;          /*  epsilon production */
            //}
        default:
            //cerr << "Deleting token: " << token_image << endl;
            throw ExpressionException();
    }
}

void factor_tail (bin_op* binary_op, token context) {
    switch (input_token) {
        case t_mul:
        case t_div:
            PREDICT("predict factor_tail --> mul_op factor factor_tail" << endl);
            mul_op (binary_op);
            factor (binary_op, context);
            factor_tail (binary_op, context);
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
//             if (context == t_id) {
//                 token specfic_token[] = { t_add, t_sub, t_eq, t_noteq, t_gt, t_lt, t_gte, t_lte };
//                 for (int i = 0; i < 8; i++) {
//                     if (specfic_token[i] == input_token)
//                         return;
//                 }
//                 throw ExpressionException();
//             }
//             else {
                PREDICT("predict factor_tail --> epsilon" << endl);
                break;          /*  epsilon production */
            //}
        default:
            //cerr << "Deleting token: " << token_image << endl;
            throw ExpressionException();
    }
}

void add_child_to_null_node(bin_op* root, bin_op *child) {
    if (root->l_child == NULL) {
        root->l_child = child;
    }
    else if (root->r_child == NULL) {
        root->r_child = child;
    }
    else {
        add_child_to_null_node(root->r_child, child);
    }
}

void factor (bin_op* binary_op, token context) {
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

            add_child_to_null_node(binary_op, child);

            break;
        case t_literal:
            PREDICT("predict factor --> literal" << endl);

            child = (bin_op *) malloc(sizeof(bin_op));
            child->type = t_literal;
            strcpy(child->name, token_image);
            child->l_child = NULL;
            child->r_child = NULL;

            match (t_literal, false);

            add_child_to_null_node(binary_op, child);

            break;
        case t_lparen:
            PREDICT("predict factor --> lparen expr rparen" << endl);
            match (t_lparen, false);

            child = relation ();

            // find null child
            add_child_to_null_node(binary_op, child);

            match (t_rparen, false);
            break;
        default:
            //cerr << "Deleting token: " << token_image << endl;
            throw ExpressionException();
    }
}

// if bin_op's type is not t_none
// create a new node and swap it with the right node
void add_or_create_swap_node(bin_op* binary_op, token tok) {
    if (binary_op->type == t_none) {
        binary_op->type = tok;
        strcpy(binary_op->name, print_names[tok]);
    } else {
        bin_op* new_node = (bin_op*) malloc(sizeof(bin_op));
        new_node->type = tok;
        strcpy(new_node->name, print_names[tok]);
        new_node->l_child = binary_op->r_child;
        binary_op->r_child = new_node;
    }
}

void relation_op(bin_op* binary_op) {
    switch (input_token) {
        case t_eq:
            PREDICT("predict relation_op --> ==" << endl);
            match (t_eq, false);

            add_or_create_swap_node(binary_op, t_eq);

            break;
        case t_noteq:
            PREDICT("predict relation_op --> <>" << endl);
            match (t_noteq, false);

            add_or_create_swap_node(binary_op, t_noteq);

            break;
        case t_lt:
            PREDICT("predict relation_op --> <" << endl);
            match (t_lt, false);

            add_or_create_swap_node(binary_op, t_lt);

            break;
        case t_gt:
            PREDICT("predict relation_op --> >" << endl);
            match (t_gt, false);

            add_or_create_swap_node(binary_op, t_gt);

            break;
        case t_lte:
            PREDICT("predict relation_op --> <=" << endl);
            match (t_lte, false);

            add_or_create_swap_node(binary_op, t_lte);

            break;
        case t_gte:
            PREDICT("predict relation_op --> >=" << endl);
            match (t_gte, false);

            add_or_create_swap_node(binary_op, t_gte);

            break;
        default:
            //cerr << "Deleting token: " << token_image << endl;
            throw ExpressionException();
    }
}

void add_op (bin_op* binary_op) {
    switch (input_token) {
        case t_add:
            PREDICT("predict add_op --> add" << endl);
            match (t_add, false);

            add_or_create_swap_node(binary_op, t_add);

            break;
        case t_sub:
            PREDICT("predict add_op --> sub" << endl);
            match (t_sub, false);

            add_or_create_swap_node(binary_op, t_sub);

            break;
        default:
            //cerr << "Deleting token: " << token_image << endl;
            throw ExpressionException();
    }
}

void mul_op (bin_op* binary_op) {
    switch (input_token) {
        case t_mul:
            PREDICT("predict mul_op --> mul" << endl);
            match (t_mul, false);

            add_or_create_swap_node(binary_op, t_mul);

            break;
        case t_div:
            PREDICT("predict mul_op --> div" << endl);
            match (t_div, false);

            add_or_create_swap_node(binary_op, t_div);

            break;
        default:
            //cerr << "Deleting token: " << token_image << endl;
            throw ExpressionException();
    }
}

/*--------------------------------- semantic check -----------------------------------------------*/

bool check_inside_do(st_list* root) {
    if (root->l_child && root->l_child->type == t_check) {
        return true;
    }
    else if (root->r_child) {
        return check_inside_do(root->r_child);
    }
    else
        return false;
}

void analysis_do_has_check(st_list *root) {
    static int count = 1;

    if (!root)
        return;

    if (root->l_child) {
        // see if check in do
        // and at least one check is inside it ant not nested
        if (root->l_child->type == t_do) {
            if (!check_inside_do(root->l_child->sl)) {
                cout << "do [" << count << "] has no check in it" << endl;
            }
            else {
                cout << "do [" << count << "] has check in it" << endl;
            }
            count++;
            analysis_do_has_check(root->l_child->sl);
        }

        // should check do in if
        if (root->l_child->type == t_if) {
            analysis_do_has_check(root->l_child->sl);
        }
    }

    // goto find the next do
    if (root->r_child) {
        analysis_do_has_check(root->r_child);
    }
}

void analysis_check_in_do(st_list *root, bool is_check) {
    static int count = 1;

    if (root->l_child && root->l_child->type == t_check) {
        if (is_check) {
            cout << "check [" << count << "] is in do" << endl;
        }
        else {
            cout << "check [" << count << "] not in do" << endl;
        }
        count++;
    }
    else if (root->l_child && root->l_child->type == t_if) {
        analysis_check_in_do(root->l_child->sl, false);
    }
    else if (root->l_child && root->l_child->type == t_do) {
        analysis_check_in_do(root->l_child->sl, true);
    }

    if (root->r_child)
        analysis_check_in_do(root->r_child, is_check);
}

/*------------------------------------------------------------------------------------------------*/

int main () {
    input_token = scan ();
    program ();

    cout << endl << "--------- static semantic check ---------" << endl;
    cout << "test do has check" << endl;
    analysis_do_has_check(pg_sl_root);
    cout << "test check in do" << endl;
    analysis_check_in_do(pg_sl_root, false);

    return 0;
}
