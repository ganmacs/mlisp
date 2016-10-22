#include "mlisp.h"

static int get_env_flag(char *name) {
  char *val = getenv(name);
  return val && val[0];
}

void error(char *msg)
{
  perror(msg);
  exit(1);
}


int main(int argc, char *argv[])
{
  node_t *node = parse();

  if (get_env_flag("MLISP_PARSE_TEST")) {
    print_node(node);
    return 0;
  }

  return 0;
}
