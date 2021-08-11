struct tdef Value
{
	VarType *type;
	void *address;
};

struct tdef ValueListElem
{
	ValueListElem *next;

	Value *value;
};

typedef ValueListElem ValueList;

struct tdef RuntimeVar
{
	Token name;
	Value *value;
};

struct tdef Runtime
{
	char *stack;
	int stack_size;
	int max_stack_size;

	RuntimeVar *vars;
	int var_n;
	int max_var_n;
};

static void
func AddRuntimeVar(Runtime *runtime, Token name, Value *value)
{
	RuntimeVar var = {};
	var.name = name;
	var.value = value;
	runtime->vars[runtime->var_n] = var;
	runtime->var_n++;
}

static RuntimeVar *
func GetRuntimeVar(Runtime *runtime, Token name)
{
	RuntimeVar *result = 0;
	for(int i = 0; i < runtime->var_n; i++)
	{
		RuntimeVar *var = &runtime->vars[i];
		if(TokensEqual(var->name, name))
		{
			result = var;
			break;
		}
	}
	return result;
}

static char *
func StackAlloc(Runtime *runtime, int size)
{
	Assert(runtime->stack_size + size <= runtime->max_stack_size);
	char *memory = runtime->stack + runtime->stack_size;
	runtime->stack_size += size;
	return memory;
}

static char *
func StackPush(Runtime *runtime, int size, char *value)
{
	char *mem = StackAlloc(runtime, size);
	for(int i = 0; i < size; i++)
	{
		mem[i] = value[i];
	}
	return mem;
}

#define StackAllocType(runtime, T) (T *)StackAlloc(runtime, sizeof(T))
#define StackPushVar(runtime, T, value) StackPush(runtime, sizeof(T), (char *)&(value))

static BaseType Int32BaseType = CreateBaseType(Int32BaseTypeId);
static VarType *Int32Type = (VarType *)&Int32BaseType;

static Value *
func PushInt32Value(Runtime *runtime, int value)
{
	Value *result = StackAllocType(runtime, Value);
	result->type = Int32Type;
	result->address = StackPushVar(runtime, int, value);
	return result;
}

static Value *
func ExecuteExpression(Runtime *runtime, Expression *expression)
{
	Value *result = 0;
	switch(expression->id)
	{
		case NoExpressionId:
		{
			break;
		}
		case AddExpressionId:
		{
			AddExpression *expr = (AddExpression *)expression;
			Value *left = ExecuteExpression(runtime, expr->left);
			Value *right = ExecuteExpression(runtime, expr->right);

			void *left_address = left->address;
			void *right_address = right->address;

			Assert(left && right);
			Assert(TypesEqual(left->type, right->type));
			VarType *type = left->type;
			Assert(IsNumericalType(type));

			switch(type->id)
			{
				case Int32BaseTypeId:
				{
					int value = *(int *)left_address + *(int*)right_address;
					result = PushInt32Value(runtime, value);
					break;
				}
				default:
				{
					DebugBreak();
				}
			}

			break;
		}
		case ProductExpressionId:
		{
			AddExpression *expr = (AddExpression *)expression;
			Value *left = ExecuteExpression(runtime, expr->left);
			Value *right = ExecuteExpression(runtime, expr->right);

			void *left_address = left->address;
			void *right_address = right->address;

			Assert(left && right);
			Assert(TypesEqual(left->type, right->type));
			VarType *type = left->type;
			Assert(IsNumericalType(type));

			Assert(type->id == BaseTypeId);
			BaseType *base_type = (BaseType *)type;

			switch(base_type->base_id)
			{
				case Int32BaseTypeId:
				{
					int value = (*(int *)left_address) * (*(int*)right_address);
					result = PushInt32Value(runtime, value);
					break;
				}
				default:
				{
					DebugBreak();
				}
			}

			break;
		}
		case VarExpressionId:
		{
			VarExpression *expr = (VarExpression *)expression;
			RuntimeVar *var = GetRuntimeVar(runtime, expr->var.name);
			Assert(var);
			result = var->value;
			break;
		}
		default:
		{
			DebugBreak();
		}
		/*
		AddressExpressionId,
		AndExpressionId,
		ArrayIndexExpressionId,
		BitAndExpressionId,
		BoolConstantExpressionId,
		CastExpressionId,
		CharacterConstantExpressionId,
		ConstructorCallExpressionId,
		DereferenceExpressionId,
		DivideExpressionId,
		EnumMemberExpressionId,
		EqualExpressionId,
		FuncCallExpressionId,
		GreaterThanExpressionId,
		GreaterThanOrEqualToExpressionId,
		IntegerConstantExpressionId,
		InvertExpressionId,
		LeftShiftExpressionId,
		LessThanExpressionId,
		LessThanOrEqualToExpressionId,
		MetaVarExpressionId,
		NegateExpressionId,
		NotEqualExpressionId,
		OperatorCallExpressionId,
		OrExpressionId,
		ParenExpressionId,
		RealConstantExpressionId,
		RightShiftExpressionId,
		StringConstantExpressionId,
		StructVarExpressionId,
		StructDefVarExpressionId,
		StructMetaVarExpressionId,
		SubtractExpressionId,
		TernaryOperatorExpressionId,
		*/
	}

	return result;
}

