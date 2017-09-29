/* Complete recursive descent parser for the calculator language.
    Builds on figure 2.16.  Prints a trace of productions predicted and
    tokens matched.  Does no error recovery: prints "syntax error" and
    dies on invalid input.
    Michael L. Scott, 2008-2017.
*/

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <set>
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
set<int> first_S(first_S_, first_S_ + sizeof(first_S_) / sizeof(int));
set<int> follow_S(follow_S_, follow_S_ + sizeof(follow_S_) / sizeof(int));

static const int first_R_[] = {t_lparen, t_id, t_literal};
static const int follow_R_[] = {t_rparen, t_id, t_read, t_write, t_if, t_do, t_check, t_fi, t_od, t_eof};
set<int> first_R(first_R_, first_R_ + sizeof(first_R_) / sizeof(int));
set<int> follow_R(follow_R_, follow_R_ + sizeof(follow_R_) / sizeof(int));

static const int ro_[] = {t_eq, t_noteq, t_lt, t_gt, t_lte, t_gte};
static const int ao_[] = {t_add, t_sub};
static const int mo_[] = {t_mul, t_div};
set<int> ro(ro_, ro_ + sizeof(ro_) / sizeof(int));
set<int> ao(ao_, ao_ + sizeof(ao_) / sizeof(int));
set<int> mo(mo_, mo_ + sizeof(mo_) / sizeof(int));

static const int first_E_[] = {t_lparen, t_id, t_literal};
set<int> first_E(first_E_, first_E_ + sizeof(first_E_) / sizeof(int));

static const int follow_E_[] = {t_rparen, t_id, t_read, t_write, t_if, t_do, t_check, t_fi, t_od, t_eq, t_noteq, t_lt, t_gt, t_lte, t_gte, t_eof};
set<int> follow_E(follow_E_, follow_E_ + sizeof(follow_E_) / sizeof(int));

static const int starter_[] = {t_lparen, t_if, t_do};
set<int> starter(starter_, starter_ + sizeof(starter_) / sizeof(int));

//set<int> first_F = first_E;
//static const int follow_F_[] = {t_rparen, t_id, t_read, t_write, t_if, t_do, t_check, t_fi, t_od, t_eq, t_noteq, t_lt, t_gt, t_lte, t_gte, t_mul, t_div, t_add, t_sub};
//set<int> follow_F(follow_F_, follow_F_ + sizeof(follow_F_) / sizeof(int));

enum Context {
    c_stmt_list, c_stmt, c_rel, c_expr, c_expr_tail, c_term, c_term_tail, c_factor, c_factor_tail,
    c_ro, c_ao, c_mo, c_none
};

/*
 * customized exception classes
 */

struct StatementlistException : public exception {
    const char * what () const throw () {
        return "Statementlist Exception";
    }
};

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

static token input_token;

bool EPS(const char* symbol) {
	if (strcmp(symbol, "stmt_list") == 0
		|| strcmp(symbol, "expr_tail") == 0
		|| strcmp(symbol, "expr_tail") == 0
		|| strcmp(symbol, "term_tail") == 0
		|| strcmp(symbol, "factor_tail") == 0)
		return true;
	else
		return false;
}
 
set<int> FIRST(const char* symbol) {

	if (strcmp(symbol, "stmt") == 0)
		return first_S;
	
	if (strcmp(symbol, "relation") == 0)
		return first_R;

	if (strcmp(symbol, "expr") == 0)
		return first_R;
	
	if (strcmp(symbol, "stmt_list") == 0)
		return first_S;

	if (strcmp(symbol, "expr_tail") == 0)
		return ro;
	
	if (strcmp(symbol, "term") == 0)
		return first_R;

	if (strcmp(symbol, "term_tail") == 0)
		return ao;

	if (strcmp(symbol, "factor") == 0)
		return first_R;

	if (strcmp(symbol, "factor_tail") == 0)
		return mo;
}

void check_for_error(const char* symbol, set<int> follow_set) {
	set<int> first_set = FIRST(symbol);
	
	if (!(first_set.find(input_token) != first_set.end()
		|| (EPS(symbol) && follow_set.find(input_token) != follow_set.end()))) {
		cerr << endl << symbol << ".." << names[input_token] << " Context specific." << endl;
		do {
			cerr << "Delete token: " << names[input_token] << endl;
			input_token = scan();

		} while (!(first_set.find(input_token) != first_set.end()
				|| follow_set.find(input_token) != follow_set.end()
				|| starter.find(input_token) != starter.end())); 
	}

}

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
void stmt_list ();
void stmt ();
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
            match (t_eof, false);
            break;
        default: error ();
    }
    AST(endl << ")");
}

