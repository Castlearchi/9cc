#include "9cc.h"

Type *ty_int = &(Type){INT, 4};
Type *ty_ptr = &(Type){PTR, 8};

void add_type(Node *node)
{
    if (!node || node->ty)
        return;

    add_type(node->next);
    add_type(node->lhs);
    add_type(node->rhs);
    add_type(node->cond);
    add_type(node->then);
    add_type(node->els);
    add_type(node->init);
    add_type(node->inc);
    for (int i = 0; i < (node->block_count); i++)
        add_type(node->block[i]);
    for (Node *arg = node->args; arg; arg = arg->next)
        add_type(arg);

    switch (node->kind)
    {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_NUM:
        node->ty = ty_int;
        return;
    case ND_NEG:
    case ND_ASSIGN:
    case ND_RETURN:
        node->ty = node->lhs->ty;
        return;
    case ND_VAR:
        node->ty = node->var->ty;
        return;
    case ND_IF:
    case ND_IFELSE:
    case ND_ELSE:
    case ND_WHILE:
    case ND_FOR:
    case ND_BLOCK:
    case ND_NONE:
        return;
    case ND_FUNCALL:
        node->ty = ty_int;
        return;
    case ND_ADDR:
        node->ty = ty_ptr;
        node->ty->ptr_to = node->lhs->ty->ptr_to;
        return;
    case ND_DEREF:
        if (!node->lhs->ty->ptr_to)
            error("invalid pointer dereference");

        node->ty->ptr_to = node->lhs->ty->ptr_to;
        return;

    default:
        break;
    }
}
