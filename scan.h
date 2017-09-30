/* Definitions the scanner shares with the parser
    Michael L. Scott, 2008-2017.
*/

#ifndef __SCAN_H
#define __SCAN_H

enum token {
    t_read, t_write, t_id, t_literal, t_gets,
    t_add, t_sub, t_mul, t_div, t_lparen, t_rparen, t_eof,
    t_if, t_fi, t_do, t_od, t_check,
    t_eq, t_noteq, t_lt, t_gt, t_lte, t_gte, t_none
};

extern char token_image[100];

extern token scan();
extern token get_next_token();

extern int lineno;

#endif
