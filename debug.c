#include "mlisp.h"

void print_node(node_t *node)
{
  switch(node->type) {
  case NODE_INT:
    printf("%d", node->value);
    return;
  case NODE_SYMBOL:
    printf("%s", node->name);
    return;
  case NODE_CELL:
    printf("(");
    node_t *car = node->car;
    node_t *cdr = node->cdr;
    print_node(car);

    if (cdr->type != NODE_NIL && cdr->type != NODE_CELL) {
      printf(" . ");
      print_node(cdr);
    } else {
      for (; cdr->type != NODE_NIL; cdr = cdr->cdr) {
        printf(" ");
        print_node(cdr->car);
      }
    }

    printf(")");
    return;
  case NODE_NIL:
    printf("nil");
    return;
  case NODE_TRUE:
    printf("t");
    return;
  default:
    printf("others(May be error)");
    return;
  }
}

    return;
  case NODE_NIL:
    printf("nil");
    return;
  default:
    printf("others(May be error)");
    return;
  }
}
