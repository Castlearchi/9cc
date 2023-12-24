#include "9cc.h"

Obj *globals;

static Node *new_node(NodeKind kind);
static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
static Node *new_unary(NodeKind kind, Node *lhs);
static Node *new_num(int val);
static Obj *find_var(Token **tok, Obj **locals);
static int type2byte(Type *ty);

/*
  program    = ( declspec (func | var_init ";") )*
  func       = declarator ( "(" func_params ")" ) "{" stmt* "}"
  declarator = ident
  func_params= (param ("," param)*)?
  param      = declspec ident
  declspec   = "int" fill_ptr_to
  fill_ptr_to= ("*")*
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
  unary      = "sizeof" unary
              | ("+" | "-" | "*" | "&") unary
              | primary
  primary    = "(" expr ")"
              | declspec var_init
              | funcall
              | ident array_index?
              | str array_index?
              | num
  var_init   = ident ("[" num "]")?
  funcall    = ident "(" (assign ("," assign)*)? ")"
  array_index= ("[" expr "]")?
*/

static Obj *program(Token **tok);
static Obj *func(Type *type, Token **tok);
static Obj *declarator(Type *type, Token **tok);
static void func_params(Token **tok, Obj *fn);
static Obj *param(Token **tok, Obj *params);
static Type *declspec(Token **tok);
static Type *fill_ptr_to(Token **tok, Type *cur);
static Node *stmt(Token **tok, Obj **locals);
static Node *expr(Token **tok, Obj **locals);
static Node *assign(Token **tok, Obj **locals);
static Node *equality(Token **tok, Obj **locals);
static Node *relational(Token **tok, Obj **locals);
static Node *add(Token **tok, Obj **locals);
static Node *mul(Token **tok, Obj **locals);
static Node *unary(Token **tok, Obj **locals);
static Node *primary(Token **tok, Obj **locals);
static Node *funcall(Token **tok, Obj **locals);
static Node *var_init(Type *type, Token **tok, Obj **vars);
static Node *array_index(Token **tok, Obj **locals);

static Obj *new_gvar(char *name, Type *ty, Token **tok)
{
  Obj *var = calloc(1, sizeof(Obj));
  var->len = (*tok)->len;
  int token_type = (*tok)->kind;
  *tok = (*tok)->next;
  if (equal(tok, "[") && token_type != TK_STR)
  {
    *tok = (*tok)->next;
    int idx = expect_number(tok);
    int type_size = ty->size;
    Type *ptr_to = calloc(1, sizeof(Type));
    ptr_to->tkey = ty->tkey;
    ty->tkey = ARRAY;
    ty->size *= idx;
    ptr_to->size = type_size;
    ty->ptr_to = ptr_to;
    expect(tok, "]");
  }
  var->name = name;
  var->ty = ty;

  var->next = globals;
  var->is_local = false;
  var->is_function = false;

  globals = var;
  return var;
}

static char *new_unique_name(void)
{
  static int id = 0;
  char *buf = calloc(1, 20);
  sprintf(buf, ".L..%d", id++);
  return buf;
}

static Obj *new_anon_gvar(Type *ty, Token **tok)
{
  return new_gvar(new_unique_name(), ty, tok);
}

static Obj *new_string_literal(char *p, Type *ty, Token **tok)
{
  Obj *var = new_anon_gvar(ty, tok);
  var->init_data = p;
  return var;
}

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

static Node *new_var_node(Obj *var)
{
  Node *node = new_node(ND_VAR);
  node->var = var;
  node->ty = var->ty;
  return node;
}

static Node *new_add(Node *lhs, Node *rhs)
{
  add_type(lhs);
  add_type(rhs);

  if ((lhs->ty->tkey == INT || lhs->ty->tkey == CHAR) &&
      (rhs->ty->tkey == INT || rhs->ty->tkey == CHAR))
    return new_binary(ND_ADD, lhs, rhs);

  if (lhs->ty->tkey == INT)
  {
    Node *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }

  return new_binary(ND_ADD, lhs, new_binary(ND_MUL, rhs, new_num(type2byte(lhs->ty->ptr_to))));
}

// Like `+`, `-` is overloaded for the pointer type.
static Node *new_sub(Node *lhs, Node *rhs)
{
  add_type(lhs);
  add_type(rhs);

  if (lhs->ty->tkey == INT && rhs->ty->tkey == INT)
    return new_binary(ND_SUB, lhs, rhs);

  if (lhs->ty->tkey == INT)
  {
    Node *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }

  return new_binary(ND_SUB, lhs, new_binary(ND_MUL, rhs, new_num(type2byte(lhs->ty))));
}

// Search var name rbut not find return NULL.
Obj *find_var(Token **tok, Obj **locals)
{
  for (Obj *var = *locals; var; var = var->next) // Local var
    if (var->len == (*tok)->len && !memcmp((*tok)->str, var->name, var->len))
      return var;

  for (Obj *var = globals; var; var = var->next) // Global var
    if (var->len == (*tok)->len && !memcmp((*tok)->str, var->name, var->len))
      return var;

  return NULL;
}

