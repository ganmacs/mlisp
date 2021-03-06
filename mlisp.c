#include "mlisp.h"
#include <sys/mman.h>

obj_t *eval(obj_t **env, obj_t *obj);
obj_t *prim_progn(struct obj_t **env, struct obj_t *args);

static obj_t *NIL = &(obj_t) { T_NIL };
static obj_t *TRUE = &(obj_t) { T_TRUE };
obj_t *Symbol;

size_t mem_used;
void *memory;

int GC_LOCK;

static int get_env_flag(char *name) {
  char *val = getenv(name);
  return val && val[0];
}

void error(char *msg)
{
  perror(msg);
  exit(1);
}

void *allocate_space()
{
  return mmap(NULL, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
}

void gc_clear_marked(obj_t **env)
{
  /* Maybe fix  NULL ... */
  for (obj_t *o = memory; o == NULL; o = o->meta.next) {
    o->meta.marked = UNMARK;
  }
}

void gc_mark(obj_t **env)
{
  gc_clear_marked(env);

  for (obj_t *e = (*env); e->type != T_NIL; e = e->cdr) {
    if (e->type == T_NIL)
      return;

    e->car->meta.marked = MARK;
  }
}

void gc_sweep()
{
  /* The first address of memory should not be NULL */
  obj_t *v = memory;
  obj_t *prev = NULL;

  while (v->meta.next != NULL) {
    if (v->meta.marked) {
      if (prev != NULL)
        prev->meta.next = v;

      prev = v;
    } else {
      /* collected */
    }

    v = v->meta.next;
  }

  if (v->meta.marked == MARK) {
    prev->meta.next = v;
  } else {
    prev->meta.next = NULL;
  }
}

void gc(obj_t **env)
{
  if (GC_LOCK)
    return;

  gc_mark(env);
  gc_sweep();
}

obj_t *allocate(obj_t **env, type_t type)
{
  size_t size = sizeof(obj_t);
  obj_t *obj = (obj_t *)(memory + mem_used);

  /* GC always runs now */
  if (1 || MEMORY_SIZE < (size + mem_used))
    gc(env);

  mem_used += size;
  obj->type = type;
  obj->meta.marked = UNMARK;
  if (MEMORY_SIZE < mem_used) {
    /* TODO fix when compaction */
    obj->meta.next = NULL;
  } else {
    obj->meta.next = (obj_t *)(memory + mem_used);
  }

  return obj;
}

obj_t *new_int(obj_t **env, int v)
{
  obj_t *obj = allocate(env, T_INT);
  obj->value = v;
  return obj;
}

obj_t *new_symbol(obj_t **env, char *name)
{
  obj_t *obj = allocate(env, T_SYMBOL);
  obj->name = strdup(name);
  return obj;
}

obj_t *new_primitive(obj_t **env, primitive_t *fn)
{
  obj_t *obj = (obj_t *)allocate(env, T_PRIMITIVE);
  obj->fn = fn;
  return obj;
}

obj_t *new_cell(obj_t **env, obj_t *car, obj_t *cdr)
{
  obj_t *obj = allocate(env, T_CELL);
  obj->car = car;
  obj->cdr = cdr;
  return obj;
}

obj_t *new_function(obj_t **env, obj_t *args, obj_t *body)
{
  obj_t *obj = allocate(env, T_FUNCTION);
  obj->args = args;
  obj->body = body;
  obj->env = env;
  return obj;
}

obj_t *new_macro(obj_t **env, obj_t *args, obj_t *body)
{
  obj_t *obj = allocate(env, T_MACRO);
  obj->args = args;
  obj->body = body;
  obj->env = env;
  return obj;
}

obj_t *intern(obj_t **env, char *name)
{
  for (obj_t *s = Symbol; s->type == T_NIL; s = s->cdr) {
    if (s->type == T_NIL) {
      break;
    } else if (strcmp(s->car->name, name) == 0) {
      return s;
    }
  }

  obj_t *sym = new_symbol(env, name);
  Symbol = new_cell(env, sym, Symbol);
  return sym;
}

obj_t *allocation(obj_t **env, node_t *node)
{
  if (node == NULL)
    return NULL;

  switch(node->type) {
  case NODE_INT:
    return new_int(env, node->value);
  case NODE_SYMBOL:
    return intern(env, node->name);
  case NODE_CELL:
    return new_cell(env, allocation(env, node->car), allocation(env, node->cdr));
  case NODE_NIL:
    return NIL;
  case NODE_TRUE:
    return TRUE;
  default:
    error("Not implemneted yet");
    return NULL;
  }
}

void define_variable(obj_t **env, char *name, obj_t *value)
{
  obj_t *sym = intern(env, name);
  obj_t *val = new_cell(env, sym, value);
  *env = new_cell(env, val, *env);
}

obj_t *find_variable(obj_t *env, char *name)
{
  if (env->type == T_NIL)
    return NULL;

  for (; env->type != T_NIL; env = env->cdr) {
    obj_t *var = env->car;
    if (var->car->type == T_SYMBOL && strcmp(var->car->name, name) == 0)
      return var->cdr;
  }

  return NULL;
}

obj_t *eval_list(obj_t **env, obj_t *args)
{
  if (args->type == T_NIL)
    return NIL;

  return new_cell(env, eval(env, args->car), eval_list(env, args->cdr));
}

/*
 * e.g) args = ((a 10) (b 20))
 */
obj_t *apply_function(obj_t **env, obj_t *fn, obj_t *args)
{
  obj_t *nenv = *env;
  obj_t *val = NIL, *arg = NIL;
  for (; args->type != T_NIL; args = args->cdr) {
    arg = args->car;
    val = new_cell(env, intern(env, arg->car->name), eval(env, arg->cdr->car));
    nenv = new_cell(env, val, nenv);
  }
  return prim_progn(&nenv, fn);
}

obj_t *apply_macro(obj_t **env, obj_t *fn, obj_t *args)
{
  obj_t *nenv = *env;
  obj_t *val = NIL, *arg = NIL;
  for (; args->type != T_NIL; args = args->cdr) {
    arg = args->car;
    /* Macro doesn't call eval to its args */
    val = new_cell(env, intern(env, arg->car->name), arg->cdr->car);
    nenv = new_cell(env, val, nenv);
  }
  return prim_progn(&nenv, fn);
}

/*
  ((a b c d) (1 2 3 4)) => ((a 1) (b 2) (c 3) (d 4))
*/
obj_t *transpose(obj_t **env, obj_t *l, obj_t* r)
{
  obj_t *ret = NIL, *ne = NIL;
  for (; l->type != T_NIL && r->type != T_NIL; l = l->cdr, r = r->cdr) {
    ne = new_cell(env, l->car, new_cell(env, r->car, ret));
    ret = new_cell(env, ne, ret);
  }
  return ret;
}

obj_t *apply(obj_t **env, obj_t *fn, obj_t *args)
{
  if (fn->type == T_PRIMITIVE) {
    return fn->fn(env, args);
  } else if (fn->type == T_FUNCTION){
    obj_t *e = new_cell(env, *fn->env, *env);
    obj_t *nargs = transpose(env, fn->args, eval_list(env, args));
    return apply_function(&e, fn->body, nargs);
  } else {
    error("Not supported yet");
    return NULL;
  }
}

obj_t *macroexpand(obj_t **env, obj_t *obj)
{
  if (obj->type != T_CELL || obj->car->type != T_SYMBOL)
    return obj;

  obj_t *val = find_variable(*env, obj->car->name);
  if (val == NULL || val->type != T_MACRO)
    return obj;

  obj_t *nargs = transpose(env, val->args, obj->cdr);
  return apply_macro(env, val->body, nargs);
}

obj_t *eval(obj_t **env, obj_t *obj)
{
  switch(obj->type) {
  case T_INT:
    return obj;
  case T_NIL:
    return NIL;
  case T_TRUE:
    return TRUE;
  case T_FUNCTION:
  case T_PRIMITIVE:
    return obj;
  case T_SYMBOL: {
    obj_t *primitve = find_variable(*env, obj->name);
    if (primitve == NULL)
      error("Unkonw symbol");

    return primitve;
  }
  case T_CELL: {
    obj_t *fn = obj->car;
    fn = eval(env, fn);

    obj_t *expanded = macroexpand(env, obj);
    if (expanded != obj)
      return eval(env, expanded);

    if (fn->type != T_PRIMITIVE && fn->type != T_FUNCTION)
      error("The head of cons should be a function");

    return apply(env, fn, obj->cdr);
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
  for (obj_t *nargs = eval_list(env, args); nargs->type != T_NIL; nargs = nargs->cdr) {
    if (nargs->car->type != T_INT)
      error("`+` is only used for int values");
    v += nargs->car->value;
  }

  return new_int(env, v);
}

obj_t *prim_minus(struct obj_t **env, struct obj_t *args)
{
  obj_t *nargs = eval_list(env, args);
  int v = 0;
  /* v1 - v2 - v3 = 0 - v1 - v2 - v3 + (v1 * 2) */
  for (obj_t *lst = nargs; lst->type != T_NIL; lst = lst->cdr) {
    if (lst->car->type != T_INT)
      error("`-` is only used for int values");
    v -= lst->car->value;
  }
  return new_int(env, v + (nargs->car->value * 2));
}

obj_t *prim_mul(struct obj_t **env, struct obj_t *args)
{
  int v = 1;
  for (obj_t *nargs = eval_list(env, args); nargs->type != T_NIL; nargs = nargs->cdr) {
    if (nargs->car->type != T_INT)
      error("`*` is only used for int values");
    v *= nargs->car->value;
  }

  return new_int(env, v);
}


obj_t *prim_div(struct obj_t **env, struct obj_t *args)
{
  if (args->car->type != T_INT)
    error("`/` is only used for int values");
  int v = args->car->value;

  obj_t *nargs = eval_list(env, args);
  for (obj_t *lst = nargs->cdr; lst->type != T_NIL;  lst = lst->cdr) {
    if (lst->car->type == T_INT && lst->car->value == 0) {
      error("Error: divided by 0");
    } else  if (lst->car->type != T_INT) {
      error("`/` is only used for int values");
    }
    v /= lst->car->value;
  }

  return new_int(env, v);
}

obj_t *prim_equal(struct obj_t **env, struct obj_t *args)
{
  obj_t *nargs = eval_list(env, args);
  obj_t *v = NULL;
  for (; nargs->type != T_NIL; nargs = nargs->cdr) {
    if (nargs->car->type != T_INT)
      error("= is only used for int values");

    if (v == NULL)
      v = nargs->car;

    if (v->value != nargs->car->value)
      return NIL;
  }

  return TRUE;
}

obj_t *prim_lt(struct obj_t **env, struct obj_t *args)
{
  obj_t *nargs = eval_list(env, args);
  obj_t *v = NULL;
  for (; nargs->type != T_NIL; nargs = nargs->cdr) {
    if (nargs->car->type != T_INT)
      error("< only takes int value");

    if (v == NULL)
      v = nargs->car;

    if (v != nargs->car && v->value >= nargs->car->value)
      return NIL;

    v = nargs->car;
  }

  return TRUE;
}

obj_t *prim_lte(struct obj_t **env, struct obj_t *args)
{
  obj_t *nargs = eval_list(env, args);
  obj_t *v = NULL;
  for (; nargs->type != T_NIL; nargs = nargs->cdr) {
    if (nargs->car->type != T_INT)
      error("< only takes int value");

    if (v == NULL)
      v = nargs->car;

    if (v != nargs->car && v->value > nargs->car->value)
      return NIL;

    v = nargs->car;
  }

  return TRUE;
}

obj_t *prim_gt(struct obj_t **env, struct obj_t *args)
{
  obj_t *nargs = eval_list(env, args);
  obj_t *v = NULL;
  for (; nargs->type != T_NIL; nargs = nargs->cdr) {
    if (nargs->car->type != T_INT)
      error("< only takes int value");

    if (v == NULL)
      v = nargs->car;

    if (v != nargs->car && v->value <= nargs->car->value)
      return NIL;

    v = nargs->car;
  }

  return TRUE;
}

obj_t *prim_gte(struct obj_t **env, struct obj_t *args)
{
  obj_t *nargs = eval_list(env, args);
  obj_t *v = NULL;
  for (; nargs->type != T_NIL; nargs = nargs->cdr) {
    if (nargs->car->type != T_INT)
      error("< only takes int value");

    if (v == NULL)
      v = nargs->car;

    if (v != nargs->car && v->value < nargs->car->value)
      return NIL;

    v = nargs->car;
  }

  return TRUE;
}

int length(obj_t *lst)
{
  if (lst->type == T_NIL)
    return 0;

  return length(lst->cdr) + 1;
}

obj_t *prim_car(struct obj_t **env, struct obj_t *args)
{
  obj_t *v = eval(env, args->car);
  if (v->type != T_CELL && v->type != T_NIL)
    error("Wrong type argument");
  return v->car;
}

obj_t *prim_cdr(struct obj_t **env, struct obj_t *args)
{
  obj_t *v = eval(env, args->car);
  if (v->type != T_CELL && v->type != T_NIL)
    error("Wrong type argument");
  return v->cdr;
}

obj_t *prim_cons(struct obj_t **env, struct obj_t *args)
{
  obj_t *v = eval_list(env, args);
  if (length(v) != 2)
    error("cons: Wrong number of arguments");

  return new_cell(env, v->car, v->cdr->car);
}

obj_t *prim_quote(struct obj_t **env, struct obj_t *args)
{
  if (length(args) != 1)
    error("quote: Wron number of arguments");
  return args->car;
}

obj_t *prim_progn(struct obj_t **env, struct obj_t *args)
{
  obj_t *ret = NIL;
  for (; args->type != T_NIL ; args = args->cdr) {
    ret = eval(env, args->car);
  }
  return ret;
}

obj_t *prim_let(struct obj_t **env, struct obj_t *args)
{
  return apply_function(env, args->cdr, args->car);
}

obj_t *prim_if(struct obj_t **env, struct obj_t *args)
{
  if (length(args) < 2)
    error("if: Wrong number of arguments");

  obj_t *cond = eval(env, args->car);

  if (cond->type == T_NIL) {
    if (args->cdr->cdr->type == T_NIL)  /* without false clause */
      return NIL;

    obj_t *false_clause = args->cdr->cdr->car;
    return eval(env, false_clause);
  } else {
    obj_t *true_clause = args->cdr->car;
    return eval(env, true_clause);
  }
}

obj_t *prim_define(struct obj_t **env, struct obj_t *args)
{
  if (length(args) != 2)
    error("define: Wrong number of arguments");

  obj_t *val = eval(env, args->cdr->car);
  define_variable(env, args->car->name, val);
  return val;
}

obj_t *prim_defun(struct obj_t **env, struct obj_t *args)
{
  if (length(args) != 3)
    error("defun: Wrong number of arguments");

  obj_t *vargs = args->cdr->car;
  obj_t *body = args->cdr->cdr;
  obj_t *fn = new_function(env, vargs, body);
  define_variable(env, args->car->name, fn);
  return NIL;
}

obj_t *prim_lambda(struct obj_t **env, struct obj_t *args)
{
  if (length(args) != 2)
    error("lambda: Wrong number of arguments");

  for (obj_t *nargs = args->car; nargs->type != T_NIL; nargs = nargs->cdr) {
    if (nargs->car->type != T_SYMBOL)
      error("Parameter should be a symbol");
  }

  return new_function(env, args->car, args->cdr);
}

obj_t *prim_list(struct obj_t **env, struct obj_t *args)
{
  if (args->type == T_NIL)
    return args;

  return new_cell(env, eval(env, args->car), prim_list(env, args->cdr));
}

obj_t *prim_defmacro(struct obj_t **env, struct obj_t *args)
{
  if (length(args) != 3)
    error("defmacro: Wrong number of arguments");

  obj_t *vargs = args->cdr->car;
  obj_t *body = args->cdr->cdr;
  obj_t *fn = new_macro(env, vargs, body);
  define_variable(env, args->car->name, fn);
  return NIL;
}

obj_t *prim_macroexpand(struct obj_t **env, struct obj_t *args)
{
  return macroexpand(env, eval(env, args->car));
}

void define_primitives(char *name, primitive_t *fn, obj_t **env)
{
  obj_t *prim = new_primitive(env, fn);
  define_variable(env, name, prim);
}

void initialize(obj_t **env)
{
  GC_LOCK = 1;
  memory = allocate_space();
  mem_used = 0;
  Symbol = NIL;
  define_primitives("+", prim_plus, env);
  define_primitives("-", prim_minus, env);
  define_primitives("*", prim_mul, env);
  define_primitives("/", prim_div, env);
  define_primitives("=", prim_equal, env);
  define_primitives("<", prim_lt, env);
  define_primitives("<=", prim_lte, env);
  define_primitives(">", prim_gt, env);
  define_primitives(">=", prim_gt, env);
  define_primitives("car", prim_car, env);
  define_primitives("cdr", prim_cdr, env);
  define_primitives("cons", prim_cons, env);
  define_primitives("list", prim_list, env);
  define_primitives("quote", prim_quote, env);
  define_primitives("progn", prim_progn, env);
  define_primitives("let", prim_let, env);
  define_primitives("lambda", prim_lambda, env);
  define_primitives("if", prim_if, env);
  define_primitives("define", prim_define, env);
  define_primitives("defun", prim_defun, env);
  define_primitives("defmacro", prim_defmacro, env);
  define_primitives("macroexpand", prim_macroexpand, env);
  GC_LOCK = 0;
}

int main(int argc, char *argv[])
{
  node_t *node = parse();

  if (get_env_flag("MLISP_PARSE_TEST")) {
    print_node(node);
    return 0;
  }

  obj_t *env = NIL;

  initialize(&env);
  obj_t *obj = allocation(&env, node);
  obj_t *ret = eval(&env, obj);

  if (get_env_flag("MLISP_EVAL_TEST")) {
    print_obj(ret);
    return 0;
  }

  print_obj(ret);
  return 0;
}
