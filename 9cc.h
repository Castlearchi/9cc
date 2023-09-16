#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenize.c
//

typedef enum
{
  TK_RESERVED, // Keywords or punctuators
  TK_IDENT,    // Identifier
  TK_NUM,      // Integer literals
  TK_RETURN,   // "return" keyword
  TK_IF,       // "if" keyword
  TK_ELSE,     // "else" keyword
  TK_WHILE,    // "while" keyword
  TK_FOR,      // "for" keyword
  TK_EOF       // End-of-file markers
} TokenKind;

// Token type
typedef struct Token Token;
struct Token
{
  TokenKind kind; // Token kind
  Token *next;    // Next token
  int val;        // If kind is TK_NUM, its value
  char *str;      // Token string
  int len;        // Token length
};

typedef struct LVar LVar;

// Local Variable
struct LVar
{
  LVar *next; // Next var or NULL
  char *name; // Var name
  int len;    // Name length
  int offset; // Offset from RBP
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(Token **tok, char *op);
bool consume_return(Token **tok);
bool consume_if(Token **tok);
bool consume_else(Token **tok);
bool consume_while(Token **tok);
bool consume_for(Token **tok);
bool expect_ident(Token **tok);
void expect(Token **tok, char *op);
int expect_number(Token **tok);
bool at_eof(Token **tok);
Token *tokenize(char *p);

//
// parse.c
//

typedef enum
{
  ND_ADD,    // +
  ND_SUB,    // -
  ND_MUL,    // *
  ND_DIV,    // /
  ND_EQ,     // ==
  ND_NE,     // !=
  ND_LT,     // <
  ND_LE,     // <=
  ND_NUM,    // Integer
  ND_ASSIGN, // =
  ND_LVAR,   // Local Variable
  ND_RETURN, // return
  ND_IF,     // if
  ND_IFELSE, // if ... else ...
  ND_ELSE,   // else
  ND_WHILE,  // while
  ND_FOR,    // for
  ND_BLOCK,  // { ... }
  ND_FUNC,   // func
  ND_ADDR,   // *pointer
  ND_DEREF,  // &address
  ND_NONE    // nothing
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node
{
  NodeKind kind; // Node kind

  Node *lhs; // Left-hand side
  Node *rhs; // Right-hand side

  bool *eof; // EOF of Node.

  Node *cond; // Conditional expressions
  Node *then; // Run Statement by conditional expressions
  Node *els;  // else statement
  Node *init; // For initialization
  Node *inc;  // For increment

  int val;    // Used if kind == ND_NUM
  int offset; // Used if kind == ND_NUM

  Node **block;      // Block
  size_t block_size; // Block size
  int block_count;   // Block count

  char *lvar_name;   // Variable name or Function name
  int *parameter;    // Fucntion parameter value
  int parameter_num; // Number of Fucntion parameters
};

typedef struct
{
  Node **stmts;   // Statement list
  int stmt_count; // Number of statements

  char *funcname;    // Func name
  int parameter_num; // Number of Fucntion parameters
} TopLevelNode;

int parse(TopLevelNode *tlnodes, Token **tok);

//
// codegen.c
//
void codegen(TopLevelNode *tlnodes, int func_num);