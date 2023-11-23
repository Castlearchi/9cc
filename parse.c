#include "9cc.h"

static Node *new_node(NodeKind kind);
static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
static Node *new_unary(NodeKind kind, Node *lhs);
static Node *new_num(int val);
static LVar *find_var(Token **tok, LVar **locals);
/*
  program    = func*
  func       = declarator ( "(" func_params ")" ) "{" stmt* "}"
  declarator = declspec ident
  func_params= (param ("," param)*)?
  param      = declspec ident
  declspec   = "int" ("*")*
  stmt       = expr ";"
              | "{" stmt* "}"
              | "return" expr ";"
              | "if" "(" expr ")" stmt ("else" stmt)?
              | "while" "(" expr ")" stmt
              | "for" (" expr? ";" expr? ";" expr ")" stmt
  expr       = assign
  assign     = equality ("=" assign)?
  equality   = relational ("==" relational | "!=" relational)*
  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
  add        = mul ("+" mul | "-" mul)*
  mul        = unary ("*" unary | "/" unary)*
  unary      = ("+" | "-" | "*" | "&") unary
              | primary
  primary    = "(" expr ")" | var_init | ident funcall? | num
  var_init   = declspec ident ("=" expr)?
  funcall    = ident "(" (assign ("," assign)*)? ")"
*/

static Function *program(Token **tok);
static Function *func(Token **tok);
static Function *declarator(Token **tok);
static void func_params(Token **tok, Function *fn);
static LVar *param(Token **tok, LVar *params);
static Type *declspec(Token **tok);
static Node *stmt(Token **tok, LVar **locals);
static Node *expr(Token **tok, LVar **locals);
static Node *assign(Token **tok, LVar **locals);
static Node *equality(Token **tok, LVar **locals);
static Node *relational(Token **tok, LVar **locals);
static Node *add(Token **tok, LVar **locals);
static Node *mul(Token **tok, LVar **locals);
static Node *unary(Token **tok, LVar **locals);
static Node *primary(Token **tok, LVar **locals);
static Node *funcall(Token **tok, LVar **locals);
static Node *var_init(Token **tok, LVar **locals);

