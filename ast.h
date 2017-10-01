#ifndef __AST_H
#define __AST_H

#include <iostream>
#include "scan.h"

typedef struct _st_list st_list;
typedef struct _st st;
typedef struct _bin_op bin_op;


/*
 * the order must be same as token
 */

struct _st_list {
    st* l_child;
    _st_list* r_child;
};

struct _st {
    token type;         // id, read, write, if, do, check
    char id[100];
    bin_op* rel;
    st_list* sl;
};

struct _bin_op {
    token type;
    char name[100];
    struct _bin_op* l_child;
    struct _bin_op* r_child;
};

void print_program_ast(st_list* root);
void print_stmt_list(st_list* root);
void print_relation(bin_op* root);

#endif
