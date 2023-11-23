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
  TK_KEYWORD,  // Keywords
  TK_TYPE,     // Type
  TK_EOF       // End-of-file markers
} TokenKind;

// Token type
typedef struct Token Token;
struct Token
{
  TokenKind kind; // Token kind
  Token *next;    // Next token
  int val;        // If kind is TK_NUM, its value
  char *loc;      // Token location
  char *str;      // Token string
  size_t len;     // Token length
};

typedef enum
{
  INT,
  PTR
} ty;

typedef struct Type Type;
struct Type
{
  ty ty;
  struct Type *ptr_to; // Use if ty == PTR
};

// Local Variable
typedef struct LVar LVar;
struct LVar
{
  LVar *next; // Next var or NULL
  char *name; // Var name
  size_t len; // Name length
  int offset; // Offset from RBP
  Type *ty;   // Type
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token **tok, char *fmt, ...);
bool consume(Token **tok, char *op);
bool equal(Token **tok, char *op);
bool expect_ident(Token **tok);
void expect(Token **tok, char *op);
int expect_number(Token **tok);
bool at_eof(Token **tok);
Token *tokenize(char *p);
char *mystrndup(const char *s, size_t n);

//
// parse.c
//

typedef enum
{
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
  ND_VAR,     // Local Variable
  ND_RETURN,  // return
  ND_IF,      // if
  ND_IFELSE,  // if ... else ...
  ND_ELSE,    // else
  ND_WHILE,   // while
  ND_FOR,     // for
  ND_BLOCK,   // { ... }
  ND_FUNCALL, // function call
  ND_ADDR,    // &address
  ND_DEREF,   // *pointer
  ND_NONE     // None
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node
{
  NodeKind kind; // Node kind
  Node *next;    // Next node

  Node *lhs; // Left-hand side
  Node *rhs; // Right-hand side

  bool eof; // EOF of Node.

  Node *cond; // Conditional expressions
  Node *then; // Run Statement by conditional expressions
  Node *els;  // else statement
  Node *init; // For initialization
  Node *inc;  // For increment

  int val; // Used if kind == ND_NUM

  Node **block;      // Block
  size_t block_size; // Block size
  int block_count;   // Block count

  char *lvar_name; // Variable name

  char *funcname;    // Function name
  Node *args;        // Fucntion parameter value
  int parameter_num; // Number of Fucntion parameters

  LVar *var; // Used if kind == ND_VAR
};

// Function
typedef struct Function Function;
struct Function
{
  Function *next;
  char *name;
  LVar *params;
  Type *ty; // return var type

  Node **body;
  int stmt_count;
  LVar **locals;
  int stack_size;
  int regards_num;
};

Function *parse(Token **tok);

//
// codegen.c
//
void codegen(Function *prog);