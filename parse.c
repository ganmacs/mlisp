#include "mlisp.h"

#define SYMBOL_MAX_LEN 50

static node_t *Nil = &(node_t){ NODE_NIL };
static node_t *True = &(node_t){ NODE_TRUE };
static node_t *RParen = &(node_t){ NODE_RPAREN };
static node_t *Dot = &(node_t){ NODE_DOT };

static char symbol_chars[] = "+-*/<=>";

static char peek() {
  char c = getchar();
  ungetc(c, stdin);
  return c;
}

node_t *new_node_symbol(char *sym)
{
  node_t* node = (node_t *)malloc(sizeof(node_t));
  node->name = strdup(sym);
  node->type = NODE_SYMBOL;
  return node;
}

node_t *new_node_int(int value)
{
  node_t* node = malloc(sizeof(node_t));
  node->value = value;
  node->type = NODE_INT;
  return node;
}

node_t *new_node_cell(node_t* car, node_t* cdr)
{
  node_t* node = (node_t *)malloc(sizeof(node_t));
  node->type = NODE_CELL;
  node->car = car;
  node->cdr = cdr;
  return node;
}

node_t *parse_quote()
{
  node_t *v = new_node_cell(parse(), Nil);
  return new_node_cell(new_node_symbol("quote"), v);
}

node_t *parse_list()
{
  node_t *node = parse();

  if (node == NULL) {
    error("Paren is Unmatch");
  } else if (node == Dot) {   /* (a . b) */
    node_t *cdr = parse();
    if (parse() != RParen) {
      error("Paren is Unmatch");
      exit(1);
    }
    return cdr;
  } else if (node == RParen) {
    return Nil;
  }

  return new_node_cell(node, parse_list());
}

node_t *parse_symbol(char v)
{
  char buf[SYMBOL_MAX_LEN + 1];
  buf[0] = v;
  int i = 1;
  while (isalpha(peek()) || isdigit(peek()) || strchr(symbol_chars, peek())) {
    if (i >= SYMBOL_MAX_LEN)
      error("Symbol name is too long");
    buf[i++] = getchar();
  }
  buf[i] = '\0';

  if ((strlen(buf) == 1) && buf[0] == 't')
    return True;

  return new_node_symbol(buf);
}

int num(int acc) {
  if (isdigit(peek()))
    return num(acc * 10 + (getchar() - '0'));

  return acc;
}

node_t *parse_digit(char d)
{
  return new_node_int(num(d - '0'));
}

void destory_ast(node_t *node)
{
  if (node == NULL)
    return;

  switch(node->type) {
  case NODE_CELL:
    destory_ast(node->car);
    destory_ast(node->cdr);
    free(node);
    return;
  case NODE_SYMBOL:
    free(node->name);
    free(node);
    return;
  case NODE_INT:
    free(node);
    return;
  default:
    /* noop */
    return;
  }
}

node_t *parse()
{
  char c = getchar();

  if (c == '(') {
    if (peek() == ')') {
      getchar();
      return Nil;
    }
    return parse_list();
  } else if (c == ' ') {
    return parse();
  } else if (c == '\'') {
    return parse_quote();
  } else if (c == ')') {
    return RParen;
  } else if (c == '.') {
    return Dot;
  } else if (c == EOF) {
    return NULL;
  } else if (isdigit(c)) {
    return parse_digit(c);
  } else if (isalpha(c) || strchr(symbol_chars, c)) {
    return parse_symbol(c);
  }

  error("Invalid token: %c");
  return NULL;                  /* must not reach */
}
