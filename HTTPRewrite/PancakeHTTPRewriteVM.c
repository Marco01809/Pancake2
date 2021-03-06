
#include "PancakeHTTPRewriteVM.h"
#include "../PancakeLogger.h"

STATIC Byte PANCAKE_HTTP_REWRITE_VM_NOP(PancakeSocket *sock, void *op1, void *op2);

STATIC Byte PANCAKE_HTTP_REWRITE_VM_SET_BOOL(PancakeSocket *sock, void *op1, void *op2);
STATIC Byte PANCAKE_HTTP_REWRITE_VM_SET_INT(PancakeSocket *sock, void *op1, void *op2);
STATIC Byte PANCAKE_HTTP_REWRITE_VM_SET_STRING(PancakeSocket *sock, void *op1, void *op2);

STATIC Byte PANCAKE_HTTP_REWRITE_VM_IS_EQUAL_BOOL(PancakeSocket *sock, void *op1, void *op2);
STATIC Byte PANCAKE_HTTP_REWRITE_VM_IS_EQUAL_INT(PancakeSocket *sock, void *op1, void *op2);
STATIC Byte PANCAKE_HTTP_REWRITE_VM_IS_EQUAL_STRING(PancakeSocket *sock, void *op1, void *op2);

STATIC Byte PANCAKE_HTTP_REWRITE_VM_IS_NOT_EQUAL_BOOL(PancakeSocket *sock, void *op1, void *op2);
STATIC Byte PANCAKE_HTTP_REWRITE_VM_IS_NOT_EQUAL_INT(PancakeSocket *sock, void *op1, void *op2);
STATIC Byte PANCAKE_HTTP_REWRITE_VM_IS_NOT_EQUAL_STRING(PancakeSocket *sock, void *op1, void *op2);

STATIC Byte PANCAKE_HTTP_REWRITE_VM_CALL(PancakeSocket *sock, void *op1, void *op2);

STATIC Byte PANCAKE_HTTP_REWRITE_VM_ACTIVATE_SCOPE(PancakeSocket *sock, void *op1, void *op2);

#ifdef PANCAKE_DEBUG
UByte *PancakeHTTPRewriteOpcodeNames[] = {
	"NOP",
	"SET_BOOL",
	"SET_INT",
	"SET_STRING",
	"SET_VARIABLE",
	"IS_EQUAL_BOOL",
	"IS_EQUAL_INT",
	"IS_EQUAL_STRING",
	"IS_EQUAL_VARIABLE",
	"IS_NOT_EQUAL_BOOL",
	"IS_NOT_EQUAL_INT",
	"IS_NOT_EQUAL_STRING",
	"IS_NOT_EQUAL_VARIABLE",
	"CALL",
	"ACTIVATE_SCOPE"
};
#endif

PancakeHTTPRewriteOpcodeHandler PancakeHTTPRewriteOpcodeHandlers[] = {
	PANCAKE_HTTP_REWRITE_VM_NOP,
	PANCAKE_HTTP_REWRITE_VM_SET_BOOL,
	PANCAKE_HTTP_REWRITE_VM_SET_INT,
	PANCAKE_HTTP_REWRITE_VM_SET_STRING,
	NULL,
	PANCAKE_HTTP_REWRITE_VM_IS_EQUAL_BOOL,
	PANCAKE_HTTP_REWRITE_VM_IS_EQUAL_INT,
	PANCAKE_HTTP_REWRITE_VM_IS_EQUAL_STRING,
	NULL,
	PANCAKE_HTTP_REWRITE_VM_IS_NOT_EQUAL_BOOL,
	PANCAKE_HTTP_REWRITE_VM_IS_NOT_EQUAL_INT,
	PANCAKE_HTTP_REWRITE_VM_IS_NOT_EQUAL_STRING,
	NULL,
	PANCAKE_HTTP_REWRITE_VM_CALL,
	PANCAKE_HTTP_REWRITE_VM_ACTIVATE_SCOPE
};

#define NEXT_OPCODE 2
#define STOP_EXECUTION 1
#define STOP_EXECUTION_ALL 0
#define STOP_PARSER -1
#define FATAL -2

