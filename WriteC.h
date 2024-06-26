typedef struct tdef Output
{
	MemoryArena arena;
	size_t tabs;
	
	bool error;
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

static void decl WriteExpression(Output *, Expression *);

static void
func WriteType(Output *output, VarType *type)
{
	switch(type->id)
	{
		case ArrayTypeId:
		{
			ArrayType *t = (ArrayType *)type;
			WriteType(output, t->element_type);
			WriteString(output, "[");
			WriteExpression(output, t->size);
			WriteString(output, "]");
			break;
		}
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
				case Float32BaseTypeId:
				{
					WriteString(output, "float");
					break;
				}
				case UInt32BaseTypeId:
				{
					WriteString(output, "unsigned int");
					break;
				}
				default:
				{
					printf("Unknown base type %i\n", (int)t->base_id);
					output->error = true;
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
		case StructTypeId:
		{
			StructType *t = (StructType *)type;
			WriteToken(output, t->def->name);
			break;
		}
		default:
		{
			printf("Unknown var type %i!\n", (int)type->id);
			output->error = true;
			break;
		}
	}
}

static void
func WriteTypeAndVar(Output *output, VarType *type, Token var_name)
{
	if(type->id == PointerTypeId)
	{
		PointerType *t = (PointerType *)type;
		WriteType(output, t->pointed_type);
		WriteString(output, " *");
		WriteToken(output, var_name);
	}
	else if(type->id == ArrayTypeId)
	{
		VarType *base_type = type;
		while(base_type->id == ArrayTypeId)
		{
			ArrayType *t = (ArrayType *)base_type;
			base_type = t->element_type;
		}
		
		WriteType(output, base_type);
		WriteString(output, " ");
		WriteToken(output, var_name);
		
		VarType *t = type;
		while(t->id == ArrayTypeId)
		{
			ArrayType *array_type = (ArrayType *)t;
			WriteString(output, "[");
			WriteExpression(output, array_type->size);
			WriteString(output, "]");
			
			t = array_type->element_type;
		}
	}
	else
	{
		WriteType(output, type);
		if(type->id != PointerTypeId)
		{
			WriteString(output, " ");
		}
		WriteToken(output, var_name);
	}
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
		if(is_first_param)
		{
			is_first_param = false;
		}
		else
		{
			WriteString(output, ", ");
		}
		
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
		case AddExpressionId:
		{
			AddExpression *e = (AddExpression *)expression;
			WriteExpression(output, e->left);
			WriteString(output, " + ");
			WriteExpression(output, e->right);
			break;
		}
		case ArrayIndexExpressionId:
		{
			ArrayIndexExpression *e = (ArrayIndexExpression *)expression;
			
			if(e->array->type->id == StructTypeId)
			{
				StructType *type = (StructType *)e->array->type;
				WriteExpression(output, e->array);
				WriteString(output, ".");
				WriteToken(output, type->def->used_var->name);
			}
			else
			{
				WriteExpression(output, e->array);
			}

			WriteString(output, "[");
			WriteExpression(output, e->index);
			WriteString(output, "]");
			break;
		}
		case BoolConstantExpressionId:
		{
			BoolConstantExpression *e = (BoolConstantExpression *)expression;
			if(e->token.id == TrueTokenId)
			{
				WriteString(output, "1");
			}
			else
			{
				WriteString(output, "0");
			}
			
			break;
		}
		case CastExpressionId:
		{
			CastExpression *e = (CastExpression *)expression;
			WriteString(output, "(");
			WriteType(output, e->type);
			WriteString(output, ")");
			WriteExpression(output, e->value);
			break;
		}
		case DereferenceExpressionId:
		{
			DereferenceExpression *e = (DereferenceExpression *)expression;
			WriteString(output, "*");
			WriteExpression(output, e->pointer);
			break;
		}
		case FloatConstantExpressionId:
		{
			FloatConstantExpression *e = (FloatConstantExpression *)expression;
			WriteToken(output, e->token);
			WriteString(output, "f");
			break;
		}
		case FuncCallExpressionId:
		{
			FuncCallExpression *e = (FuncCallExpression *)expression;
			FuncDefinition *def = e->func_def;
			
			WriteToken(output, def->header.name);
			WriteString(output, "(");
			for(FuncCallArgument *arg = e->first_call_arg; arg; arg = arg->next)
			{
				WriteExpression(output, arg->arg);
				if(arg->next)
				{
					WriteString(output, ", ");
				}
			}
			WriteString(output, ")");
			break;
		}
		case IntegerConstantExpressionId:
		{
			IntegerConstantExpression *e = (IntegerConstantExpression *)expression;
			WriteToken(output, e->token);
			break;
		}
		case GreaterThanExpressionId:
		{
			GreaterThanExpression *e = (GreaterThanExpression *)expression;
			WriteExpression(output, e->left);
			WriteString(output, " > ");
			WriteExpression(output, e->right);
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
		case LessThanEqualExpressionId:
		{
			LessThanEqualExpression *e = (LessThanEqualExpression *)expression;
			WriteExpression(output, e->left);
			WriteString(output, " <= ");
			WriteExpression(output, e->right);
			break;
		}
		case MultiplyExpressionId:
		{
			MultiplyExpression *e = (MultiplyExpression *)expression;
			WriteExpression(output, e->left);
			WriteString(output, " * ");
			WriteExpression(output, e->right);
			break;
		}
		case NegativeExpressionId:
		{
			NegativeExpression *e = (NegativeExpression *)expression;
			WriteString(output, "-");
			WriteExpression(output, e->value);
			break;
		}
		case OperatorCallExpressionId:
		{
			OperatorCallExpression *e = (OperatorCallExpression *)expression;
			WriteToken(output, e->def->name);
			WriteString(output, "(");
			WriteExpression(output, e->left);
			WriteString(output, ", ");
			WriteExpression(output, e->right);
			WriteString(output, ")");
			break;
		}
		case ParenExpressionId:
		{
			ParenExpression *e = (ParenExpression *)expression;
			WriteString(output, "(");
			WriteExpression(output, e->in);
			WriteString(output, ")");
			break;
		}
		case StructVarExpressionId:
		{
			StructVarExpression *e = (StructVarExpression *)expression;
			WriteExpression(output, e->base);
			
			if(e->base->type->id == PointerTypeId)
			{
				WriteString(output, "->");
			}
			else
			{
				WriteString(output, ".");
			}
			
			WriteToken(output, e->var_name);
			
			break;
		}
		case SubtractExpressionId:
		{
			SubtractExpression *e = (SubtractExpression *)expression;
			WriteExpression(output, e->left);
			WriteString(output, " - ");
			WriteExpression(output, e->right);
			break;
		}
		
		case VarExpressionId:
		{
			VarExpression *e = (VarExpression *)expression;
			WriteToken(output, e->var.name);
			break;
		}
		default:
		{
			printf("Unknown expression type %i!\n", (int)expression->id);
			output->error = true;
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
		case AndEqualsInstructionId:
		{
			AndEqualsInstruction *i = (AndEqualsInstruction *)instruction;
			WriteExpression(output, i->left);
			WriteString(output, " &= ");
			WriteExpression(output, i->right);
			break;
		}
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
		case FuncCallInstructionId:
		{
			FuncCallInstruction *i = (FuncCallInstruction *)instruction;
			WriteExpression(output, (Expression *)i->e);
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
			if(i->value)
			{
				WriteExpression(output, i->value);
			}
			break;
		}
		default:
		{
			printf("Unknown instruction type: %i\n", instruction->id);
			output->error = true;
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

		if(NeedsSemicolon(instruction->id))
		{
			WriteString(output, ";");
		}
		
		WriteString(output, "\n");
		
		instruction = instruction->next;
	}
	
	output->tabs--;
	
	WriteTabs(output);
	WriteString(output, "}");
}

static void
func WriteStructDefinition(Output *output, StructDefinition *def)
{
	WriteString(output, "typedef struct ");
	WriteToken(output, def->name);
	
	WriteString(output, "\n{\n");
	
	output->tabs++;
	
	for(StructVar *var = def->first_var; var; var = var->next)
	{
		WriteTabs(output);
		WriteTypeAndVar(output, var->type, var->name);
		WriteString(output, ";\n");
	}
	
	output->tabs--;
	
	WriteString(output, "} ");
	WriteToken(output, def->name);
	WriteString(output, ";\n");
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
			case FuncDefinitionId:
			{
				FuncDefinition *def = (FuncDefinition *)definition;
				
				WriteFuncHeader(output, &def->header);

				if(!def->is_extern)
				{
					WriteString(output, "\n");
					WriteBlock(output, def->body);
					WriteString(output, "\n");
				}
				else
				{
					WriteString(output, ";\n");
				}
				
				break;
			}
			case OperatorDefinitionId:
			{
				OperatorDefinition *def = (OperatorDefinition *)definition;
				
				if(def->return_type)
				{
					WriteType(output, def->return_type);
					WriteString(output, " ");
				}
				else
				{
					WriteString(output, "void ");
				}
				
				WriteToken(output, def->name);
				WriteString(output, "(");
				
				WriteTypeAndVar(output, def->left_type, def->left_name);
				WriteString(output, ", ");
				
				WriteTypeAndVar(output, def->right_type, def->right_name);
				WriteString(output, ")\n");
				
				WriteBlock(output, def->body);
				WriteString(output, "\n");
				
				break;
			}
			case StructDefinitionId:
			{
				StructDefinition *def = (StructDefinition *)definition;
				WriteStructDefinition(output, def);
				break;
			}
			default:
			{
				printf("Unknown definition type %i!\n", (int)definition->id);
				output->error = true;
				break;
			}
		}
		
		elem = elem->next;
		first = false;
	}
}