#include <windows.h>
#include <stdio.h>
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

struct Object
{
	u8 type;
	union {
		u8 as_u8;
		s32 as_s32;
		u32 as_u32;
	};
};

struct Stack
{
	Object* memory;
	u32 size;
	u32 top;
};

Stack create_stack(u32 size)
{
	Stack stack;
	stack.memory = new Object[size];
	stack.size = size;
	stack.top = 0;
	return stack;
}

u32 stack_push(Stack* stack, Object object)
{
	stack->memory[stack->top++] = object;
	return stack->top;
}

Object stack_pop(Stack* stack)
{
	return stack->memory[--stack->top];
}

Object stack_peek(Stack* stack)
{
	return stack->memory[stack->top - 1];
}

typedef u8*(*bytecode_instruction)(u8*, Stack*);

u8* no_op_instruction(u8* code, Stack*)
{
	return code + 1;
}

u8* push_char_instruction(u8* code, Stack* stack)
{
	Object object;
	object.type = 'c';
	object.as_u8 = *(code + 1);

	stack_push(stack, object);
	
	return code + 2;
}

u8* emit_char_instruction(u8* code, Stack* stack)
{
	auto object = stack_pop(stack);
	putchar(object.as_u8);
	return code + 1;
}

u8* push_int_instruction(u8* code, Stack* stack)
{
	Object object;
	object.type = 's';
	object.as_s32 = *(s32*)(code + 1);
	stack_push(stack, object);
	return code + 5;
}

u8* add_int_instruction(u8* code, Stack* stack)
{
	Object result;
	result.type = 's';
	
	auto op1 = stack_pop(stack);
	auto op2 = stack_pop(stack);

	result.as_s32 = op1.as_s32 + op2.as_s32;

	stack_push(stack, result);
	
	return code + 1;
}

u8* div_int_instruction(u8* code, Stack* stack)
{
	Object result;
	result.type = 's';
	
	auto op1 = stack_pop(stack);
	auto op2 = stack_pop(stack);

	result.as_s32 = op1.as_s32 / op2.as_s32;

	stack_push(stack, result);
	
	return code + 1;
}

u8* mult_int_instruction(u8* code, Stack* stack)
{
	Object result;
	result.type = 's';
	
	auto op1 = stack_pop(stack);
	auto op2 = stack_pop(stack);

	result.as_s32 = op1.as_s32 * op2.as_s32;

	stack_push(stack, result);
	
	return code + 1;
}

u8* mod_int_instruction(u8* code, Stack* stack)
{
	Object result;
	result.type = 's';
	
	auto op1 = stack_pop(stack);
	auto op2 = stack_pop(stack);

	result.as_s32 = op1.as_s32 % op2.as_s32;

	stack_push(stack, result);
	
	return code + 1;
}

u8* sub_int_instruction(u8* code, Stack* stack)
{
	Object result;
	result.type = 's';
	
	auto op1 = stack_pop(stack);
	auto op2 = stack_pop(stack);

	result.as_s32 = op1.as_s32 - op2.as_s32;

	stack_push(stack, result);
	
	return code + 1;
}

u8* jump_instruction(u8* code, Stack* stack)
{
	auto offset = *((s32*)(code + 1));
	return code + offset;
}

u8* jump_zero_instruction(u8* code, Stack* stack)
{
	auto object = stack_pop(stack);
	if(object.as_u32 == 0)
	{
		auto offset = *((s32*)(code + 1));
		return code + offset;
	}
	return code + 5;
}

u8* jump_not_zero_instruction(u8* code, Stack* stack)
{
	auto object = stack_pop(stack);
	if(object.as_u32 != 0)
	{
		auto offset = *((s32*)(code + 1));
		return code + offset;
	}
	return code + 5;
}


u8* duplicate_instruction(u8* code, Stack* stack)
{
	auto object = stack_peek(stack);
	stack_push(stack, object);
	return code + 1;
}

void print_usage(char* first_arg)
{
	printf("Usage: %s <file path>\n", first_arg);
}

u8* read_file(char* filename)
{
	u8* result = NULL;

	HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(file != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER file_size;
		if(GetFileSizeEx(file, &file_size))
		{
			result = (u8*)VirtualAlloc(0, file_size.QuadPart, MEM_COMMIT, PAGE_READWRITE);
			if(result)
			{
				DWORD file_size_32 = (DWORD)file_size.QuadPart;
				DWORD bytes_read;
				if(ReadFile(file, result, file_size_32, &bytes_read, 0) &&
					 bytes_read == file_size_32)
				{
					
				}
			}
		}
		CloseHandle(file);
	}
	return result;
}

#define INSTRUCTION_HALT 'h'

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		print_usage(argv[0]);
		return -1;
	}

	u8* code = read_file(argv[1]);
	
	if(!code)
	{
		print_usage(argv[0]);
		return -1;
	}
	
	bytecode_instruction handlers[256];

	for(int i = 0; i < 256; ++i)
	{
		handlers[i] = no_op_instruction;
	}

	handlers['c'] = push_char_instruction;
	handlers['i'] = push_int_instruction;
	handlers['a'] = add_int_instruction;
	handlers['s'] = sub_int_instruction;
	handlers['m'] = mult_int_instruction;
	handlers['M'] = mod_int_instruction;
	handlers['d'] = div_int_instruction;
	handlers['e'] = emit_char_instruction;
	handlers['j'] = jump_instruction;
	handlers['z'] = jump_zero_instruction;
	handlers['Z'] = jump_not_zero_instruction;
	handlers['D'] = duplicate_instruction;
	
	
	
	u8* ip = code;
	auto stack = create_stack(1024);
	
	while(*ip != INSTRUCTION_HALT)
		ip = handlers[*ip](ip, &stack);
	
	return 0;
}
