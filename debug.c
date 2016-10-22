#include "mlisp.h"

void _print_node(node_t *node)
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
    _print_node(car);

    if (cdr->type != NODE_NIL && cdr->type != NODE_CELL) {
      printf(" . ");
      _print_node(cdr);
    } else {
      for (; cdr->type != NODE_NIL; cdr = cdr->cdr) {
        printf(" ");
        _print_node(cdr->car);
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

void _print_obj(obj_t *obj)
{
  switch(obj->type) {
  case T_INT:
    printf("%d", obj->value);
    return;
  case T_SYMBOL:
    printf("%s", obj->name);
    return;
  case T_PRIMITIVE:
    printf("(fn () <primtive>)");
    return;
  case T_FUNCTION:
    printf("(fn () <function>)");
    return;
  case T_CELL:
    printf("(");
    _print_obj(obj->car);

    for (obj_t *o = obj->cdr; o->type != T_NIL; o = o->cdr) {
      if (o->type != T_NIL && o->type != T_CELL) {
        printf(" . ");
        _print_obj(o);
        printf(")");
        return;
      } else if (o->type != T_NIL) {
        printf(" ");
        _print_obj(o->car);
      } else {
        error("An error in print_obj");
      }
    }
    printf(")");
    return;
  case T_MOVED:
    puts("TMOVED");
    return;
  case T_NIL:
    printf("()");
    return;
  case T_TRUE:
    printf("t");
    return;
  }
}

void print_obj(obj_t *obj)
{
  if (obj == NULL)
    return;

  _print_obj(obj);
  puts("");
}

void print_node(node_t *node)
{
  if (node == NULL)
    return;

  _print_node(node);
  puts("");
}
