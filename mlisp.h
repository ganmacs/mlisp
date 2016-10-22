#ifndef MLISP_H
#define MLISP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
  NODE_INT,
  NODE_SYMBOL,
  NODE_CELL,

  NODE_NIL,
  NODE_TRUE,
  NODE_DOT,
  NODE_RPAREN
} node_type_t;

/* abstract syntax tree */
typedef struct node_t {
  node_type_t type;

  union {
    int value;                  /* store integer */

    char *name;                 /* store string */

    struct {                    /* store cell */
      struct node_t *car;
      struct node_t *cdr;
    };
  };
} node_t;

typedef enum {
  T_INT,
  T_SYMBOL,
  T_PRIMITIVE,
  T_FUNCTION,
  T_CELL,
  /* T_ENV, */
  T_MOVED,

  T_NIL,
  T_TRUE
} type_t;


typedef struct obj_t *primitive_t(struct obj_t **env, struct obj_t *args);

/* mlisp object */
typedef struct obj_t {
  type_t type;

  /* int size; */

  union {
    int value;                  /* store integer */

    char *name;                 /* store string */

    primitive_t *fn;            /* store primitive (function) */

    struct {                    /* store cell */
      struct obj_t *car;
      struct obj_t *cdr;
    };
  };
} obj_t;

/* mlisp.c */
void error(char *msg);

/* parse.c */
node_t *parse();

/* debug */
void print_node(node_t *node);
void print_obj(obj_t *obj);

#endif  /* MLISP_H */
