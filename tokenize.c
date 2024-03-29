#include "9cc.h"

// Filename
char *filename;
// Input
static char *user_input;

void error(char *fmt, ...);
void error_at(char *loc, char *msg);
void error_tok(Token **tok, char *fmt, ...);
bool consume(Token **tok, char *op);
void expect(Token **tok, char *op);
int expect_number(Token **tok);
bool is_al(char character);
bool is_alnum(char character);
bool at_eof(Token **tok);
static Token *new_token(TokenKind kind, Token *cur, char *loc, char *str, int len);
static bool startswith(char *p, char *q);

// Reports an error and exit.
void error(char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports an error location and exit.
static void verror_at(char *loc, char *fmt, va_list ap)
{
  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// エラーの起きた場所を報告するための関数
// 下のようなフォーマットでエラーメッセージを表示する
//
// foo.c:10: x = y + + 5;
//                   ^ 式ではありません
void error_at(char *loc, char *msg)
{
  // locが含まれている行の開始地点と終了地点を取得
  char *line = loc;
  while (user_input < line && line[-1] != '\n')
    line--;

  char *end = loc;
  while (*end != '\n')
    end++;

  // 見つかった行が全体の何行目なのかを調べる
  int line_num = 1;
  for (char *p = user_input; p < line; p++)
    if (*p == '\n')
      line_num++;

  // 見つかった行を、ファイル名と行番号と一緒に表示
  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  // エラー箇所を"^"で指し示して、エラーメッセージを表示
  int pos = loc - line + indent;
  fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
  fprintf(stderr, "^ %s\n", msg);
  exit(1);
}

// Reports an error location and exit.
void error_tok(Token **tok, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  verror_at((*tok)->loc, fmt, ap);
}

// Consumes the current token if it matches `op`.
bool consume(Token **tok, char *op)
{
  if (equal(tok, op))
  {
    *tok = (*tok)->next;
    return true;
  }
  return false;
}

// Ensure the current token if it matches `op`.
bool equal(Token **tok, char *op)
{
  return memcmp((*tok)->str, op, (*tok)->len) == 0 && op[(*tok)->len] == '\0';
}

// Ensure that the x-next token is `op`.
// ex. equal_xnext(tok, "==", 2)
// => equal(&(tok->next->next), "==")
bool equal_xnext(Token **tok, char *op, int x)
{
  Token *cur = *tok;
  for (int i = 0; i < x; i++)
  {
    if (cur->next == NULL)
      error_tok(tok, "equal_xnext: %dnext token is NULL", i);
    cur = cur->next;
  }
  return equal(&cur, op);
}

bool expect_ident(Token **tok)
{
  if ((*tok)->kind != TK_IDENT)
    return false;
  else
    return true;
}

// Ensure that the current token is `op`.
void expect(Token **tok, char *op)
{
  if ((*tok)->kind != TK_RESERVED ||
      (*tok)->len != strlen(op) ||
      memcmp((*tok)->str, op, (*tok)->len))
  {
    char *msg = calloc(1, 50);
    sprintf(msg, "expected '%s' but got '%s'", op, (*tok)->str);
    error_at((*tok)->loc, msg);
  }
  *tok = (*tok)->next;
}

// Ensure that the current token is TK_NUM.
int expect_number(Token **tok)
{
  if ((*tok)->kind != TK_NUM)
    error_at((*tok)->loc, "expected a number");
  int val = (*tok)->val;
  *tok = (*tok)->next;
  return val;
}

bool is_al(char character)
{
  return ('a' <= character && character <= 'z') ||
         ('A' <= character && character <= 'Z') ||
         (character == '_');
}

bool is_alnum(char character)
{
  return ('a' <= character && character <= 'z') ||
         ('A' <= character && character <= 'Z') ||
         ('0' <= character && character <= '9') ||
         (character == '_');
}

bool at_eof(Token **tok)
{
  return (*tok)->kind == TK_EOF;
}

char *mystrndup(const char *s, size_t n)
{
  char *t;
  size_t len = strlen(s);

  if (len > n)
  {
    len = n;
  }

  if ((t = malloc(sizeof(char) * (len + 1))) == NULL)
  {
    fprintf(stderr, "Error: cannot allocate memory %zu bytes\n",
            sizeof(char) * (len + 1));
    exit(2);
  }

  strncpy(t, s, len);
  t[len] = '\0';

  return t;
}

// Create a new token and add it as the next token of `cur`.
static Token *new_token(TokenKind kind, Token *cur, char *loc, char *str, int len)
{
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = loc;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

static bool startswith(char *p, char *q)
{
  return memcmp(p, q, strlen(q)) == 0;
}

// Tokenize `user_input` and returns new tokens.
Token *tokenize(char *filename, char *p)
{
  filename = filename;
  user_input = p;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p)
  {
    // Skip whitespace characters.
    if (isspace(*p))
    {
      p++;
      continue;
    }

    // Punctuator
    if (startswith(p, "==") || startswith(p, "!=") ||
        startswith(p, "<=") || startswith(p, ">="))
    {
      char *str = mystrndup(p, 2);
      cur = new_token(TK_RESERVED, cur, p, str, 2);
      p += 2;
      continue;
    }
    if (strchr("+-*/()<>;,={}&[]", *p))
    {
      char *str = mystrndup(p, 1);
      cur = new_token(TK_RESERVED, cur, p++, str, 1);
      continue;
    }

    // String
    if (startswith(p, "\""))
    {
      p++;
      int str_len = 0;
      while (!startswith(p, "\""))
      {
        if (str_len > 128)
          error_at(p, "String too long");
        str_len++;
        p++;
      }

      char *str = mystrndup(p - str_len, str_len);
      cur = new_token(TK_STR, cur, p, str, str_len);
      p++;
      continue;
    }

    // Integer literal
    if (isdigit(*p))
    {
      cur = new_token(TK_NUM, cur, p, "", 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6]))
    {
      cur = new_token(TK_KEYWORD, cur, p, "return ", 6);
      p += 6;
      continue;
    }
    if (strncmp(p, "else", 4) == 0)
    {
      cur = new_token(TK_KEYWORD, cur, p, "else", 4);
      p += 4;
      continue;
    }
    if (strncmp(p, "for", 3) == 0)
    {
      cur = new_token(TK_KEYWORD, cur, p, "for", 3);
      p += 3;
      continue;
    }
    if (strncmp(p, "while", 5) == 0)
    {
      cur = new_token(TK_KEYWORD, cur, p, "while", 5);
      p += 5;
      continue;
    }

    if (strncmp(p, "if", 2) == 0)
    {
      cur = new_token(TK_KEYWORD, cur, p, "if", 2);
      p += 2;
      continue;
    }

    if (strncmp(p, "int", 3) == 0)
    {
      cur = new_token(TK_TYPE, cur, p, "int", 3);
      p += 3;
      continue;
    }

    if (strncmp(p, "char", 4) == 0)
    {
      cur = new_token(TK_TYPE, cur, p, "char", 4);
      p += 4;
      continue;
    }

    if (strncmp(p, "sizeof", 6) == 0)
    {
      cur = new_token(TY_SIZEOF, cur, p, "sizeof", 6);
      p += 6;
      continue;
    }

    if (is_al(*p))
    {
      char *q = p;
      char *var_str = calloc(128, sizeof(char));
      int var_length = 0;
      while (is_alnum(*p))
      {
        var_str[var_length] = *p;
        var_length++;
        p++;
      }
      var_str[var_length] = '\0';
      cur = new_token(TK_IDENT, cur, q, var_str, var_length);
      continue;
    }

    error_at(p, "invalid token");
  }
  new_token(TK_EOF, cur, p, "", 0);
  return head.next;
}