static void
func ExecuteInstruction(Runtime *runtime, Instruction *instruction)
{
	switch(instruction->id)
	{
		case NoInstructionId:
		{
			break;
		}
		case CreateVariableInstructionId:
		{
			CreateVariableInstruction *instr = (CreateVariableInstruction *)instruction;
			Value *value = ExecuteExpression(runtime, instr->init);
			AddRuntimeVar(runtime, instr->var_name, value);
			break;
		}
		default:
		{
			DebugBreak();
		}
		/*
		AssignInstructionId,
		BlockInstructionId,
		BreakInstructionId,
		ContinueInstructionId,
		CreateMetaVariableInstructionId,
		DeclInstructionId,
		DecrementInstructionId,
		ElseInstructionId,
		ForInstructionId,
		FuncCallInstructionId,
		IfInstructionId,
		IncrementInstructionId,
		MinusEqualsInstructionId,
		OrEqualsInstructionId,
		PlusEqualsInstructionId,
		ReturnInstructionId,
		StarEqualsInstructionId,
		UseInstructionId,
		WhileInstructionId,
		*/
	}
}

static Value *
func ExecuteFunction(Runtime *runtime, Func *function, ValueList *param_values)
{
	Value *return_value = 0;
	FuncHeader header = function->header;
	FuncParam *param = header.first_param;
	ValueListElem *param_value = param_values;
	while(param_value)
	{
		Assert(param);
		Assert(TypesEqual(param->type, param_value->value->type));

		AddRuntimeVar(runtime, param->name, param_value->value);

		param = param->next;
		param_value = param_value->next;
	}
	Assert(param == 0);

	BlockInstruction *body = function->body;
	Instruction *instruction = body->first;
	while(instruction)
	{
		if(instruction->id == ReturnInstructionId)
		{
			ReturnInstruction *instr = (ReturnInstruction *)instruction;
			return_value = ExecuteExpression(runtime, instr->value);
			break;
		}
		else
		{
			ExecuteInstruction(runtime, instruction);
		}
		instruction = instruction->next;
	}

	return return_value;
}

CreateStaticArena(runtime_arena, 64 * 1024 * 1024);

static Runtime
func CreateRuntime(MemArena *arena)
{
	Runtime runtime = {};
	int max_var_n = 1024;
	runtime.max_var_n = max_var_n;
	runtime.vars = ArenaPushArray(arena, max_var_n, RuntimeVar);

	runtime.stack = arena->memory + arena->used_size;
	runtime.max_stack_size = arena->max_size - arena->used_size;

	return runtime;
}

static ValueListElem *
func PushValueListElem(Runtime *runtime, Value *value)
{
	ValueListElem *result = StackAllocType(runtime, ValueListElem);
	result->value = value;
	result->next = 0;
	return result;
}

static Func *
func GetFuncByName(DefinitionList *defs, char *name)
{
	Func *result = 0;
	DefinitionListElem *elem = defs;
	while(elem)
	{
		if(elem->definition->id == FuncDefinitionId)
		{
			FuncDefinition *def = (FuncDefinition *)elem->definition;
			if(TokenEquals(def->function->header.name, name))
			{
				result = def->function;
				break;
			}
		}
		elem = elem->next;
	}

	return result;
}

static void
func PrintValue(Value *value)
{
	VarType *type = value->type;
	switch(type->id)
	{
		case BaseTypeId:
		{
			BaseType *base_type = (BaseType *)type;
			switch(base_type->base_id)
			{
				case Int32BaseTypeId:
				{
					int val = *(int *)value->address;
					printf("%i", val);
					break;
				}
				default:
				{
					DebugBreak();
				}
			}
			break;
		}
		default:
		{
			DebugBreak();
		}
	}
}

// TODO: create a way to open a window and show texture generated in M64 code!
// TODO: command line interaction?
static void
func TestRuntime(DefinitionList *defs)
{
	Runtime runtime = CreateRuntime(&runtime_arena);
	Value *value = PushInt32Value(&runtime, 5);
	ValueListElem *param = PushValueListElem(&runtime, value);
	Func *function = GetFuncByName(defs, "SquareInt");
	Assert(function);
	Value *result = ExecuteFunction(&runtime, function, param);
	printf("Value = ");
	PrintValue(result);
	printf("\n");

	printf("Press Enter...\n");
	char c;
	scanf_s("%c", &c, 1);
}