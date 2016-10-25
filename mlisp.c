#include "mlisp.h"
#include <sys/mman.h>

obj_t *eval(obj_t *obj, obj_t **env);

static obj_t *NIL = &(obj_t) { T_NIL };
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
  for (; env->type != T_NIL; env = env->cdr) {
    if (env->type == T_NIL)
      return NULL;

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

  return new_cell(env, eval(args->car, env), eval_list(args->cdr, env));
}

obj_t *apply(obj_t *fn, obj_t *args, obj_t **env)
{
  return fn->fn(env, args);
}

obj_t *eval(obj_t *obj, obj_t **env)
{
  switch(obj->type) {
  case T_INT:
    return obj;
  case T_SYMBOL: {
    obj_t *primitve = find_variable(*env, obj->name);
    if (primitve == NULL)
      error("Unkonw symbol");

    return primitve;
  }
  case T_CELL: {
    obj_t *fn = obj->car;
    fn = eval(fn, env);

    if (fn->type != T_PRIMITIVE)
      error("The head of cons should be a function");

    return apply(fn, obj->cdr, env);
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
  for (obj_t *nargs = eval_list(args, env); nargs->type != T_NIL; nargs = nargs->cdr) {
    if (nargs->car->type != T_INT)
      error("`+` is only used for int values");
    v += nargs->car->value;
  }

  return new_int(env, v);
}

obj_t *prim_minus(struct obj_t **env, struct obj_t *args)
{
  if (args->car->type != T_INT)
    error("`-` is only used for int values");
  int v = args->car->value;

  obj_t *nargs = eval_list(args, env);
  for (obj_t *lst = nargs->cdr; lst->type != T_NIL;  lst = lst->cdr) {
    if (lst->car->type != T_INT)
      error("`-` is only used for int values");
    v -= lst->car->value;
  }

  return new_int(env, v);
}

obj_t *prim_mul(struct obj_t **env, struct obj_t *args)
{
  int v = 1;
  for (obj_t *nargs = eval_list(args, env); nargs->type != T_NIL; nargs = nargs->cdr) {
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

  obj_t *nargs = eval_list(args, env);
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

obj_t *prim_progn(struct obj_t **env, struct obj_t *args)
{
  obj_t *ret;
  for (; args->type != T_NIL ; args = args->cdr) {
    ret = eval(args->car, env);
  }
  return ret;
}

obj_t *prim_let(struct obj_t **env, struct obj_t *args)
{
  obj_t *nenv = *env;
  obj_t *sym = NIL, *val = NIL;
  for (obj_t *largs = args->car; largs->type != T_NIL; largs = largs->cdr) {
    sym = intern(env, largs->car->car->name);
    val = new_cell(env, sym, largs->car->cdr->car);
    nenv = new_cell(env, val, nenv);
  }

  return prim_progn(&nenv, args->cdr);
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
  define_primitives("progn", prim_progn, env);
  define_primitives("let", prim_let, env);
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

  if (get_env_flag("MLISP_EVAL_TEST")) {
    print_obj(eval(obj, &env));
    return 0;
  }

  return 0;
}
