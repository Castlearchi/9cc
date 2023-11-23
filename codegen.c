#include "9cc.h"

static void gen(Node *node);
static void gen_addr(Node *node);
static void gen_funcall(Node *node);
static void gen_definefunc();

static char *regargs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static Function *current_fn;

static void gen_addr(Node *node)
{
  switch (node->kind)
  {
  case ND_VAR:
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->var->offset);
    // printf("  push rax\n");
    return;
  case ND_DEREF:
    gen(node->lhs);
    return;
  default:
    error("Unexpected node kind: %d", node->kind);
  }
}

static void gen_funcall(Node *node)
{
  int nargs = 0;
  for (Node *arg = node->args; arg; arg = arg->next)
  {
    gen(arg);
    printf("  push rax\n");
    nargs++;
  }

  for (int i = nargs - 1; i >= 0; i--)
    printf("  pop %s\n", regargs[i]);

  printf("  mov rax, 0\n");
  printf("  call %s\n", node->funcname);
  return;
}

static void gen(Node *node)
{
  static unsigned int Lnum = 0; // Lnum is serial number for control statement.
  // printf("debug %d\n", node->kind);
  switch (node->kind)
  {
  case ND_NONE:
    return;
  case ND_BLOCK:
    for (int i = 0; i < (node->block_count); i++)
    {
      gen(node->block[i]);
    }
    return;
  case ND_IF:
    gen(node->cond);
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", Lnum);
    gen(node->then);
    printf(".Lend%d:\n", Lnum++);
    return;
  case ND_IFELSE:
    gen(node->cond);
    printf("  cmp rax, 0\n");
    printf("  je  .Lelse%d\n", Lnum);
    gen(node->then);
    printf("  jmp .Lend%d\n", Lnum);
    printf(".Lelse%d:\n", Lnum);
    gen(node->els);
    printf(".Lend%d:\n", Lnum++);
    return;
  case ND_FOR:
    if (node->init)
      gen(node->init);
    printf(".Lbegin%d:\n", Lnum);

    if (node->cond)
    {
      gen(node->cond);
      printf("  cmp rax, 0\n");
      printf("  je  .Lend%d\n", Lnum);
    }

    gen(node->then);

    if (node->inc)
      gen(node->inc);
    printf("  jmp .Lbegin%d\n", Lnum);
    printf(".Lend%d:\n", Lnum);
    return;
  case ND_WHILE:
    printf(".Lbegin%d:\n", Lnum);
    gen(node->cond);
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", Lnum);
    gen(node->then);
    printf("  jmp .Lbegin%d\n", Lnum);
    printf(".Lend%d:\n", Lnum++);
    return;
  case ND_RETURN:
    gen(node->lhs);
    printf("  jmp .L.return.%s\n", current_fn->name);
    return;
  case ND_NUM:
    printf("  mov rax, %d\n", node->val);
    return;
  case ND_VAR:
    gen_addr(node);
    printf("  mov rax, [rax]\n");
    return;
  case ND_FUNCALL:
    gen_funcall(node);
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs);
    printf("  push rax\n");
    gen(node->rhs);
    printf("  pop rdi\n");
    printf("  mov [rdi], rax\n");
    return;
  case ND_ADDR:
    gen_addr(node->lhs);
    return;
  case ND_DEREF:
    gen(node->lhs);
    printf("  mov rax, [rax]\n");
    return;
  default:
  }

  gen(node->lhs);
  printf("  push rax\n");
  gen(node->rhs);
  printf("  push rax\n");
  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind)
  {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  default:
  }
}

static void gen_definefunc()
{
  // Allocate memory for 26 variables.
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", current_fn->stack_size);

  // Stack regargs into rbp
  for (LVar *param = current_fn->params; param->next; param = param->next)
  {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", param->offset);
    printf("  push rax\n");
  }
  for (int i = 0; i < current_fn->regards_num; i++)
  {
    printf("  pop rax\n");
    printf("  mov [rax], %s\n", regargs[i]);
  }

  // Traverse the AST to emit assembly.
  int code_num = current_fn->stmt_count;
  for (int i = 0; i < code_num; i++)
  {
    gen(current_fn->body[i]);
    // Since there should be one value left in the stack
    // as a result of evaluating the expression,
    // make sure to pop it to avoid overflowing the stack.
    // printf("  pop rax\n");
  }

  // The result of the last expression is stored in RAX,
  // so it becomes the return value.
  printf(".L.return.%s:\n", current_fn->name);
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}
void codegen(Function *prog)
{
  // Print out the first half of assembly.
  printf(".intel_syntax noprefix\n");
  for (Function *fn = prog; fn; fn = fn->next)
  {
    current_fn = fn;
    printf(".global %s\n", fn->name);
    printf("%s:\n", fn->name);
    gen_definefunc();
  }
}