Byte PancakeHTTPRewriteVMExecute(PancakeSocket *sock, PancakeHTTPRewriteRuleset *ruleset) {
	UNative i;

	for(i = 0; i < ruleset->numOpcodes; i++) {
		PancakeHTTPRewriteOpcode *op = ruleset->opcodes[i];

		// Execute opcode and handle result
		switch(op->handler(sock, op->op1, op->op2)) {
			case STOP_EXECUTION:
				return 1;
			case STOP_EXECUTION_ALL:
				return 0;
			case FATAL:
				PancakeHTTPException(sock, 500);
				return -1;
			case STOP_PARSER:
				return -1;
		}
	}

	return 1;
}

STATIC Byte PANCAKE_HTTP_REWRITE_VM_NOP(PancakeSocket *sock, void *op1, void *op2) {
	/* Do nothing */
	return NEXT_OPCODE;
}

STATIC Byte PANCAKE_HTTP_REWRITE_VM_SET_BOOL(PancakeSocket *sock, void *op1, void *op2) {
	PancakeHTTPRewriteVariable *var = (PancakeHTTPRewriteVariable*) op1;

	if(var->locationType == PANCAKE_HTTP_REWRITE_LOCATION_CALLBACK) {
		PancakeHTTPRewriteValue value;

		value.boolv = (UByte) (UNative) op2;

		if(UNEXPECTED(!var->location.callback.set(sock, var, &value))) {
			return FATAL;
		}
	} else {
		var->location.ptr->boolv = (UByte) (UNative) op2;
	}

	return NEXT_OPCODE;
}

STATIC Byte PANCAKE_HTTP_REWRITE_VM_SET_INT(PancakeSocket *sock, void *op1, void *op2) {
	PancakeHTTPRewriteVariable *var = (PancakeHTTPRewriteVariable*) op1;

	if(var->locationType == PANCAKE_HTTP_REWRITE_LOCATION_CALLBACK) {
		PancakeHTTPRewriteValue value;

		value.intv = (Int32) (UNative) op2;

		if(UNEXPECTED(!var->location.callback.set(sock, var, &value))) {
			return FATAL;
		}
	} else {
		var->location.ptr->intv = (Int32) (UNative) op2;
	}

	return NEXT_OPCODE;
}

STATIC Byte PANCAKE_HTTP_REWRITE_VM_SET_STRING(PancakeSocket *sock, void *op1, void *op2) {
	PancakeHTTPRewriteVariable *var = (PancakeHTTPRewriteVariable*) op1;
	String *string = (String*) op2;

	if(var->locationType == PANCAKE_HTTP_REWRITE_LOCATION_CALLBACK) {
		PancakeHTTPRewriteValue value;

		value.stringv.value = string->value;
		value.stringv.length = string->length;

		if(UNEXPECTED(!var->location.callback.set(sock, var, &value))) {
			return FATAL;
		}
	} else {
		var->location.ptr->stringv.value = string->value;
		var->location.ptr->stringv.length = string->length;
	}

	return NEXT_OPCODE;
}

STATIC Byte PANCAKE_HTTP_REWRITE_VM_IS_EQUAL_BOOL(PancakeSocket *sock, void *op1, void *op2) {
	PancakeHTTPRewriteVariable *var = (PancakeHTTPRewriteVariable*) op1;

	if(var->locationType == PANCAKE_HTTP_REWRITE_LOCATION_CALLBACK) {
		PancakeHTTPRewriteValue value;

		if(UNEXPECTED(!var->location.callback.get(sock, var, &value))) {
			return FATAL;
		}

		if(value.boolv != (UByte) (UNative) op2) {
			return STOP_EXECUTION;
		}
	} else if(var->location.ptr->boolv != (UByte) (UNative) op2) {
		return STOP_EXECUTION;
	}

	return NEXT_OPCODE;
}

STATIC Byte PANCAKE_HTTP_REWRITE_VM_IS_EQUAL_INT(PancakeSocket *sock, void *op1, void *op2) {
	PancakeHTTPRewriteVariable *var = (PancakeHTTPRewriteVariable*) op1;

	if(var->locationType == PANCAKE_HTTP_REWRITE_LOCATION_CALLBACK) {
		PancakeHTTPRewriteValue value;

		if(UNEXPECTED(!var->location.callback.get(sock, var, &value))) {
			return FATAL;
		}

		if(value.intv != (Int32) (UNative) op2) {
			return STOP_EXECUTION;
		}
	} else if(var->location.ptr->intv != (Int32) (UNative) op2) {
		return STOP_EXECUTION;
	}

	return NEXT_OPCODE;
}

