#include "9cc.h"

static Node *new_node(NodeKind kind);
static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
static Node *new_num(int val);
static LVar *find_lvar(Token **tok, LVar **locals);
/*
  program    = stmt*
  stmt       = expr ";" 
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
  unary      = ("+" | "-")? primary
  primary    = num | ident | "(" expr ")"
*/

static int program(Node **code, Token **tok);
static Node *stmt(Token **tok, LVar **locals);
static Node *expr(Token **tok, LVar **locals);
static Node *assign(Token **tok, LVar **locals);
static Node *equality(Token **tok, LVar **locals);
static Node *relational(Token **tok, LVar **locals);
static Node *add(Token **tok, LVar **locals);
static Node *mul(Token **tok, LVar **locals);
static Node *unary(Token **tok, LVar **locals);
static Node *primary(Token **tok, LVar **locals);


static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->eof = false;
  return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

// Search var name rbut not find return NULL.
LVar *find_lvar(Token **tok, LVar **locals) {
  for (LVar *var = *locals; var; var = var->next) 
    if (var->len == (*tok)->len && !memcmp((*tok)->str, var->name, var->len))
      return var;
  return NULL;
}

// program = stmt*
static int program(Node **code, Token **tok) {
  int i = 0;
  LVar **locals =(LVar**)calloc(1, sizeof(LVar*));
  *locals = (LVar*)calloc(1, sizeof(LVar));
  (*locals)->offset = 0;
  (*locals)->next = NULL;

  while (!at_eof(tok)) {
    code[i] = malloc(sizeof(Node));
    code[i] = stmt(tok, locals);
    if (code[i]->eof) {
      return i+1;
    }
    i++;
  }
  code[i-1]->eof = true;
  return i;
}

// stmt       = expr ";" 
//            | "return" expr ";"
//            | "if" "(" expr ")" stmt ("else" stmt)? 
//            | "while" "(" expr ")" stmt
//            | "for" (" expr? ";" expr? ";" expr? ")" stmt
static Node *stmt(Token **tok, LVar **locals) {
  Node *node;
  if (at_eof(tok)) {
    node = calloc(1, sizeof(Node));
    node->eof = true;
    return node;
  }
    

  if (consume_return(tok)) {
    node = new_node(ND_RETURN);
    node->lhs = expr(tok, locals);
    expect(tok, ";");
  } else if (consume_if(tok)) {
    node = new_node(ND_IF);
    expect(tok, "(");
    node->cond = expr(tok, locals);
    expect(tok, ")");
    node->then = stmt(tok, locals);
    if (consume_else(tok)) {
      node->kind = ND_IFELSE;
      node->els = stmt(tok, locals);
    }
  } else if (consume_while(tok)) {
    node = new_node(ND_WHILE);
    expect(tok, "(");
    node->cond = expr(tok, locals);
    expect(tok, ")");
    node->then = stmt(tok, locals);
  } else if (consume_for(tok)) {
    node = new_node(ND_FOR);
    expect(tok, "(");
    if (!consume(tok, ";")) {
      node->init = expr(tok, locals);
      expect(tok, ";");
    }
    if (!consume(tok, ";")) {
      node->cond = expr(tok, locals);
      expect(tok, ";");
    }
    if (!consume(tok, ")")) {
      node->inc = expr(tok, locals);
      expect(tok, ")");
    }
    node->then = stmt(tok, locals);
  } else {
    node = expr(tok, locals);
    expect(tok, ";");
  }
  return node;
}

// expr = assign
static Node *expr(Token **tok, LVar **locals) {
  return assign(tok, locals);
}

//assign = equality ("=" assign)?
static Node *assign(Token **tok, LVar **locals) {
  Node *node = equality(tok, locals);
  if(consume(tok, "="))
    node = new_binary(ND_ASSIGN, node, assign(tok, locals));
    
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **tok, LVar **locals) {
  Node *node = relational(tok, locals);

  for(;;){
    if(consume(tok, "=="))
      node = new_binary(ND_EQ, node, relational(tok, locals));
    else if(consume(tok, "!="))
      node = new_binary(ND_NE, node, relational(tok, locals));
    else
      return node;
  }
  
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **tok, LVar **locals) {
  Node *node = add(tok, locals);
  for(;;){
    if(consume(tok, "<"))
      node = new_binary(ND_LT, node, add(tok, locals));
    else if(consume(tok, "<="))
      node = new_binary(ND_LE, node, add(tok, locals));
    else if(consume(tok, ">"))
      node = new_binary(ND_LT, add(tok, locals), node);
    else if(consume(tok, ">="))
      node = new_binary(ND_LE, add(tok, locals), node);
    else 
      return node;
  }
  
}
// add = mul ("+" mul | "-" mul)*
static Node *add(Token **tok, LVar **locals) {
  Node *node = mul(tok, locals);
  for(;;){
    if(consume(tok, "+"))
      node = new_binary(ND_ADD, node, mul(tok, locals));
    else if(consume(tok, "-"))
      node = new_binary(ND_SUB, node, mul(tok, locals));
    else 
      return node;
  }
  
  
}
// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **tok, LVar **locals) {
  Node *node = unary(tok, locals);

  for (;;) {
    if (consume(tok, "*"))
      node = new_binary(ND_MUL, node, unary(tok, locals));
    else if (consume(tok, "/"))
      node = new_binary(ND_DIV, node, unary(tok, locals));
    else
      return node;
  }
}

// unary = ("+" | "-")? primary
static Node *unary(Token **tok, LVar **locals) {
  if(consume(tok, "+"))
    return unary(tok, locals);

  if(consume(tok, "-"))
    return new_binary(ND_SUB, new_num(0), unary(tok, locals));
  return primary(tok, locals);
}

// primary = num | ident | "(" expr ")"
static Node *primary(Token **tok, LVar **locals) {
  if (consume(tok, "(")) {
    Node *node = expr(tok, locals);
    expect(tok, ")");
    return node;
  }
  
  if(expect_ident(tok)) {
    Node *node = new_node(ND_LVAR);
    LVar *lvar = find_lvar(tok, locals);
    if (lvar) {
      node->offset = lvar->offset;
    } else {
      lvar = calloc(1, sizeof(LVar));
      lvar->next = *locals;
      lvar->name = (*tok)->str;
      lvar->len = (*tok)->len;
      lvar->offset = (*locals)->offset + 8;
      node->offset = lvar->offset;
      *locals = lvar;
    }
    (*tok) = (*tok)->next;
    return node;
  }

  return new_num(expect_number(tok));;
}


int parse(Node **code, Token **tok) {
  int code_num = program(code, tok);
  return code_num;
}
