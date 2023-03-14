#include "9cc.h"

// Input
static char *user_input;

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(Token **tok, char *op);
bool consume_return(Token **tok);
void expect(Token **tok, char *op);
int expect_number(Token **tok);
bool is_al(char character);
int is_alnum(char character);
bool at_eof(Token **tok);
static Token *new_token(TokenKind kind, Token *cur, char *str, int len);
static bool startswith(char *p, char *q);

// Reports an error and exit.
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports an error location and exit.
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Consumes the current token if it matches `op`.
bool consume(Token **tok, char *op) {
  if ((*tok)->kind != TK_RESERVED ||
      (*tok)->len != strlen(op) ||
      memcmp((*tok)->str, op, (*tok)->len))
      return false;
  (*tok) = (*tok)->next;
  return true;
}

bool consume_return(Token **tok) {
  if((*tok)->kind != TK_RETURN) 
    return false;
  (*tok) = (*tok)->next;
  return true;
}

bool consume_if(Token **tok) {
  if((*tok)->kind != TK_IF) 
    return false;
  (*tok) = (*tok)->next;
  return true;
}

bool consume_else (Token **tok) {
  if((*tok)->kind != TK_ELSE) 
    return false;
  (*tok) = (*tok)->next;
  return true;
}

bool consume_while(Token **tok) {
  if((*tok)->kind != TK_WHILE) 
    return false;
  (*tok) = (*tok)->next;
  return true;
}

bool consume_for(Token **tok) {
  if((*tok)->kind != TK_FOR) 
    return false;
  (*tok) = (*tok)->next;
  return true;
}

bool expect_ident(Token **tok) {
  if((*tok)->kind != TK_IDENT)
    return false;
  else
    return true;
}

// Ensure that the current token is `op`.
void expect(Token **tok, char *op) {
  if ((*tok)->kind != TK_RESERVED ||
      (*tok)->len != strlen(op) ||
      memcmp((*tok)->str, op, (*tok)->len))
    error_at((*tok)->str, "expected '%s' but got '%s'", op, (*tok)->str);
  
  (*tok) = (*tok)->next;
}

// Ensure that the current token is TK_NUM.
int expect_number(Token **tok) {
  if ((*tok)->kind != TK_NUM)
    error_at((*tok)->str, "expected a number");
  int val = (*tok)->val;
  (*tok) = (*tok)->next;
  return val;
}

bool is_al(char character) {
  return ('a' <= character && character <= 'z') ||
         ('A' <= character && character <= 'Z') ||
         (character == '_');
}

int is_alnum(char character) {
  return ('a' <= character && character <= 'z') ||
         ('A' <= character && character <= 'Z') ||
         ('0' <= character && character <= '9') ||
         (character == '_');
}

bool at_eof(Token **tok) {
  return (*tok)->kind == TK_EOF;
}

// Create a new token and add it as the next token of `cur`.
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

static bool startswith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

// Tokenize `user_input` and returns new tokens.
Token *tokenize(char *p) {
  user_input = p;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // Skip whitespace characters.
    if (isspace(*p)) {
      p++;
      continue;
    }

    // Punctuator
    if(startswith(p, "==") || startswith(p, "!=") ||
       startswith(p, "<=") || startswith(p, ">=") ){
      cur = new_token(TK_RESERVED, cur, p, 2);
      p+=2;
      continue; 
    }
    if (strchr("+-*/()<>;={}", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    // Integer literal
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
      cur = new_token(TK_RETURN, cur, "return ", 6);
      p += 6;
      continue;
    }
    if (strncmp(p, "else", 4) == 0) {
      cur = new_token(TK_ELSE, cur, "else", 4);
      p += 4;
      continue;
    }
    if (strncmp(p, "for", 3) == 0) {
      cur = new_token(TK_FOR, cur, "for", 3);
      p += 3;
      continue;
    }
    if (strncmp(p, "while", 5) == 0) {
      cur = new_token(TK_WHILE, cur, "while", 5);
      p += 5;
      continue;
    }

    if (strncmp(p, "if", 2) == 0) {
      cur = new_token(TK_IF, cur, "if", 2);
      p += 2;
      continue;
    }

    if (is_al(*p)) {
      char *var_str = calloc(128, sizeof(char));
      int var_length = 0;
      while (is_alnum(*p)) {
        var_str[var_length] = *p;
        var_length++;
        p++;
      }
      var_str[var_length]='\0';
      cur = new_token(TK_IDENT, cur, var_str, var_length);
      continue;
    }

    error_at(p, "invalid token");
  }
  new_token(TK_EOF, cur, p, 0);
  return head.next;
}
