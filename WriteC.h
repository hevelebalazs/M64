typedef struct tdef Output
{
	MemoryArena arena;
	size_t tabs;
} Output;

static void 
func WriteChar(Output *output, char c)
{
	char *mem = ArenaPushType(&output->arena, char);
	*mem = c;
}

static void
func WriteTabs(Output *output)
{
	for(size_t i = 0; i < 4 * output->tabs; i++)
	{
		WriteChar(output, ' ');
	}
}

static void 
func WriteString(Output *output, char *string)
{
	for(size_t i = 0; string[i]; i++)
	{
		WriteChar(output, string[i]);
	}
}

static void 
func WriteToken(Output *output, Token token)
{
	for(size_t i = 0; i < token.length; i++)
	{
		WriteChar(output, token.text[i]);
	}
}

static void
func WriteType(Output *output, VarType *type)
{
	switch(type->id)
	{
		case BaseTypeId:
		{
			BaseType *t = (BaseType *)type;
			switch(t->base_id)
			{
				case BoolBaseTypeId:
				{
					WriteString(output, "int");
					break;
				}
				case Int32BaseTypeId:
				{
					WriteString(output, "int");
					break;
				}
			}
			break;
		}
		case PointerTypeId:
		{
			PointerType *t = (PointerType *)type;
			WriteType(output, t->pointed_type);
			WriteString(output, " *");
			break;
		}
	}
}

static void
func WriteTypeAndVar(Output *output, VarType *type, Token var_name)
{
	WriteType(output, type);
	if(type->id != PointerTypeId)
	{
		WriteString(output, " ");
	}
	WriteToken(output, var_name);
}

static void
func WriteFuncHeader(Output *output, FuncHeader *header)
{
	if(header->return_type)
	{
		WriteType(output, header->return_type);
		WriteString(output, " ");
	}
	else
	{
		WriteString(output, "void ");
	}
	
	WriteToken(output, header->name);
	WriteString(output, "(");
	
	bool is_first_param = true;
	FuncParam *param = header->first_param;
	while(param)
	{
		if(is_first_param) is_first_param = false;
		else WriteString(output, ", ");
		
		WriteTypeAndVar(output, param->type, param->name);
		
		param = param->next;
	}
	
	WriteString(output, ")");
}

static void
func WriteExpression(Output *output, Expression *expression)
{
	switch(expression->id)
	{
		case ArrayIndexExpressionId:
		{
			ArrayIndexExpression *e = (ArrayIndexExpression *)expression;
			WriteExpression(output, e->array);
			WriteString(output, "[");
			WriteExpression(output, e->index);
			WriteString(output, "]");
			break;
		}
		case IntegerConstantExpressionId:
		{
			IntegerConstantExpression *e = (IntegerConstantExpression *)expression;
			WriteToken(output, e->token);
			break;
		}
		case LessThanExpressionId:
		{
			LessThanExpression *e = (LessThanExpression *)expression;
			WriteExpression(output, e->left);
			WriteString(output, " < ");
			WriteExpression(output, e->right);
			break;
		}
		case VarExpressionId:
		{
			VarExpression *e = (VarExpression *)expression;
			WriteToken(output, e->var.name);
			break;
		}
	}
}

static void func WriteBlock(Output *, BlockInstruction *);

static void
func WriteInstruction(Output *output, Instruction *instruction)
{
	switch(instruction->id)
	{
		case AssignInstructionId:
		{
			AssignInstruction *i = (AssignInstruction *)instruction;
			WriteExpression(output, i->left);
			WriteString(output, " = ");
			WriteExpression(output, i->right);
			break;
		}
		case BlockInstructionId:
		{
			BlockInstruction *i = (BlockInstruction *)instruction;
			WriteBlock(output, i);
			break;
		}
		case CreateVariableInstructionId:
		{
			CreateVariableInstruction *i = (CreateVariableInstruction *)instruction;
			WriteTypeAndVar(output, i->type, i->name);
			WriteString(output, " = ");
			if(i->init)
			{
				WriteExpression(output, i->init);
			}
			else
			{
				WriteString(output, "{}");
			}
			break;
		}
		case IfInstructionId:
		{
			IfInstruction *i = (IfInstruction *)instruction;
			WriteString(output, "if(");
			WriteExpression(output, i->condition);
			WriteString(output, ")\n");
			
			WriteBlock(output, i->body);
			
			break;
		}
		case IncrementInstructionId:
		{
			IncrementInstruction *i = (IncrementInstruction *)instruction;
			WriteExpression(output, i->value);
			
			WriteString(output, "++");
			
			break;
		}
		case ForInstructionId:
		{
			ForInstruction *i = (ForInstruction *)instruction;
			
			WriteString(output, "for(");
			WriteInstruction(output, i->init);
			WriteString(output, "; ");
			
			WriteExpression(output, i->condition);
			WriteString(output, "; ");
			
			WriteInstruction(output, i->update);
			WriteString(output, ")\n");
			
			WriteBlock(output, i->body);
			
			break;
		}
		case ReturnInstructionId:
		{
			ReturnInstruction *i = (ReturnInstruction *)instruction;
			WriteString(output, "return ");
			WriteExpression(output, i->value);
			break;
		}
	}
}

static void
func WriteBlock(Output *output, BlockInstruction *block)
{
	WriteTabs(output);
	WriteString(output, "{\n");
	
	output->tabs++;
	
	Instruction *instruction = block->first;
	while(instruction)
	{
		WriteTabs(output);
		WriteInstruction(output, instruction);

		if(NeedsSemicolon(instruction->id)) WriteString(output, ";");
		WriteString(output, "\n");
		
		instruction = instruction->next;
	}
	
	output->tabs--;
	
	WriteTabs(output);
	WriteString(output, "}\n");
}

static void
func WriteDefinitionList(Output *output, DefinitionList *def_list)
{
	DefinitionListElem *elem = def_list;
	bool first = true;
	while(elem)
	{
		if(!first)
		{
			WriteString(output, "\n");
		}
		
		Definition *definition = elem->definition;
		switch(definition->id)
		{
			case CCodeDefinitionId:
			{
				CCodeDefinition *def = (CCodeDefinition *)definition;
				WriteToken(output, def->code);
				break;
			}
			case FuncDefinitionId:
			{
				FuncDefinition *def = (FuncDefinition *)definition;
				
				WriteFuncHeader(output, &def->header);
				WriteString(output, "\n");

				WriteBlock(output, def->body);
				
				break;
			}
		}
		
		elem = elem->next;
		first = false;
	}
}