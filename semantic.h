#ifndef __SEMANTIC_H
#define __SEMANTIC_H

#include "ast.h"
#include "scan.h"

bool semantic_analysis(st_list* root);
void analysis_do_has_check(st_list *root);
void analysis_check_in_do(st_list *root, bool is_check);

#endif
