#pragma once

#include "shell_types.h"
#include <stdbool.h>


Args *create_args_obj();
void add_cmd_args(Args *ao);
static char *get_normal_arg(void);
static void skip_past_spaces(void);
static char *get_single_quote_arg(void);
static char *skip_past_adjacent_quotes_and_combine(char *first_arg,
                                                   char type_of_quote);
static char *get_double_quote_arg(void);
static char *check_empty_quoted_arg(char *first_arg);
static char handle_backslash_char(BKSLSH_MODE bm);
static bool empty_single_quotes_in_normal_arg(void);
static bool empty_double_quotes_in_normal_arg(void);
static bool start_of_double_quote_text(void);
static bool start_of_single_quoted_text(void);