static Node *new_node(NodeKind kind)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->eof = false;
  return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs)
{
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_unary(NodeKind kind, Node *lhs)
{
  Node *node = new_node(kind);
  node->lhs = lhs;
  return node;
}

static Node *new_num(int val)
{
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node *new_var_node(LVar *var)
{
  Node *node = new_node(ND_VAR);
  node->var = var;
  return node;
}

// Search var name rbut not find return NULL.
LVar *find_var(Token **tok, LVar **locals)
{
  for (LVar *var = *locals; var; var = var->next)
    if (var->len == (*tok)->len && !memcmp((*tok)->str, var->name, var->len))
      return var;
  return NULL;
}

// program = func*
static Function *program(Token **tok)
{
  Function head = {};
  Function *cur = &head;
  while (!at_eof(tok))
  {
    cur = cur->next = func(tok);
  }

  return head.next;
}

// func       = declarator ( "(" func_params ")" ) "{" stmt* "}"
static Function *func(Token **tok)
{
  Function *fn = declarator(tok);

  expect(tok, "(");

  func_params(tok, fn);

  expect(tok, ")");

  expect(tok, "{");

  LVar **locals = (LVar **)calloc(1, sizeof(LVar *));
  *locals = (LVar *)calloc(1, sizeof(LVar));
  (*locals)->offset = fn->params->offset;
  (*locals)->next = fn->params;
  // *locals = fn->params;
  fn->body = calloc(1, sizeof(Node *));
  fn->stmt_count = 0;
  while (!consume(tok, "}"))
  {
    fn->stmt_count++;
    fn->body = realloc(fn->body, sizeof(Node *) * fn->stmt_count);
    fn->body[fn->stmt_count - 1] = stmt(tok, locals);
  }

  int stack_size = 0;
  for (LVar *var = *locals; var->next; var = var->next)
  {
    stack_size += 8;
  }
  fn->locals = locals;
  fn->stack_size = stack_size;

  return fn;
}

// declarator = declspec ident
static Function *declarator(Token **tok)
{
  Function *fn = (Function *)calloc(1, sizeof(Function));
  Type *type = declspec(tok);
  if (!expect_ident(tok))
    error_tok(tok, "Here should be function name. %d\n", (*tok)->kind);
  fn->ty = type;
  fn->name = (*tok)->str;
  *tok = (*tok)->next;
  return fn;
}
// func_params= (param ("," param)*)?
static void func_params(Token **tok, Function *fn)
{
  LVar *params = (LVar *)calloc(1, sizeof(LVar));
  int offset = 0;

  params->next = NULL;
  params->offset = offset;

  Token head = {};
  Token *cur = &head;

  int regards_num = 0;
  while (!equal(tok, ")"))
  {
    if (cur != &head)
      consume(tok, ",");
    params = param(tok, params);
    cur = *tok;
    regards_num++;
  }

  fn->params = params;
  fn->regards_num = regards_num;
}

// param      = declspec ident
static LVar *param(Token **tok, LVar *params)
{
  Type *type = declspec(tok);
  if ((*tok)->kind != TK_IDENT)
    error_tok(tok, "Here should be function argument name.\n");
  LVar *lvar = find_var(tok, &params);
  if (lvar)
    error_tok(tok, "Redeclaration of argument.\n");
  lvar = calloc(1, sizeof(LVar));
  lvar->next = params;
  lvar->name = (*tok)->str;
  lvar->len = (*tok)->len;
  lvar->offset = params->offset + 8;
  lvar->ty = type;
  params = lvar;
  *tok = (*tok)->next;
  return params;
}

// declspec = "int" ("*")*
static Type *declspec(Token **tok)
{
  Type *cur = calloc(1, sizeof(Type));
  if ((*tok)->kind != TK_TYPE)
    error_tok(tok, "Here should be type.\n");
  if (consume(tok, "int"))
  {
    if (equal(tok, "*"))
    {
      while (consume(tok, "*"))
      {
        cur->ty = PTR;
        cur = cur->ptr_to = calloc(1, sizeof(Type));
      }
    }
    cur->ty = INT;
  }
  return cur;
}

// stmt       = expr ";"
//            | "{" stmt* "}"
//            | "return" expr ";"
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "for" (" expr? ";" expr? ";" expr? ")" stmt
static Node *stmt(Token **tok, LVar **locals)
{
  Node *node;
  if (at_eof(tok))
  {
    node = calloc(1, sizeof(Node));
    node->eof = true;
    return node;
  }

  if (consume(tok, "{"))
  {
    node = new_node(ND_BLOCK);
    node->block_size = 4;
    node->block = calloc(node->block_size, sizeof(Node *));
    size_t count = 0;
    while (!consume(tok, "}"))
    {
      if (count > (node->block_size))
      {
        (node->block_size) *= 2;
        node->block = realloc(node->block, sizeof(Node *) * node->block_size);
      }
      node->block[count++] = stmt(tok, locals);
    }
    node->block_count = count;
  }
  else if (consume(tok, "return"))
  {
    node = new_node(ND_RETURN);
    node->lhs = expr(tok, locals);
    expect(tok, ";");
  }
  else if (consume(tok, "if"))
  {
    node = new_node(ND_IF);
    expect(tok, "(");
    node->cond = expr(tok, locals);
    expect(tok, ")");
    node->then = stmt(tok, locals);
    if (consume(tok, "else"))
    {
      node->kind = ND_IFELSE;
      node->els = stmt(tok, locals);
    }
  }
  else if (consume(tok, "while"))
  {

    node = new_node(ND_WHILE);
    expect(tok, "(");
    node->cond = expr(tok, locals);
    expect(tok, ")");
    node->then = stmt(tok, locals);
  }
  else if (consume(tok, "for"))
  {
    node = new_node(ND_FOR);
    expect(tok, "(");
    if (!consume(tok, ";"))
    {
      node->init = expr(tok, locals);
      expect(tok, ";");
    }
    if (!consume(tok, ";"))
    {
      node->cond = expr(tok, locals);
      expect(tok, ";");
    }
    if (!consume(tok, ")"))
    {
      node->inc = expr(tok, locals);
      expect(tok, ")");
    }
    node->then = stmt(tok, locals);
  }
  else
  {
    node = expr(tok, locals);
    expect(tok, ";");
  }
  return node;
}

// expr = assign
static Node *expr(Token **tok, LVar **locals)
{
  return assign(tok, locals);
}

// assign = equality ("=" assign)?
static Node *assign(Token **tok, LVar **locals)
{
  Node *node = equality(tok, locals);
  if (consume(tok, "="))
    node = new_binary(ND_ASSIGN, node, assign(tok, locals));
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **tok, LVar **locals)
{
  Node *node = relational(tok, locals);

  for (;;)
  {
    if (consume(tok, "=="))
      node = new_binary(ND_EQ, node, relational(tok, locals));
    else if (consume(tok, "!="))
      node = new_binary(ND_NE, node, relational(tok, locals));
    else
      return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **tok, LVar **locals)
{
  Node *node = add(tok, locals);
  for (;;)
  {
    if (consume(tok, "<"))
      node = new_binary(ND_LT, node, add(tok, locals));
    else if (consume(tok, "<="))
      node = new_binary(ND_LE, node, add(tok, locals));
    else if (consume(tok, ">"))
      node = new_binary(ND_LT, add(tok, locals), node);
    else if (consume(tok, ">="))
      node = new_binary(ND_LE, add(tok, locals), node);
    else
      return node;
  }
}
// add = mul ("+" mul | "-" mul)*
static Node *add(Token **tok, LVar **locals)
{
  Node *node = mul(tok, locals);
  for (;;)
  {
    if (consume(tok, "+"))
      node = new_binary(ND_ADD, node, mul(tok, locals));
    else if (consume(tok, "-"))
      node = new_binary(ND_SUB, node, mul(tok, locals));
    else
      return node;
  }
}
// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **tok, LVar **locals)
{
  Node *node = unary(tok, locals);

  for (;;)
  {
    if (consume(tok, "*"))
      node = new_binary(ND_MUL, node, unary(tok, locals));
    else if (consume(tok, "/"))
      node = new_binary(ND_DIV, node, unary(tok, locals));
    else
      return node;
  }
}

// unary      = ("+" | "-" | "*" | "&") unary
//             | primary
static Node *unary(Token **tok, LVar **locals)
{
  if (consume(tok, "+"))
    return unary(tok, locals);

  if (consume(tok, "-"))
    return new_unary(ND_SUB, unary(tok, locals));

  if (consume(tok, "*"))
    return new_unary(ND_DEREF, unary(tok, locals));

  if (consume(tok, "&"))
    return new_unary(ND_ADDR, unary(tok, locals));

  return primary(tok, locals);
}

// primary    = "(" expr ")" | declspec ident | ident funcall? | num
static Node *primary(Token **tok, LVar **locals)
{

  if (consume(tok, "("))
  {
    Node *node = expr(tok, locals);
    expect(tok, ")");
    return node;
  }

  if ((*tok)->kind == TK_IDENT)
  {

    // Function call
    if (equal(&(*tok)->next, "("))
      return funcall(tok, locals);

    // Variable
    LVar *var = find_var(tok, locals);

    if (!var)
      error_tok(tok, "変数が未定義です\n");

    *tok = (*tok)->next;
    return new_var_node(var);
  }

  if ((*tok)->kind == TK_TYPE)
  {
    return var_init(tok, locals);
  }

  if ((*tok)->kind == TK_NUM)
  {
    Node *node = new_num(expect_number(tok));
    return node;
  }

  return new_node(ND_NONE);
}

// var_init   = declspec ident
static Node *var_init(Token **tok, LVar **locals)
{
  Type *type = declspec(tok);
  if (!type)
    return NULL;

  if ((*tok)->kind != TK_IDENT)
    error_tok(tok, "Here should be variable name.\n");

  LVar *var = find_var(tok, locals);

  if (var)
    error_tok(tok, "変数が再定義されています\n");

  var = calloc(1, sizeof(LVar));
  var->next = *locals;
  var->name = (*tok)->str;
  var->len = (*tok)->len;
  var->offset = (*locals)->offset + 8;
  var->ty = type;
  *locals = var;
  *tok = (*tok)->next;

  return new_var_node(var);
}

// funcall    = ident "(" (assign ("," assign)*)? ")"
static Node *funcall(Token **tok, LVar **locals)
{
  Token *start = *tok;
  *tok = (*tok)->next->next;

  Node head = {};
  Node *cur = &head;

  while (!equal(tok, ")"))
  {
    if (cur != &head)
      consume(tok, ",");
    cur = cur->next = assign(tok, locals);
  }

  expect(tok, ")");

  Node *node = new_node(ND_FUNCALL);
  node->funcname = mystrndup(start->str, start->len);
  node->args = head.next;
  return node;
}

Function *parse(Token **tok)
{
  return program(tok);
}
