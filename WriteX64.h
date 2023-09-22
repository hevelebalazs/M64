typedef struct tdef
{
	MemoryArena arena;
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
func X64WriteInstruction(X64Output *output, char *instruction)
{
	X64WriteTabs(output);
	X64WriteString(output, instruction);
	X64WriteString(output, "\n");
}

static void
func X64WriteFunc(X64Output *output, FuncDefinition *func_def)
{
	FuncHeader header = func_def->header;
	X64WriteToken(output, header.name);
	X64WriteString(output, ":\n");
	
	X64WriteInstruction(output, "push rbp");
	X64WriteInstruction(output, "mov rbp, rsp");
	X64WriteInstruction(output, "sub rsp, 32");
	X64WriteString(output, "\n");
	
	X64WriteInstruction(output, "add rsp, 32");
	X64WriteInstruction(output, "pop rbp");
	X64WriteInstruction(output, "ret");
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