static int type2byte(Type *ty)
{
  if (!ty)
    error("type2byte: ty is none\n");

  switch (ty->tkey)
  {
  case CHAR:
    return 1;

  case INT:
    return 4;

  case PTR:
    return 8;

  case ARRAY:
    return ty->size;

  default:
    error("定義されていない変数型です %d\n", ty->tkey);
  }
}

// program = ( declspec (func | var_init ";") )*
static Obj *program(Token **tok)
{
  globals = (Obj *)calloc(1, sizeof(Obj));
  globals = NULL;

  while (!at_eof(tok))
  {

    Type *type = declspec(tok);
    if (!type)
      error_tok(tok, "program: Here should be type. %d\n", (*tok)->kind);

    if (equal_xnext(tok, "(", 1)) // func
    {
      Obj *fn = func(type, tok);
      fn->next = globals;
      globals = fn;
    }
    else // global var
    {
      if ((*tok)->kind != TK_IDENT)
        error_tok(tok, "program: Here should be ident. %d\n", (*tok)->kind);
      char *name = (*tok)->str;
      new_gvar(name, type, tok);
      expect(tok, ";");
    }
  }

  return globals;
}

// func       = declarator ( "(" func_params ")" ) "{" stmt* "}"
static Obj *func(Type *type, Token **tok)
{
  Obj *fn = declarator(type, tok);
  fn->is_function = true;

  expect(tok, "(");

  func_params(tok, fn);

  expect(tok, ")");

  expect(tok, "{");

  Obj **locals = (Obj **)calloc(1, sizeof(Obj *));
  *locals = (Obj *)calloc(1, sizeof(Obj));
  *locals = fn->params;

  fn->body = calloc(1, sizeof(Node *));
  fn->stmt_count = 0;
  while (!consume(tok, "}"))
  {
    fn->stmt_count++;
    fn->body = realloc(fn->body, sizeof(Node *) * fn->stmt_count);
    fn->body[fn->stmt_count - 1] = stmt(tok, locals);
  }

  int stack_size = 0;
  for (Obj *var = *locals; var->next; var = var->next)
  {
    int size = type2byte(var->ty);
    stack_size += size;
    var->ty->size = size;
  }
  fn->locals = locals;
  fn->stack_size = stack_size;

  return fn;
}

// declarator = declspec ident
static Obj *declarator(Type *type, Token **tok)
{
  Obj *fn = (Obj *)calloc(1, sizeof(Obj));
  if (!expect_ident(tok))
    error_tok(tok, "Here should be Obj name. %d\n", (*tok)->kind);
  fn->ty = type;
  fn->name = (*tok)->str;
  *tok = (*tok)->next;
  return fn;
}