STATIC Byte PANCAKE_HTTP_REWRITE_VM_IS_EQUAL_STRING(PancakeSocket *sock, void *op1, void *op2) {
	PancakeHTTPRewriteVariable *var = (PancakeHTTPRewriteVariable*) op1;
	String *string = (String*) op2;

	if(var->locationType == PANCAKE_HTTP_REWRITE_LOCATION_CALLBACK) {
		PancakeHTTPRewriteValue value;

		if(UNEXPECTED(!var->location.callback.get(sock, var, &value))) {
			return FATAL;
		}

		if(value.stringv.length != string->length
		|| memcmp(value.stringv.value, string->value, string->length)) {
			return STOP_EXECUTION;
		}
	} else if(var->location.ptr->stringv.length != string->length
			|| memcmp(var->location.ptr->stringv.value, string->value, string->length)) {
		return STOP_EXECUTION;
	}

	return NEXT_OPCODE;
}

STATIC Byte PANCAKE_HTTP_REWRITE_VM_IS_NOT_EQUAL_BOOL(PancakeSocket *sock, void *op1, void *op2) {
	PancakeHTTPRewriteVariable *var = (PancakeHTTPRewriteVariable*) op1;

	if(var->locationType == PANCAKE_HTTP_REWRITE_LOCATION_CALLBACK) {
		PancakeHTTPRewriteValue value;

		if(UNEXPECTED(!var->location.callback.get(sock, var, &value))) {
			return FATAL;
		}

		if(value.boolv == (UByte) (UNative) op2) {
			return STOP_EXECUTION;
		}
	} else if(var->location.ptr->boolv == (UByte) (UNative) op2) {
		return STOP_EXECUTION;
	}

	return NEXT_OPCODE;
}

STATIC Byte PANCAKE_HTTP_REWRITE_VM_IS_NOT_EQUAL_INT(PancakeSocket *sock, void *op1, void *op2) {
	PancakeHTTPRewriteVariable *var = (PancakeHTTPRewriteVariable*) op1;

	if(var->locationType == PANCAKE_HTTP_REWRITE_LOCATION_CALLBACK) {
		PancakeHTTPRewriteValue value;

		if(UNEXPECTED(!var->location.callback.get(sock, var, &value))) {
			return FATAL;
		}

		if(value.intv == (Int32) (UNative) op2) {
			return STOP_EXECUTION;
		}
	} else if(var->location.ptr->intv == (Int32) (UNative) op2) {
		return STOP_EXECUTION;
	}

	return NEXT_OPCODE;
}

STATIC Byte PANCAKE_HTTP_REWRITE_VM_IS_NOT_EQUAL_STRING(PancakeSocket *sock, void *op1, void *op2) {
	PancakeHTTPRewriteVariable *var = (PancakeHTTPRewriteVariable*) op1;
	String *string = (String*) op2;

	if(var->locationType == PANCAKE_HTTP_REWRITE_LOCATION_CALLBACK) {
		PancakeHTTPRewriteValue value;

		if(UNEXPECTED(!var->location.callback.get(sock, var, &value))) {
			return FATAL;
		}

		if(value.stringv.length == string->length
		&& !memcmp(value.stringv.value, string->value, string->length)) {
			return STOP_EXECUTION;
		}
	} else if(var->location.ptr->stringv.length == string->length
			&& !memcmp(var->location.ptr->stringv.value, string->value, string->length)) {
		return STOP_EXECUTION;
	}

	return NEXT_OPCODE;
}

STATIC Byte PANCAKE_HTTP_REWRITE_VM_CALL(PancakeSocket *sock, void *op1, void *op2) {
	PancakeHTTPRewriteCallback *cb = (PancakeHTTPRewriteCallback*) op1;

	switch(cb->callback(sock)) {
		case 1:
			return NEXT_OPCODE;
		case 0:
			return STOP_EXECUTION;
		case -1:
			return STOP_PARSER;
		default:
			return FATAL;
	}
}

STATIC Byte PANCAKE_HTTP_REWRITE_VM_ACTIVATE_SCOPE(PancakeSocket *sock, void *op1, void *op2) {
	PancakeConfigurationScope *scope = (PancakeConfigurationScope*) op1;
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	PancakeConfigurationActivateScope(scope);
	PancakeConfigurationScopeGroupAddScope(&request->scopeGroup, scope);

	return NEXT_OPCODE;
}
