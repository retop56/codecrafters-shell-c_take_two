#include <stdlib.h>
#include <readline/history.h>

void check_for_history() {
  char *hist_file = getenv("HISTFILE");
  if (hist_file == NULL) {
    return;
  }
  read_history(hist_file);
}

void write_history_on_exit() {
  char *hist_file = getenv("HISTFILE");
  if (hist_file == NULL) {
    return;
  }
  write_history(hist_file);
}
