#include "9cc.h"

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  Token *token = tokenize(argv[1]);
  Node *node = parse(&token);
  codegen(node);

  return 0;
}