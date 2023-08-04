#include "9cc.h"

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);
  
  Token *token = tokenize(argv[1]);

  TopLevelNode *tlnodes = calloc(10, sizeof(TopLevelNode));
  int func_num = parse(tlnodes, &token);

  codegen(tlnodes, func_num);
  
  return 0;
}