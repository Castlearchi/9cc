#include "9cc.h"

static void gen(Node *node);
static void gen_lval(Node *node);
static void gen_func(Node *node);

static void gen_lval(Node *node) {
  if (node->kind != ND_LVAR)
    error("代入の左辺値が変数ではありません");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

static void gen_func(Node *node) {
  printf("  lea rax, [rip + foo]\n");  // Add the relative address of the 'foo'
  printf("  call rax\n");              // Call the function whose address is stored in the rax register.
}

static void gen(Node *node) {
  static unsigned int Lnum = 0;        // Lnum is serial number for control statement.

  switch (node->kind) {
  case ND_BLOCK:
    for (int i = 0; i < (node->block_count); i++) {
      gen(node->block[i]);
      printf("  pop rax\n");
    }
    return;
  case ND_IF:
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", Lnum);
    gen(node->then);
    printf(".Lend%d:\n", Lnum++);
    return;
  case ND_IFELSE:
    gen(node->cond);
    printf("  pop rax\n");
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
      gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", Lnum);
    if (node->inc)
      gen(node->inc);
    gen(node->then);
    printf("  jmp .Lbegin%d\n", Lnum);
    printf(".Lend%d:\n", Lnum);
    return;
  case ND_WHILE:
    printf(".Lbegin%d:\n", Lnum);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", Lnum);
    gen(node->then);
    printf("  jmp .Lbegin%d\n", Lnum);
    printf(".Lend%d:\n", Lnum++);
    return;
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_LVAR:
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_FUNC:
    gen_func(node);
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  default:
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
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
  printf("  push rax\n");
}

void codegen(Node **code, int code_num){
  // Print out the first half of assembly.
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // Allocate memory for 26 variables.
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 208\n");

  // Traverse the AST to emit assembly.
  for (int i = 0; i < code_num; i++) {
    gen(code[i]);
    // Since there should be one value left in the stack 
    // as a result of evaluating the expression,
    // make sure to pop it to avoid overflowing the stack.
    printf("  pop rax\n");
  }
  
  // The result of the last expression is stored in RAX,
  // so it becomes the return value.
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}