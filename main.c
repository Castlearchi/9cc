#include "9cc.h"

int main(int argc, char **argv)
{
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  char *filename = argv[1];
  char *input_content = read_file(filename);

  Token *tok = tokenize(filename, input_content);

  Obj *prog = parse(&tok);

  codegen(prog);

  return 0;
}