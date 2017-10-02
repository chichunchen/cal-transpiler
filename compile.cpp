#include "compile.h"
#include "debug.h"
#include <set>
#include <string>
#include <cstdlib>
#include <fstream>
#include <unistd.h>

using namespace std;

void compile_program_ast(st_list* root);
void compile_stmt_list(st_list *root);
void compile_relation(bin_op* root);

set<string> variables;
ofstream outputC;

void compileToC(st_list* root)  {
    outputC.open ("test.c");
    compile_program_ast(root);
    outputC.close();
}

void parse_variable(st_list* root) {
    if (!root)
        return;
    if (root->l_child) {
        if (root->l_child->type == t_id || root->l_child->type == t_read) {
            variables.insert(string(root->l_child->id));
        }
        else if (root->l_child->type == t_if || root->l_child->type == t_do) {
            parse_variable(root->l_child->sl);
        }
    }
    parse_variable(root->r_child);
}

void compile_variables(st_list* root) {
    parse_variable(root);
    for (set<string>::iterator it = variables.begin(); it != variables.end(); it++) {
        outputC << "int " << *it << ";" << endl;
    }
}

void compile_program_ast(st_list* root) {
    outputC << "#include <stdio.h>" << endl << endl;
    outputC << "int main() {" << endl;
    compile_variables(root);
    compile_stmt_list(root);
    outputC << endl <<  "return 0;";
    outputC << endl << "}";
}

void compile_stmt_list(st_list *root) {
    if (root->l_child != NULL) {
        switch(root->l_child->type) {
            case t_id:
                outputC << root->l_child->id << " = ";
                compile_relation(root->l_child->rel);
                outputC << ";" << endl;
                break;
            case t_read:
                outputC << "scanf(\"%d\", &" << root->l_child->id << ");" << endl;
                break;
            case t_write:
                outputC << "printf(\"%d\\n\",";
                compile_relation(root->l_child->rel);
                outputC << ");" << endl;
                break;
            case t_do:
                outputC << "while(1) {" << endl;
                compile_stmt_list(root->l_child->sl);
                outputC << "}" << endl;
                break;
            case t_if:
                outputC << "if (";
                compile_relation(root->l_child->rel);
                outputC << ") {" << endl;
                compile_stmt_list(root->l_child->sl);
                outputC << "}" << endl;
                break;
            case t_check:
                outputC << "if (!(";
                compile_relation(root->l_child->rel);
                outputC << ")) {" << endl;
                outputC << "break;" << endl << "}" << endl;
                break;
            default:
                cerr << "wrong type" << endl;
        }
        outputC << endl;
    }
    if (root->r_child != NULL)
        compile_stmt_list(root->r_child);
}

// prefix tree traversal
void compile_relation(bin_op* root) {
    if (root->l_child != NULL && root->r_child != NULL) {
        outputC << " (";
    }

    if (root->l_child) {
        compile_relation(root->l_child);
    }

    if (root) {
        if (root->type == t_id) {
            outputC << root->name;
        }
        else if (root->type == t_literal) {
            outputC << root->name;
        }
        else {
            outputC << root->name;
        }
    }

    if (root->r_child) {
        compile_relation(root->r_child);
    }

    if (root->l_child != NULL && root->r_child != NULL) {
        outputC << ")";
    }
}
