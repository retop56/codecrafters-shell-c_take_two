typedef enum Backslash_Mode{ 
  INSIDE_DOUBLE_QUOTES,
  OUTSIDE_QUOTES
}BKSLSH_MODE;

int add_cmd_args(char **args);
static char *get_normal_arg(void);
static void skip_past_spaces(void);
static char *get_single_quote_arg(void);
static char *skip_past_adjacent_quotes_and_combine(char *first_arg,
                                                   char type_of_quote);
static char *get_double_quote_arg(void);
static char *check_empty_quoted_arg(char *first_arg);
static char handle_backslash_char(BKSLSH_MODE bm);