void stmt_list () {
	set<int> follow_set;
	follow_set.insert(t_eof);
	check_for_error(__FUNCTION__, follow_set);
	
	try{
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
//			case t_fi:
//			case t_od:
				PREDICT("predict stmt_list --> epsilon" << endl);
				break;          /*  epsilon production */
			default:
				cerr << "Deleting token: " << token_image << endl;
				throw StatementlistException();
		}
	} catch (StatementlistException ste) {
		cerr << ste.what() << " " << token_image << " , line number: " << lineno << endl;

        while ((input_token = scan())) {
            // recover
            if (find(first_S.begin(), first_S.end(), input_token) != first_S.end()) {
                cerr << "first: in lineno: " << lineno << ", token: " << token_image << endl;
                stmt();
                input_token = scan();
                return;
            } else if (input_token != t_eof) {
                cerr << "follow:  in lineno: " << lineno << ", token: " << token_image << endl;
                input_token = scan();
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

void stmt () {
    bin_op* root;
	check_for_error(__FUNCTION__, follow_S);
    try {
        switch (input_token) {
            case t_id:
                PREDICT("predict stmt --> id gets expr" << endl);
                AST(":= ");
                AST("\"");
                match (t_id, true);
                AST("\"");
                match (t_gets, false);
                // the bracket only show while there is more than one child
                print_relation(relation ());
                break;
            case t_read:
                PREDICT("predict stmt --> read id" << endl);
                match (t_read, false);
                AST("read ");
                AST("\"");
                match (t_id, true);
                AST("\"");
                break;
            case t_write:
                PREDICT("predict stmt --> write relation" << endl);
                match (t_write, false);
                AST("write");
                print_relation(relation ());
                break;
            case t_if:
                PREDICT("predict stmt --> if R SL fi" << endl);
                match (t_if, false);
                AST("if\n");
                print_relation(relation ());
                AST(endl << "[ ");
                stmt_list ();
                AST("]" << endl);
                match (t_fi, false);
                break;
            case t_do:
                PREDICT("predict stmt --> do SL od" << endl);
                match (t_do, false);
                AST("do\n");
                AST("[ ");
                stmt_list ();
                AST("]" << endl);
                match (t_od, false);
                break;
            case t_check:
                PREDICT("predict stmt --> check R" << endl);
                match (t_check, false);
                AST("check");
                print_relation(relation ());
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
                cerr << "first: in lineno: " << lineno << ", token: " << token_image << endl;
                stmt();
                input_token = scan();
                return;
            } else if (find(follow_S.begin(), follow_S.end(), input_token) != follow_S.end()) {
                cerr << "follow:  in lineno: " << lineno << ", token: " << token_image << endl;
                input_token = scan();
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

	check_for_error(__FUNCTION__, follow_R);
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
                cerr << "first: in lineno: " << lineno << ", token: " << token_image << endl;
                expr(binary_op);
                return binary_op;
            } else if (find(follow_R.begin(), follow_R.end(), input_token) != follow_R.end()) {
                cerr << "follow:  in lineno: " << lineno << ", token: " << token_image << endl;
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
	
	check_for_error(__FUNCTION__, follow_E);
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
        cerr << ee.what() << ": error in line number: " << lineno << endl;

        while ((input_token = scan())) {

            // recover
            if (find(first_E.begin(), first_E.end(), input_token) != first_E.end()) {
                cerr << "first: in lineno: " << lineno << ", token: " << names[input_token] << endl;
                expr(binary_op);
                return;
            } else if (find(follow_E.begin(), follow_E.end(), input_token) != follow_E.end()) {
                cerr << "follow:  in lineno: " << lineno << ", token: " << names[input_token] << endl;
                return;
            } else {
                cerr << "deleting token: " << names[input_token] << ", error in lineno: " << lineno << endl;
                input_token = scan();

                if (input_token == t_eof)
                    return;
            }
        }
    }
}

void expr_tail(bin_op* binary_op) {
    
	check_for_error(__FUNCTION__, follow_R);
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
    set<int> follow_set = follow_E;
	follow_set.insert(ao.begin(), ao.end());
	check_for_error(__FUNCTION__, follow_set);
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
	check_for_error(__FUNCTION__, follow_E);
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
    set<int> follow_set = follow_E;
	follow_set.insert(ao.begin(), ao.end());
	check_for_error(__FUNCTION__, follow_set);
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
    
	set<int> follow_set = follow_E;
	follow_set.insert(ao.begin(), ao.end());
	follow_set.insert(mo.begin(), mo.end());
	check_for_error(__FUNCTION__, follow_set);

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

    const char* print_names[] = {"read", "write", "id", "literal", "gets",
                           "+", "-", "*", "/", "lparen", "rparen", "eof",
                           "if", "fi", "do", "od", "check",
                           "==", "<>", "<", ">", "<=", ">=", "none"};

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

int main () {
    input_token = scan ();
    program ();
    return 0;
}
