#include "mlisp.h"

obj_t *eval(obj_t *obj, obj_t **env);

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

obj_t *new_primitive(primitive_t *fn)
{
  obj_t *obj = (obj_t *)allocate(T_PRIMITIVE);
  obj->fn = fn;
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

void add_variable(obj_t **env, char *name, obj_t *value)
{
  obj_t* sym = intern(name);
  obj_t *val = new_cell(sym, value);
  *env = new_cell(val, *env);
}

obj_t *find_variable(obj_t *env, char *name)
{
  for (; env->type != T_NIL; env = env->cdr) {
    if (env->type == T_NIL) {
      return NULL;
    }

    obj_t *var = env->car;

    if (strcmp(var->car->name, name) == 0)
      return var->cdr;
  }

  return NULL;
}

obj_t *eval_list(obj_t *args, obj_t **env)
{
  if (args->type == T_NIL)
    return NIL;

  return new_cell(eval(args->car, env), eval_list(args->cdr, env));
}

obj_t *apply(obj_t *fn, obj_t *args, obj_t **env)
{
  obj_t *nargs = eval_list(args, env);
  return fn->fn(env, nargs);
}

obj_t *eval(obj_t *obj, obj_t **env)
{
  switch(obj->type) {
  case T_INT:
    return obj;
  case T_SYMBOL: {
    obj_t* primitve = find_variable(*env, obj->name);
    if (primitve == NULL)
      error("Unkonw symbol");

    return primitve;
  }
  case T_CELL: {
    obj_t *fn = obj->car;
    fn = eval(fn, env);

    if (fn->type != T_PRIMITIVE) {
      error("The head of cons should is a function");
    }

    obj_t *nargs = eval_list(obj->cdr, env);
    return apply(fn, nargs, env);
  }
  default:
    printf("%d\n", obj->type);
    error("Not implemented");
    return obj;
  }
}

obj_t *prim_plus(struct obj_t **env, struct obj_t *args)
{
  int v = 0;
  for (; args->type != T_NIL; args = args->cdr) {
    if (args->car->type != T_INT)
      error("`+` is only used for int values");
    v += args->car->value;
  }

  return new_int(v);
}

obj_t *prim_minus(struct obj_t **env, struct obj_t *args)
{
  if (args->car->type != T_INT)
    error("`-` is only used for int values");
  int v = args->car->value;

  for (obj_t *lst = args->cdr; lst->type != T_NIL;  lst = lst->cdr) {
    if (lst->car->type != T_INT)
      error("`-` is only used for int values");
    v -= lst->car->value;
  }

  return new_int(v);
}

obj_t *prim_mul(struct obj_t **env, struct obj_t *args)
{
  int v = 1;
  for (; args->type != T_NIL; args = args->cdr) {
    if (args->car->type != T_INT)
      error("`*` is only used for int values");
    v *= args->car->value;
  }

  return new_int(v);
}

obj_t *prim_div(struct obj_t **env, struct obj_t *args)
{
  if (args->car->type != T_INT)
    error("`/` is only used for int values");
  int v = args->car->value;

  for (obj_t *lst = args->cdr; lst->type != T_NIL;  lst = lst->cdr) {
    if (lst->car->type == T_INT && lst->car->value == 0) {
      error("Can't divied by 0");
    } else  if (lst->car->type != T_INT) {
      error("`/` is only used for int values");
    }
    v /= lst->car->value;
  }

  return new_int(v);
}

void define_primitives(char *name, primitive_t *fn, obj_t **env)
{
  obj_t *prim = new_primitive(fn);
  add_variable(env, name, prim);
}

void initialize(obj_t **env)
{
  define_primitives("+", prim_plus, env);
  define_primitives("-", prim_minus, env);
  define_primitives("*", prim_mul, env);
  define_primitives("/", prim_div, env);
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

  initialize(&env);

  if (get_env_flag("MLISP_EVAL_TEST")) {
    print_obj(eval(obj, &env));
    return 0;
  }

  return 0;
}
