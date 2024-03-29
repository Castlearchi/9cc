#include "9cc.h"

static void gen(Node *node);
static void gen_addr(Node *node);
static void gen_funcall(Node *node);

static char *regards64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static char *regards32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
static char *regards8[] = {"DIL", "SIL", "DL", "CL", "R8B", "R9B"};

static Obj *current_fn;
int push_pop = 0;

static void push(char *reg)
{
  printf("  push %s\n", reg);
  push_pop++;
}

static void pop(char *reg)
{
  printf("  pop %s\n", reg);
  push_pop--;
}

static void load(Type *ty)
{
  if (!ty)
    return;
  if (ty->tkey == ARRAY)
    return;

  switch (ty->size)
  {
  case 1:
    printf("  mov AL, BYTE PTR [rax]\n");
    break;
  case 4:
    printf("  mov eax, [rax]\n");
    break;
  case 8:
    printf("  mov rax, [rax]\n");
    break;
  default:
    error("load: Unexpected size %d", ty->size);
  }
}

static void store(Type *ty)
{
  if (!ty)
    error("store: ty is none\n");

  pop("rdi");

  switch (ty->size)
  {
  case 1:
    printf("  mov [rdi], AL\n");
    break;
  case 4:
    printf("  mov [rdi], eax\n");
    break;
  case 8:
    printf("  mov [rdi], rax\n");
    break;
  default:
    error("store: Unexpected size %d", ty->size);
  }
}

static void gen_addr(Node *node)
{
  switch (node->kind)
  {
  case ND_VAR:
    if (node->var->is_local)
    {
      printf("  mov rax, rbp\n");
      printf("  sub rax, %d\n", node->var->offset);
    }
    else
    {
      printf("  lea rax, %s[rip]\n", node->var->name);
    }
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
    push("rax");
    nargs++;
  }

  for (int i = nargs - 1; i >= 0; i--)
  {
    pop(regards64[i]);
  }

  printf("  mov rax, 0\n");
  printf("  call %s\n", node->funcname);
  return;
}

static void gen(Node *node)
{
  static unsigned int Lnum = 0; // Lnum is serial number for control statement.
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
  case ND_SIZEOF:
    printf("  mov rax, %d\n", node->ty->size);
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
    printf("  mov eax, %d\n", node->val);
    return;
  case ND_NEG:
    gen(node->lhs);
    printf("  neg rax\n");
    return;
  case ND_VAR:
    gen_addr(node);
    load(node->ty);
    return;
  case ND_FUNCALL:
    gen_funcall(node);
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs);
    push("rax");
    gen(node->rhs);
    store(node->ty);
    return;
  case ND_ADDR: // &address
    gen_addr(node->lhs);
    return;
  case ND_DEREF: // *pointer
    gen(node->lhs);
    load(node->ty);
    return;
  default:
  }

  gen(node->lhs);
  push("rax");
  gen(node->rhs);
  push("rax");
  pop("rdi");
  pop("rax");

  char *lreg, *rreg;
  if (node->lhs->ty->tkey == PTR || node->lhs->ty->tkey == ARRAY)
  {
    lreg = "rax";
    rreg = "rdi";
  }
  else
  {
    lreg = "eax";
    rreg = "edi";
  }

  switch (node->kind)
  {
  case ND_ADD:
    printf("  add %s, %s\n", lreg, rreg);
    break;
  case ND_SUB:
    printf("  sub %s, %s\n", lreg, rreg);
    break;
  case ND_MUL:
    printf("  imul %s, %s\n", lreg, rreg);
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv %s\n", rreg);
    break;
  case ND_EQ:
    printf("  cmp %s, %s\n", lreg, rreg);
    printf("  sete al\n");
    printf("  movzb %s, al\n", rreg);
    break;
  case ND_NE:
    printf("  cmp %s, %s\n", lreg, rreg);
    printf("  setne al\n");
    printf("  movzb %s, al\n", rreg);
    break;
  case ND_LT:
    printf("  cmp %s, %s\n", lreg, rreg);
    printf("  setl al\n");
    printf("  movzb %s, al\n", rreg);
    break;
  case ND_LE:
    printf("  cmp %s, %s\n", lreg, rreg);
    printf("  setle al\n");
    printf("  movzb %s, al\n", lreg);
    break;
  default:
  }
}

static void store_gp(int i, int offset, int size)
{
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", offset);

  switch (size)
  {
  case 1:
    printf("  mov [rax], %s\n", regards8[i]);
    return;
  case 4:
    printf("  mov [rax], %s\n", regards32[i]);
    return;
  case 8:
    printf("  mov [rax], %s\n", regards64[i]);
    return;
  default:
    error("store_gp: Unexpected size %d", size);
  }
}

void emit_data(Obj *prog)
{
  for (Obj *var = prog; var; var = var->next)
  {
    if (var->is_function)
      continue;

    printf("  .data\n");
    printf("  .globl %s\n", var->name);
    printf("%s:\n", var->name);

    if (var->init_data)
    {
      for (int i = 0; i < var->ty->size; i++)
        printf("  .byte %d\n", var->init_data[i]);
    }
    else
    {
      printf("  .zero %d\n", var->ty->size);
    }
  }
}

void emit_text(Obj *prog)
{
  for (Obj *fn = prog; fn; fn = fn->next)
  {
    if (!fn->is_function)
      continue;
    current_fn = fn;

    printf("  .globl %s\n", current_fn->name);
    printf("  .text\n");
    printf("%s:\n", current_fn->name);

    // Allocate memory.
    push("rbp");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", current_fn->stack_size);

    // Save passed-by-register arguments to the stack
    int i = current_fn->regards_num - 1;
    for (Obj *param = current_fn->params; param->next; param = param->next)
    {
      store_gp(i--, param->offset, param->ty->size);
    }

    // Traverse the AST to emit assembly.
    int code_num = current_fn->stmt_count;
    for (int i = 0; i < code_num; i++)
    {
      add_type(current_fn->body[i]);
    }
    for (int i = 0; i < code_num; i++)
    {
      gen(current_fn->body[i]);
    }

    printf(".L.return.%s:\n", current_fn->name);
    printf("  mov rsp, rbp\n");
    pop("rbp");
    printf("  ret\n");
  }
}

void codegen(Obj *prog)
{
  printf("  .intel_syntax noprefix\n");

  emit_data(prog);
  emit_text(prog);

  if (push_pop != 0)
    error("pushとpopの数が合わない push - pop = %d\n", push_pop);
}