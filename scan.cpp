/* Simple ad-hoc scanner for the calculator language.
    Michael L. Scott, 2008-2017.
*/

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <stdio.h>

#include "scan.h"

using namespace std;

char token_image[100];

int lineno = 1;
token look_ahead = t_none;

void incr_line (char c) {
    if (c == '\n')
        lineno++;
}

token scan() {
    if (look_ahead != t_none) {
        token temp = look_ahead;
        look_ahead = t_none;
        return temp;
    }

    static int c = ' ';
    /* next available char; extra (int) width accommodates EOF */
    int i = 0;              /* index into token_image */

    /* skip white space */
    while (isspace(c)) {
        c = getchar();
    }
    if (c == EOF)
        return t_eof;
    if (isalpha(c)) {
        do {
            token_image[i++] = c;
            c = getchar();
        } while (isalpha(c) || isdigit(c) || c == '_');

        incr_line(c);

        token_image[i] = '\0';
        if (!strcmp(token_image, "if")) return t_if;
        else if (!strcmp(token_image, "fi")) return t_fi;
        else if (!strcmp(token_image, "do")) return t_do;
        else if (!strcmp(token_image, "od")) return t_od;
        else if (!strcmp(token_image, "read")) return t_read;
        else if (!strcmp(token_image, "write")) return t_write;
        else if (!strcmp(token_image, "check")) return t_check;
        else return t_id;
    } else if (isdigit(c)) {
        do {
            token_image[i++] = c;
            c = getchar();
        } while (isdigit(c));

        incr_line(c);

        token_image[i] = '\0';
        return t_literal;
    } else {
        switch (c) {
            case ':':
                if ((c = getchar()) != '=') {
                    cerr << "error" << endl;
                    exit(1);
                } else {
                    c = getchar();
                    return t_gets;
                }
                break;
            case '+':
                c = getchar();
                return t_add;
            case '-':
                c = getchar();
                return t_sub;
            case '*':
                c = getchar();
                return t_mul;
            case '/':
                c = getchar();
                return t_div;
            case '(':
                c = getchar();
                return t_lparen;
            case ')':
                c = getchar();
                return t_rparen;
            case '=':
                if ((c = getchar()) != '=') {
                    cerr << "error" << endl;
                    exit(1);
                } else {
                    c = getchar();
                    return t_eq;
                }
            case '<':
                c = getchar();
                if (c == '>') {
                    c = getchar();
                    return t_noteq;
                } else if (c == '=') {
                    c = getchar();
                    return t_lte;
                } else if (c == ' ') {
                    c = getchar();
                    return t_lt;
                } else {
                    cerr << "error" << endl;
                    exit(1);
                }
            case '>':
                c = getchar();
                if (c == '=') {
                    c = getchar();
                    return t_gte;
                } else if (c == ' ') {
                    c = getchar();
                    return t_gt;
                } else {
                    cerr << "error" << endl;
                    exit(1);
                }
            default:
                cout << "error" << endl;
                exit(1);
        }
        incr_line(c);
    }
}

token get_next_token () {
    look_ahead = scan();
    return look_ahead;
}