
#ifndef _PANCAKE_HTTP_REWRITE_VM_H
#define _PANCAKE_HTTP_REWRITE_VM_H

#include "Pancake.h"

#ifdef PANCAKE_HTTP_REWRITE

#include "PancakeHTTPRewrite.h"

/* VM Opcodes */
#define PANCAKE_HTTP_REWRITE_OP_NOP 0
#define PANCAKE_HTTP_REWRITE_OP_SET_BOOL 1
#define PANCAKE_HTTP_REWRITE_OP_SET_INT 2
#define PANCAKE_HTTP_REWRITE_OP_SET_STRING 3
#define PANCAKE_HTTP_REWRITE_OP_SET_VARIABLE 4

extern PancakeHTTPRewriteOpcodeHandler PancakeHTTPRewriteOpcodeHandlers[];

#ifdef PANCAKE_DEBUG
extern UByte *PancakeHTTPRewriteOpcodeNames[];
#endif

Byte PancakeHTTPRewriteVMExecute(PancakeSocket *sock, PancakeHTTPRewriteRuleset *ruleset);

#endif

#endif
