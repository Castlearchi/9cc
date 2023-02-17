#include "9cc.h"

static Node *new_node(NodeKind kind);
static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
static Node *new_num(int val);
static Node *expr(Token **tok);
static Node *equality(Token **tok);
static Node *relational(Token **tok);
static Node *add(Token **tok);
static Node *mul(Token **tok);
static Node *unary(Token **tok);
static Node *primary(Token **tok);


static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
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

// expr = equality
static Node *expr(Token **tok) {
  return equality(tok);
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **tok) {
  Node *node = relational(tok);

  for(;;){
    if(consume(tok, "=="))
      node = new_binary(ND_EQ, node, relational(tok));
    else if(consume(tok, "!="))
      node = new_binary(ND_NE, node, relational(tok));
    else
      return node;
  }
  
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **tok) {
  Node *node = add(tok);
  for(;;){
    if(consume(tok, "<"))
      node = new_binary(ND_LT, node, add(tok));
    else if(consume(tok, "<="))
      node = new_binary(ND_LE, node, add(tok));
    else if(consume(tok, ">"))
      node = new_binary(ND_LT, add(tok), node);
    else if(consume(tok, ">="))
      node = new_binary(ND_LE, add(tok), node);
    else 
      return node;
  }
  
}
// add = mul ("+" mul | "-" mul)*
static Node *add(Token **tok) {
  Node *node = mul(tok);
  for(;;){
    if(consume(tok, "+"))
      node = new_binary(ND_ADD, node, mul(tok));
    else if(consume(tok, "-"))
      node = new_binary(ND_SUB, node, mul(tok));
    else 
      return node;
  }
  
  
}
// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **tok) {
  Node *node = unary(tok);

  for (;;) {
    if (consume(tok, "*"))
      node = new_binary(ND_MUL, node, unary(tok));
    else if (consume(tok, "/"))
      node = new_binary(ND_DIV, node, unary(tok));
    else
      return node;
  }
}

// unary = ("+" | "-")? primary
static Node *unary(Token **tok) {
  if(consume(tok, "+"))
    return unary(tok);

  if(consume(tok, "-"))
    return new_binary(ND_SUB, new_num(0), unary(tok));
  return primary(tok);
}

// primary = "(" expr ")" | num
static Node *primary(Token **tok) {
  if (consume(tok, "(")) {
    Node *node = expr(tok);
    expect(tok, ")");
    return node;
  }
  Node *a = new_num(expect_number(tok));
  return a;
}


Node *parse(Token **tok) {
  Node *node = expr(tok);
  return node;
}
