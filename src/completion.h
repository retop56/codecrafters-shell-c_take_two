#include <stdbool.h>

char *built_in_generator(const char *text, int state);
char *executable_name_generator(const char *text, int state);
char **completion_func(const char *text, int start, int end);
void add_partial_completion(char *text_to_insert, bool add_space);
