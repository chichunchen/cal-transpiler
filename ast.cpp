#include "ast.h"

using namespace std;

void print_program_ast(st_list* root) {
    cout << "(program" << endl;
    cout << "[ ";
    print_stmt_list(root);
    cout << "] ";
    cout << endl << ") ";
}

void print_stmt_list(st_list* root) {
    if (root->l_child != NULL) {
        cout << "(";
        switch(root->l_child->type) {
            case t_id:
                cout << ":= \"" << root->l_child->id << "\"";
                print_relation(root->l_child->rel);
                break;
            case t_read:
                cout << "read \"" << root->l_child->id << "\"";
                break;
            case t_write:
                cout << "write ";
                print_relation(root->l_child->rel);
                break;
            case t_do:
                cout << "do" << endl;

                cout << "[" << endl;
                print_stmt_list(root->l_child->sl);
                cout << "]" << endl;
                break;
            case t_if:
                cout << "if " << endl;
                print_relation(root->l_child->rel);

                cout << "[" << endl;
                print_stmt_list(root->l_child->sl);
                cout << "]" << endl;
                break;
            case t_check:
                cout << "check ";
                print_relation(root->l_child->rel);
                break;
            default:
                cerr << "wrong type" << endl;
        }
        cout << ")" << endl;
    }
    if (root->r_child != NULL)
        print_stmt_list(root->r_child);
}

// prefix tree traversal
void print_relation(bin_op* root) {
    if (root->l_child != NULL && root->r_child != NULL) {
        cout << " (";
    }

    if (root) {
        if (root->type == t_id) {
            cout << "(id \"";
            cout << root->name;
            cout << "\")";
        }
        else if (root->type == t_literal) {
            cout << "(num \"";
            cout << root->name;
            cout << "\")";
        }
        else {
            // print op
            cout << root->name;
        }
    }

    if (root->l_child) {
        cout << " ";
        print_relation(root->l_child);
    }

    if (root->r_child) {
        cout << " ";
        print_relation(root->r_child);
    }

    if (root->l_child != NULL && root->r_child != NULL) {
        cout << ")";
    }
}

