#include "9cc.h"

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  Token *token = tokenize(argv[1]);

  Node *code[100];
  int code_num = parse(code, &token);
  codegen(code, code_num);

  return 0;
}