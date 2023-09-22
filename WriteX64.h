typedef struct tdef
{
	MemoryArena arena;
	FuncDefinition *in_func;
} X64Output;

static void
func X64WriteChar(X64Output *output, char c)
{
	char *mem = ArenaPushType(&output->arena, char);
	*mem = c;
}

static void
func X64WriteString(X64Output *output, char *string)
{
	for(size_t i = 0; string[i]; i++)
	{
		X64WriteChar(output, string[i]);
	}
}

static void
func X64WriteToken(X64Output *output, Token token)
{
	for(size_t i = 0; i < token.length; i++)
	{
		X64WriteChar(output, token.text[i]);
	}
}

static void
func X64WriteTabs(X64Output *output)
{
	for(int i = 0; i < 4; i++)
	{
		X64WriteChar(output, ' ');
	}
}

static void
func X64WriteAsmInstruction(X64Output *output, char *instruction)
{
	X64WriteTabs(output);
	X64WriteString(output, instruction);
	X64WriteString(output, "\n");
}

static void
func X64WriteExpression(X64Output *output, Expression *expression)
{
	// Puts the expression value on the top of the stack
	switch(expression->id)
	{
		case IntegerConstantExpressionId:
		{
			IntegerConstantExpression *e = (IntegerConstantExpression *)expression;
			
			X64WriteTabs(output);
			X64WriteString(output, "push ");
			X64WriteToken(output, e->token);
			X64WriteString(output, "\n");
			
			break;
		}
		default:
		{
			// TODO: get expression name from its id
			printf("Writing expression type %i to asm not yet implemented.\n", (int)expression->id);
			break;
		}
	}
}

static void
func X64WriteInstruction(X64Output *output, Instruction *instruction)
{	
	MemoryArena *arena = &output->arena;
	switch(instruction->id)
	{
		case ReturnInstructionId:
		{
			ReturnInstruction *i = (ReturnInstruction *)instruction;
			X64WriteTabs(output);
			X64WriteString(output, "; ");
			WriteFormattedReturnInstruction(arena, i);
			X64WriteString(output, "\n");
			
			X64WriteExpression(output, i->value);
			
			X64WriteAsmInstruction(output, "pop rax");
			
			X64WriteTabs(output);
			X64WriteString(output, "jmp ");
			X64WriteToken(output, output->in_func->header.name);
			X64WriteString(output, "_return\n");
			break;
		}
		default:
		{
			// TODO: get instruction name from its id
			printf("Writing instruction type %i to asm not yet implemented.\n", (int)instruction->id);
			break;
		}
	}
	X64WriteString(output, "\n");
}

static void
func X64WriteBlock(X64Output *output, BlockInstruction *block)
{
	Instruction *instruction = block->first;
	while(instruction)
	{
		X64WriteInstruction(output, instruction);
		instruction = instruction->next;
	}
}

static void
func X64WriteFunc(X64Output *output, FuncDefinition *func_def)
{
	output->in_func = func_def;
	FuncHeader header = func_def->header;
	X64WriteToken(output, header.name);
	X64WriteString(output, ":\n");
	
	X64WriteAsmInstruction(output, "push rbp");
	X64WriteAsmInstruction(output, "mov rbp, rsp");
	X64WriteAsmInstruction(output, "sub rsp, 32");
	X64WriteString(output, "\n");
	
	X64WriteBlock(output, func_def->body);
	
	X64WriteTabs(output);
	X64WriteToken(output, header.name);
	X64WriteString(output, "_return:\n");
	
	X64WriteAsmInstruction(output, "add rsp, 32");
	X64WriteAsmInstruction(output, "pop rbp");
	X64WriteAsmInstruction(output, "ret");
	output->in_func = 0;
}

static void
func X64WriteDefinitionList(X64Output *output, DefinitionList *def_list)
{
	X64WriteString(output, "global main\n");
	X64WriteString(output, "\n");
	X64WriteString(output, "section .text\n");
	
	DefinitionListElem *elem = def_list;
	bool first = true;
	while(elem)
	{
		Definition *definition = elem->definition;
		switch(definition->id)
		{
			case CCodeDefinitionId:
			{
				printf("C code to x64 is not supported!\n");
				break;
			}
			case FuncDefinitionId:
			{
				FuncDefinition *def = (FuncDefinition *)definition;
				
				X64WriteFunc(output, def);
				break;
			}
		}
		
		elem = elem->next;
	}
}