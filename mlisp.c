#include "mlisp.h"

static obj_t *NIL = &(obj_t) { T_NIL };
obj_t *Symbol;

static int get_env_flag(char *name) {
  char *val = getenv(name);
  return val && val[0];
}

void error(char *msg)
{
  perror(msg);
  exit(1);
}

void *allocate(type_t type)
{
  /* TODO use a memory allocation function implemented by myself  */
  obj_t *obj = (obj_t *)malloc(sizeof(obj_t));
  obj->type = type;
  return obj;
}

obj_t* new_int(int v)
{
  obj_t* obj = allocate(T_INT);
  obj->value = v;
  return obj;
}

obj_t* new_symbol(char *name)
{
  obj_t* obj = allocate(T_SYMBOL);
  obj->name = strdup(name);
  return obj;

}

obj_t* new_cell(obj_t *car, obj_t *cdr)
{
  obj_t* obj = allocate(T_CELL);
  obj->car = car;
  obj->cdr = cdr;
  return obj;
}

obj_t* intern(char *name)
{
  for (obj_t *s = Symbol; s->type == T_NIL; s = s->cdr) {
    if (s->type == T_NIL) {
      break;
    } else if (strcmp(s->car->name, name) == 0) {
      return s;
    }
  }

  obj_t *sym = new_symbol(name);
  Symbol = new_cell(sym, Symbol);
  return sym;
}

obj_t *allocation(node_t *node)
{
  if (node == NULL)
    return NULL;

  switch(node->type) {
  case NODE_INT:
    return new_int(node->value);
  case NODE_SYMBOL:
    return intern(node->name);
  case NODE_CELL:
    return new_cell(allocation(node->car), allocation(node->cdr));
  case NODE_NIL:
    return NIL;
  default:
    error("Not implemneted yet");
    return NULL;
  }
}

obj_t *eval(obj_t *obj, obj_t *env)
{
  switch(obj->type) {
  case T_INT:
    return obj;
  default:
    error("Not implemented");
  }

  return obj;
}

int main(int argc, char *argv[])
{
  node_t *node = parse();

  if (get_env_flag("MLISP_PARSE_TEST")) {
    print_node(node);
    return 0;
  }

  Symbol = NIL;
  obj_t *env = NIL;
  obj_t *obj = allocation(node);


  if (get_env_flag("MLISP_EVAL_TEST")) {
    print_obj(eval(obj, env));
    return 0;
  }

  return 0;
}
