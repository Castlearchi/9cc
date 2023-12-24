#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node Node;

//
// tokenize.c
//

typedef enum
{
  TK_RESERVED, // Keywords or punctuators
  TK_IDENT,    // Identifier
  TK_NUM,      // Integer literals
  TK_STR,      // String literals
  TK_KEYWORD,  // Keywords
  TK_TYPE,     // Type
  TY_SIZEOF,   // sizeof
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
  CHAR,
  INT,
  PTR,
  ARRAY
} TypeKeyword;

typedef struct Type Type;
struct Type
{
  TypeKeyword tkey;
  int size;
  struct Type *ptr_to; // Use if tkey == PTR
};

// Variable or function
typedef struct Obj Obj;
struct Obj
{
  Obj *next;     // Next var or NULL
  char *name;    // Var name
  size_t len;    // Name length
  int offset;    // Offset from RBP
  Type *ty;      // Type
  bool is_local; // local or global/function/str

  // Global variable or function
  bool is_function;
  char *init_data;
  Obj *globals;

  Obj *params;
  Node **body;
  int stmt_count;
  Obj **locals;
  int stack_size;
  int regards_num;
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token **tok, char *fmt, ...);
bool consume(Token **tok, char *op);
bool equal(Token **tok, char *op);
bool equal_xnext(Token **tok, char *op, int x);
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
  ND_NEG,     // unary -
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
  ND_SIZEOF,  // sizeof
  ND_BLOCK,   // { ... }
  ND_FUNCALL, // function call
  ND_ADDR,    // &address
  ND_DEREF,   // *pointer
  ND_NONE     // None
} NodeKind;

// AST node type
struct Node
{
  NodeKind kind; // Node kind
  Node *next;    // Next node
  Type *ty;      // Type, e.g. int or pointer to int.

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

  Obj *var; // Use if kind == ND_VAR

  char *str; // Use if kind == ND_STR
};

Obj *parse(Token **tok);

//
// codegen.c
//
void codegen(Obj *prog);

//
// type.c
//
void add_type(Node *node);