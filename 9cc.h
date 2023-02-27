#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenize.c
//

typedef enum {
  TK_RESERVED, // Keywords or punctuators
  TK_IDENT,    // Identifier
  TK_NUM,      // Integer literals
  TK_EOF,      // End-of-file markers
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind; // Token kind
  Token *next;    // Next token
  int val;        // If kind is TK_NUM, its value
  char *str;      // Token string
  int len;        // Token length
};

typedef struct LVar LVar;

// Local Variable
struct LVar {
  LVar *next; // Next var or NULL
  char *name; // Var name
  int len;    // Name length
  int offset; // Offset from RBP
};


void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(Token **tok, char *op);
int consume_ident();
void expect(Token **tok, char *op);
int expect_number(Token **tok);
bool at_eof(Token **tok);
Token *tokenize(char *p);



//
// parse.c
//

typedef enum {
  ND_ADD,     // +
  ND_SUB,     // -
  ND_MUL,     // *
  ND_DIV,     // /
  ND_EQ,      // ==
  ND_NE,      // !=
  ND_LT,      // <
  ND_LE,      // <=
  ND_NUM,     // Integer
  ND_ASSIGN,  // =
  ND_LVAR     // Local Variable
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind; // Node kind
  Node *lhs;     // Left-hand side
  Node *rhs;     // Right-hand side
  int val;       // Used if kind == ND_NUM
  int offset;    // Used if kind == ND_NUM
};

int parse(Node **code, Token **tok);


//
// codegen.c
//
void codegen(Node **node, int code_num);