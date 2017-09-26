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
 * the order must be same as token
 */
const char* names[] = {"read", "write", "id", "literal", "gets",
                       "add", "sub", "mul", "div", "lparen", "rparen", "eof",
                       "if", "fi", "do", "od", "check", "ro",
                       "eq", "noteq", "lt", "gt", "lte", "gte" };

static token input_token;

void error () {
    cout << "syntax error" << endl;
    exit (1);
}

void match (token expected) {
    if (input_token == expected) {
        PREDICT("matched " << names[input_token]);
        if (input_token == t_id || input_token == t_literal)
            PREDICT(": " << token_image);
        PREDICT(endl);
        input_token = scan ();
    }
    else error ();
}

void program ();
void stmt_list ();
void stmt ();
void relation ();
void expr ();
void expr_tail();
void term_tail ();
void term ();
void factor_tail ();
void factor ();
void relation_op();
void add_op ();
void mul_op ();

void program () {
    switch (input_token) {
        case t_id:
        case t_read:
        case t_write:
        case t_if:
        case t_do:
        case t_check:
        case t_eof:
            PREDICT("predict program --> stmt_list eof" << endl);
            stmt_list ();
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
            stmt ();
            stmt_list ();
            break;
        /* Follow(stmt) */
        case t_eof:
        case t_fi:
        case t_od:
            PREDICT("predict stmt_list --> epsilon" << endl);
            break;          /*  epsilon production */
        default:
            cout << "error: " << input_token << endl;
            error ();
    }
}

void stmt () {
    switch (input_token) {
        case t_id:
            PREDICT("predict stmt --> id gets expr" << endl);
            match (t_id);
            match (t_gets);
            relation ();
            break;
        case t_read:
            PREDICT("predict stmt --> read id" << endl);
            match (t_read);
            match (t_id);
            break;
        case t_write:
            PREDICT("predict stmt --> write relation" << endl);
            match (t_write);
            relation ();
            break;
        case t_if:
            PREDICT("predict stmt --> if R SL fi" << endl);
            match (t_if);
            relation ();
            stmt_list ();
            match (t_fi);
            break;
        case t_do:
            PREDICT("predict stmt --> do SL od" << endl);
            match (t_do);
            stmt_list ();
            match (t_od);
            break;
        case t_check:
            PREDICT("predict stmt --> check R" << endl);
            match (t_check);
            relation ();
            break;
        default: error ();
    }
}

void relation() {
    switch (input_token) {
        case t_id:
        case t_literal:
        case t_lparen:
            PREDICT("predict relation --> expr expr_tail" << endl);
            expr();
            expr_tail ();
            break;
        default: error ();
    }
}

void expr () {
    switch (input_token) {
        case t_id:
        case t_literal:
        case t_lparen:
            PREDICT("predict expr --> term term_tail" << endl);
            term ();
            term_tail ();
            break;
        default: error ();
    }
}

void expr_tail() {
    switch (input_token) {
        case t_eq:
        case t_noteq:
        case t_lt:
        case t_gt:
        case t_lte:
        case t_gte:
            relation_op();
            expr();
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
            cout << "error: " << input_token << endl;
            error();
    }
}

void term_tail () {
    switch (input_token) {
        case t_add:
        case t_sub:
            PREDICT("predict term_tail --> add_op term term_tail" << endl);
            add_op ();
            term ();
            term_tail ();
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
        case t_ro:
            PREDICT("predict term_tail --> epsilon" << endl);
            break;          /*  epsilon production */
        default:
            error ();
            break;
    }
}

void term () {
    switch (input_token) {
        case t_id:
        case t_literal:
        case t_lparen:
            PREDICT("predict term --> factor factor_tail" << endl);
            factor ();
            factor_tail ();
            break;
        default: error ();
    }
}

void factor_tail () {
    switch (input_token) {
        case t_mul:
        case t_div:
            PREDICT("predict factor_tail --> mul_op factor factor_tail" << endl);
            mul_op ();
            factor ();
            factor_tail ();
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
        case t_ro:
            PREDICT("predict factor_tail --> epsilon" << endl);
            break;          /*  epsilon production */
        default:
            cout << "error: " << input_token << endl;
            error ();
    }
}

void factor () {
    switch (input_token) {
        case t_id :
            PREDICT("predict factor --> id" << endl);
            match (t_id);
            break;
        case t_literal:
            PREDICT("predict factor --> literal" << endl);
            match (t_literal);
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

void relation_op() {
    switch (input_token) {
        case t_eq:
            PREDICT("predict relation_op --> ==" << endl);
            match (t_eq);
            break;
        case t_noteq:
            PREDICT("predict relation_op --> <>" << endl);
            match (t_noteq);
            break;
        case t_lt:
            PREDICT("predict relation_op --> <" << endl);
            match (t_lt);
            break;
        case t_gt:
            PREDICT("predict relation_op --> >" << endl);
            match (t_gt);
            break;
        case t_lte:
            PREDICT("predict relation_op --> <=" << endl);
            match (t_lte);
            break;
        case t_gte:
            PREDICT("predict relation_op --> >=" << endl);
            match (t_gte);
            break;
        default: error ();
    }
}

void add_op () {
    switch (input_token) {
        case t_add:
            PREDICT("predict add_op --> add" << endl);
            match (t_add);
            break;
        case t_sub:
            PREDICT("predict add_op --> sub" << endl);
            match (t_sub);
            break;
        default: error ();
    }
}

void mul_op () {
    switch (input_token) {
        case t_mul:
            PREDICT("predict mul_op --> mul" << endl);
            match (t_mul);
            break;
        case t_div:
            PREDICT("predict mul_op --> div" << endl);
            match (t_div);
            break;
        default: error ();
    }
}

int main () {
    input_token = scan ();
    program ();
    return 0;
}