// func_params= (param ("," param)*)?
static void func_params(Token **tok, Obj *fn)
{
  Obj *params = (Obj *)calloc(1, sizeof(Obj));
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
static Obj *param(Token **tok, Obj *params)
{
  Type *type = declspec(tok);
  if ((*tok)->kind != TK_IDENT)
    error_tok(tok, "Here should be Obj argument name.\n");
  Obj *NoObj = find_var(tok, &params);
  if (NoObj)
    error_tok(tok, "Redeclaration of argument.\n");
  Obj *obj = calloc(1, sizeof(Obj));
  obj->next = params;
  obj->name = (*tok)->str;
  obj->len = (*tok)->len;
  obj->offset = params->offset + type2byte(type);
  obj->ty = type;
  obj->is_local = true;
  params = obj;
  *tok = (*tok)->next;
  return params;
}

// declspec   = ("int" | "char") fill_ptr_to
static Type *declspec(Token **tok)
{
  Type *cur = calloc(1, sizeof(Type));
  if ((*tok)->kind != TK_TYPE)
    error_tok(tok, "Here should be type.\n");

  if (consume(tok, "int"))
  {
    cur->tkey = INT;
    cur->size = 4;
    if (equal(tok, "*"))
      cur = fill_ptr_to(tok, cur);
  }

  else if (consume(tok, "char"))
  {
    cur->tkey = CHAR;
    cur->size = 1;
    if (equal(tok, "*"))
      cur = fill_ptr_to(tok, cur);
  }

  return cur;
}

// fill_ptr_to = ("*")*
static Type *fill_ptr_to(Token **tok, Type *cur)
{
  while (consume(tok, "*"))
  {
    Type *ptr = calloc(1, sizeof(Type));
    ptr->ptr_to = cur;
    ptr->tkey = PTR;
    ptr->size = 8;
    cur = ptr;
  }

  return cur;
}

// stmt       = expr ";"
//            | "{" stmt* "}"
//            | "return" expr ";"
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "for" (" expr? ";" expr? ";" expr? ")" stmt
static Node *stmt(Token **tok, Obj **locals)
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
static Node *expr(Token **tok, Obj **locals)
{
  return assign(tok, locals);
}

// assign = equality ("=" assign)?
static Node *assign(Token **tok, Obj **locals)
{
  Node *node = equality(tok, locals);
  if (consume(tok, "="))
    node = new_binary(ND_ASSIGN, node, assign(tok, locals));
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **tok, Obj **locals)
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
static Node *relational(Token **tok, Obj **locals)
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
static Node *add(Token **tok, Obj **locals)
{
  Node *node = mul(tok, locals);
  for (;;)
  {
    if (consume(tok, "+"))
    {
      node = new_add(node, mul(tok, locals));
    }

    else if (consume(tok, "-"))
    {
      node = new_sub(node, mul(tok, locals));
    }

    else
      return node;
  }
}
// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **tok, Obj **locals)
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

// unary      = "sizeof" unary
//             | ("+" | "-" | "*" | "&") unary
//             | primary
static Node *unary(Token **tok, Obj **locals)
{
  if (consume(tok, "sizeof"))
    return new_unary(ND_SIZEOF, unary(tok, locals));

  if (consume(tok, "+"))
    return unary(tok, locals);

  if (consume(tok, "-"))
    return new_unary(ND_NEG, unary(tok, locals));

  if (consume(tok, "*"))
    return new_unary(ND_DEREF, unary(tok, locals));

  if (consume(tok, "&"))
    return new_unary(ND_ADDR, unary(tok, locals));

  return primary(tok, locals);
}

// primary    = "(" expr ")"
//             | declspec var_init
//             | funcall
//             | ident array_index?
//             | str array_index?
//             | num
static Node *primary(Token **tok, Obj **locals)
{

  if (consume(tok, "("))
  {
    Node *node = expr(tok, locals);
    expect(tok, ")");
    return node;
  }

  if ((*tok)->kind == TK_TYPE)
  {
    Type *type = declspec(tok);
    if (!type)
      error_tok(tok, "Type is not defined.\n");
    return var_init(type, tok, locals);
  }

  if ((*tok)->kind == TK_IDENT && equal_xnext(tok, "(", 1))
  {
    return funcall(tok, locals);
  }

  if ((*tok)->kind == TK_IDENT)
  {
    // Variable
    Obj *var = find_var(tok, locals);
    if (!var)
      error_tok(tok, "変数が未定義です\n");

    *tok = (*tok)->next;

    if (equal(tok, "[")) // Array
    {
      Node *node_idx = array_index(tok, locals);
      Node *node_var = new_var_node(var);
      return new_unary(ND_DEREF, new_add(node_var, node_idx));
    }
    else // Normal var
    {
      return new_var_node(var);
    }
  }

  if ((*tok)->kind == TK_STR)
  {
    Type *ty = calloc(1, sizeof(Type));
    Type *ptr_to = calloc(1, sizeof(Type));
    char *str = (*tok)->str;

    ty->tkey = ARRAY;
    ty->size = (*tok)->len + 1;

    ptr_to->tkey = CHAR;
    ptr_to->size = 1;

    ty->ptr_to = ptr_to;

    Obj *var = new_string_literal(str, ty, tok);

    if (equal(tok, "[")) // return character
    {
      Node *node_idx = array_index(tok, locals);
      Node *node_var = new_var_node(var);
      return new_unary(ND_DEREF, new_add(node_var, node_idx));
    }
    else // return string
    {
      return new_var_node(var);
    }
  }

  if ((*tok)->kind == TK_NUM)
  {
    Node *node = new_num(expect_number(tok));
    return node;
  }

  return new_node(ND_NONE);
}

//   var_init   = ident ("[" num "]")?
static Node *var_init(Type *type, Token **tok, Obj **vars)
{
  if ((*tok)->kind != TK_IDENT)
    error_tok(tok, "Here should be variable name.\n");

  Obj *var = find_var(tok, vars);
  if (var)
    error_tok(tok, "変数が再定義されています\n");

  var = calloc(1, sizeof(Obj));
  var->name = (*tok)->str;
  var->len = (*tok)->len;
  var->next = *vars;
  var->is_local = true;
  *tok = (*tok)->next;

  if (consume(tok, "[")) // Array
  {
    int idx = expect_number(tok);
    expect(tok, "]");

    int type_size = type->size;
    var->offset = (*vars)->offset + type_size * idx;
    type->size *= idx;
    Type *ty = calloc(1, sizeof(Type));
    ty->tkey = type->tkey;
    ty->size = type_size;
    type->tkey = ARRAY;
    type->ptr_to = ty;
    var->ty = type;
    *vars = var;
    return new_unary(ND_DEREF, new_var_node(var));
  }
  else // Normal var
  {
    var->offset = (*vars)->offset + type2byte(type);
    var->ty = type;
    *vars = var;
    return new_var_node(var);
  }
}

// funcall    = ident "(" (assign ("," assign)*)? ")"
static Node *funcall(Token **tok, Obj **locals)
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

// array_index = ("[" expr "]")?
static Node *array_index(Token **tok, Obj **locals)
{
  *tok = (*tok)->next; // skip
  Node *node_idx = expr(tok, locals);
  expect(tok, "]");
  return node_idx;
}

Obj *parse(Token **tok)
{
  return program(tok);
}
