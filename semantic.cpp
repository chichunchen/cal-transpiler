#include "semantic.h"

using namespace std;

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
