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
	Expression *used_from;
};

struct tdef RuntimeStackState
{
	int first_var;
	int var_n;
	int size;
};

struct tdef Runtime
{
	char *stack;
	int max_stack_size;

	RuntimeVar *vars;
	int max_var_n;

	RuntimeStackState stack_state;
};

static void
func AddRuntimeVar(Runtime *runtime, Token name, Value *value)
{
	RuntimeVar var = {};
	var.name = name;
	var.value = value;
	runtime->vars[runtime->stack_state.var_n] = var;
	runtime->stack_state.var_n++;
}

static void
func AddRuntimeUseVar(Runtime *runtime, Token name, Expression *used_from)
{
	Assert(used_from);
	RuntimeVar var = {};
	var.name = name;
	var.used_from = used_from;
	runtime->vars[runtime->stack_state.var_n] = var;
	runtime->stack_state.var_n++;
}

static RuntimeVar *
func GetRuntimeVar(Runtime *runtime, Token name)
{
	RuntimeVar *result = 0;
	RuntimeStackState state = runtime->stack_state;
	for(int i = state.first_var; i < state.var_n; i++)
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
	Assert(runtime->stack_state.size + size <= runtime->max_stack_size);
	char *memory = runtime->stack + runtime->stack_state.size;
	runtime->stack_state.size += size;
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

static BaseType Bool32Type = CreateBaseType(Bool32BaseTypeId);

static BaseType Int32Type = CreateBaseType(Int32BaseTypeId);

static BaseType UInt32Type = CreateBaseType(UInt32BaseTypeId);
static PointerType UInt32PointerType = CreatePointerType(&UInt32Type.type);

// TODO: is it possible to not allocate Value structures on stack?

static Value *
func PushPointerValue(Runtime *runtime, VarType *type, void *address)
{
	Value *result = StackAllocType(runtime, Value);
	result->type = type;
	result->address = StackPushVar(runtime, void *, address);
	return result;
}

static Value *
func PushBool32Value(Runtime *runtime, bool value)
{
	Value *result = StackAllocType(runtime, Value);
	result->type = (VarType *)&Bool32Type;
	result->address = StackPushVar(runtime, int, (int)value);
	return result;
}

static Value *
func PushInt32Value(Runtime *runtime, int value)
{
	Value *result = StackAllocType(runtime, Value);
	result->type = (VarType *)&Int32Type;
	result->address = StackPushVar(runtime, int, value);
	return result;
}

static Value *
func PushUInt32Value(Runtime *runtime, unsigned int value)
{
	Value *result = StackAllocType(runtime, Value);
	result->type = (VarType *)&UInt32Type;
	result->address = StackPushVar(runtime, unsigned int, value);
	return result;
}

typedef long long I64;

static Value *
func PushIntegerConstantValue(Runtime *runtime, I64 value, BaseVarTypeId base_type)
{
	Value *result = 0;
	switch(base_type)
	{
		case Int32BaseTypeId:
		{
			result = PushInt32Value(runtime, (int)value);
			break;
		}
		case UInt32BaseTypeId:
		{
			result = PushUInt32Value(runtime, (unsigned int)value);
			break;
		}
		default:
		{
			DebugBreak();
		}
	}

	return result;
}

static Value *
func PushUInt32PointerValue(Runtime *runtime, unsigned int *value)
{
	Value *result = StackAllocType(runtime, Value);
	result->type = (VarType *)&UInt32PointerType;
	result->address = StackPushVar(runtime, unsigned int *, value);
	return result;
}

static Value *
func PushZeroedValueForType(Runtime *runtime, VarType *type)
{
	Assert(type);
	int size = GetTypeByteSize(type);

	Value *result = StackAllocType(runtime, Value);
	result->type = type;
	result->address = StackAlloc(runtime, size);

	char *address = (char *)result->address;
	for(int i = 0; i < size; i++)
	{
		address[i] = 0;
	}

	return result;
}

static ValueListElem *
func PushValueListElem(Runtime *runtime, Value *value)
{
	ValueListElem *result = StackAllocType(runtime, ValueListElem);
	result->value = value;
	result->next = 0;
	return result;
}

static Value *decl ExecuteFunction(Runtime *runtime, Func *function, ValueList *param_values);

static I64
func ParseIntFromToken(Token token)
{
	Assert(token.id == IntegerConstantTokenId);
	I64 result = 0;
	
	char *s = token.text;
	char *end = token.text + token.length;

	bool is_negative = false;
	if(s[0] == '-')
	{
		is_negative = true;
		s++;
	}
	else if(s[0] == '+')
	{
		s++;
	}

	if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
	{
		s += 2;
		while(s < end)
		{
			int digit = 0;
			if(s[0] >= '0' && s[0] <= '0')
			{
				digit = s[0] - '0';
			}
			else if(s[0] >= 'a' && s[0] <= 'f')
			{
				digit = 10 + s[0] - 'a';
			}
			else if(s[0] >= 'A' && s[0] <= 'F')
			{
				digit = 10 + s[0] - 'A';
			}

			result = 16 * result + digit;

			s++;
		}
	}
	else if(s[0] == '0' && (s[1] == 'b' || s[1] == 'B'))
	{
		s += 2;
		while(s < end)
		{
			int digit = (s[0] - '0');
			result = 2 * result + digit;

			s++;
		}
	}
	else if(s[0] == '0' && (s + 1 < end && IsOctalDigit(s[1])))
	{
		s++;
		while(s < end)
		{
			int digit = (s[0] - '0');
			result = 8 * result + digit;

			s++;
		}
	}
	else
	{
		while(s < end)
		{
			int digit = (s[0] - '0');
			result = 10 * result + digit;

			s++;
		}
	}

	if(is_negative)
	{
		result = -result;
	}

	return result;
}

static void
func I64ToValue(Value *value, I64 i)
{
	// TODO: also allow pointer
	Assert(value->type->id == BaseTypeId);
	BaseType *base_type = (BaseType *)value->type;

	switch(base_type->base_id)
	{
		case Int32BaseTypeId:
		{
			*(int *)value->address = (int)i;
			break;
		}
		case UInt32BaseTypeId:
		{
			*(unsigned int *)value->address = (unsigned int)i;
			break;
		}
		default:
		{
			DebugBreak();
		}
	}
}

static I64
func ValueToI64(Value *value)
{
	Assert(value->type->id == BaseTypeId);
	BaseType *base_type = (BaseType *)value->type;

	I64 result = 0;
	switch(base_type->base_id)
	{
		case Int32BaseTypeId:
		{
			int v = *(int *)value->address;
			result = (I64)v;
			break;
		}
		case UInt32BaseTypeId:
		{
			unsigned int v = *(unsigned int *)value->address;
			result = (I64)v;
			break;
		}
		default:
		{
			DebugBreak();
		}
	}

	return result;
}

static void *
func OffsetPointer(void *base, int offset)
{
	void *result = (void *)((char *)base + offset);
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
		case ParenExpressionId:
		{
			ParenExpression *e = (ParenExpression *)expression;

			result = ExecuteExpression(runtime, e->in);
			break;
		}
		case IntegerConstantExpressionId:
		{
			IntegerConstantExpression *e = (IntegerConstantExpression *)expression;
			I64 value = ParseIntFromToken(e->token);

			Assert(expression->type->id == BaseTypeId);
			BaseType *base_type = (BaseType *)expression->type;
			result = PushIntegerConstantValue(runtime, value, base_type->base_id);

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
			if(TypesEqual(left->type, right->type))
			{
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
			}
			else
			{
				Assert(IsPointerType(left->type));
				Assert(IsIntegerType(right->type));

				VarType *pointed_type = ((PointerType *)left->type)->pointed_type;
				int increment_size = GetTypeByteSize(pointed_type);

				I64 increment = ValueToI64(right);
				void *address = OffsetPointer(*(void **)left->address, (int)(increment_size * increment));
				result = PushPointerValue(runtime, left->type, address);
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
		case LessThanExpressionId:
		{
			LessThanExpression *e = (LessThanExpression *)expression;
			Value *left = ExecuteExpression(runtime, e->left);
			Value *right = ExecuteExpression(runtime, e->right);

			I64 left_value = ValueToI64(left);
			I64 right_value = ValueToI64(right);

			result = PushBool32Value(runtime, left_value < right_value);

			break;
		}
		case VarExpressionId:
		{
			VarExpression *e = (VarExpression *)expression;
			RuntimeVar *var = GetRuntimeVar(runtime, e->var.name);
			Assert(var);

			Expression *used_from = var->used_from;
			if(used_from)
			{
				Value *used_value = ExecuteExpression(runtime, used_from);

				void *base_address = used_value->address;
				
				VarType *type = used_from->type;
				if(type->id == PointerTypeId)
				{
					base_address = *(void **)base_address;
					type = ((PointerType *)type)->pointed_type;
				}

				Assert(type->id == StructTypeId);
				StructType *struct_type = (StructType *)type;

				StructVar *var = GetStructVar(struct_type->str, e->var.name);
				Assert(var);

				result = StackAllocType(runtime, Value);
				result->type = var->type;
				result->address = OffsetPointer(base_address, var->address_offset);
			}
			else
			{
				result = var->value;
			}

			break;
		}
		case MetaVarExpressionId:
		{
			MetaVarExpression *e = (MetaVarExpression *)expression;
			result = ExecuteExpression(runtime, e->var.expression);

			break;
		}
		case StructVarExpressionId:
		{
			StructVarExpression *e = (StructVarExpression *)expression;
			Value *str = ExecuteExpression(runtime, e->struct_expression);

			VarType *type = str->type;
			Assert(type->id == StructTypeId);

			StructType *struct_type = (StructType *)type;

			StructVar *var = GetStructVar(struct_type->str, e->var_name);
			Assert(var);

			result = StackAllocType(runtime, Value);
			result->type = var->type;
			result->address = (void *)((char *)str->address + var->address_offset);
			break;
		}
		case FuncCallExpressionId:
		{
			FuncCallExpression *e = (FuncCallExpression *)expression;

			ValueList *param_values = 0;
			ValueListElem *last_param_value = 0;

			FuncCallParam *param = e->first_param;
			while(param)
			{
				Value *value = ExecuteExpression(runtime, param->expression);
				ValueListElem *elem = PushValueListElem(runtime, value);
				if(!param_values)
				{
					param_values = elem;
					last_param_value = elem;
				}
				else
				{
					last_param_value->next = elem;
					last_param_value = elem;
				}

				param = param->next;
			}

			result = ExecuteFunction(runtime, e->f, param_values);

			break;
		}
		case AddressExpressionId:
		{
			AddressExpression *e = (AddressExpression *)expression;

			Value *value = ExecuteExpression(runtime, e->addressed);

			result = StackAllocType(runtime, Value);

			result->type = expression->type;
			result->address = StackAllocType(runtime, void *);
			*(void **)result->address = value->address;

			break;
		}
		case DereferenceExpressionId:
		{
			DereferenceExpression *e = (DereferenceExpression *)expression;

			Value *base = ExecuteExpression(runtime, e->base);

			Assert(base->type->id == PointerTypeId);
			PointerType *pointer_type = (PointerType *)base->type;

			result = StackAllocType(runtime, Value);

			result->type = pointer_type->pointed_type;
			result->address = *(void **)base->address;

			break;
		}
		default:
		{
			DebugBreak();
		}
	}

	return result;
}

static void
func Copy(void *to, void *from, int size)
{
	char *to_c = (char *)to;
	char *from_c = (char *)from;

	for(int i = 0; i < size; i++)
	{
		to_c[i] = from_c[i];
	}
}

static bool
func GetBoolValue(Value *value)
{
	Assert(value->type->id == BaseTypeId);
	BaseType *base_type = (BaseType *)value->type;
	Assert(base_type->base_id == Bool32BaseTypeId);

	int val = *(int *)value->address;
	Assert(val == 0 || val == 1);
	bool result = (val != 0);
	return val;
}

static void
func ExecuteInstruction(Runtime *runtime, Instruction *instruction)
{
	switch(instruction->id)
	{
		case NoInstructionId:
		case CreateMetaVariableInstructionId:
		{
			break;
		}
		case UseInstructionId:
		{
			UseInstruction *i = (UseInstruction *)instruction;

			VarType *type = i->expression->type;
			if(type->id == PointerTypeId)
			{
				type = ((PointerType *)type)->pointed_type;
			}

			Assert(type->id == StructTypeId);
			StructType *struct_type = (StructType *)type;

			Struct *str = struct_type->str;

			StructVar *var = str->first_var;
			while(var)
			{
				AddRuntimeUseVar(runtime, var->name, i->expression);

				var = var->next;
			}

			break;
		}
		case CreateVariableInstructionId:
		{
			CreateVariableInstruction *instr = (CreateVariableInstruction *)instruction;
			Value *value = 0;
			if(instr->init)
			{
				value = ExecuteExpression(runtime, instr->init);
			}
			else
			{
				value = PushZeroedValueForType(runtime, instr->type);
			}
			AddRuntimeVar(runtime, instr->var_name, value);
			break;
		}
		case AssignInstructionId:
		{
			AssignInstruction *i = (AssignInstruction *)instruction;
			// TODO: add modifiable bool to Value

			Value *left = ExecuteExpression(runtime, i->left);
			Value *right = ExecuteExpression(runtime, i->right);
			Assert(TypesEqual(left->type, right->type));

			int size = GetTypeByteSize(left->type);
			Copy(left->address, right->address, size);

			break;
		}
		case FuncCallInstructionId:
		{
			FuncCallInstruction *i = (FuncCallInstruction *)instruction;

			ExecuteExpression(runtime, (Expression *)i->call_expression);

			break;
		}
		case ForInstructionId:
		{
			RuntimeStackState stack_state = runtime->stack_state;

			ForInstruction *i = (ForInstruction *)instruction;

			ExecuteInstruction(runtime, i->init);

			while(1)
			{
				RuntimeStackState stack_state = runtime->stack_state;

				Value *condition = ExecuteExpression(runtime, i->condition);
				bool condition_value = GetBoolValue(condition);
				if(!condition_value)
				{
					break;
				}

				BlockInstruction *body = i->body;
				Instruction *instruction = body->first;


				bool did_break = false;
				while(instruction)
				{
					// TODO: handle nested break/continue
						// add flags to runtime?
					if(instruction->id == BreakInstructionId)
					{
						did_break = true;
						break;
					}
					else if(instruction->id == ContinueInstructionId)
					{
						ExecuteInstruction(runtime, i->advance);
						continue;
					}
					else
					{
						ExecuteInstruction(runtime, instruction);
					}
					instruction = instruction->next;
				}

				if(!did_break)
				{
					ExecuteInstruction(runtime, i->advance);
				}

				runtime->stack_state = stack_state;
			}

			runtime->stack_state = stack_state;

			break;
		}
		case IncrementInstructionId:
		{
			IncrementInstruction *i = (IncrementInstruction *)instruction;

			Value *value = ExecuteExpression(runtime, i->expression);

			Assert(IsPointerType(value->type) || IsNumericalType(value->type));

			// TODO: it should be incremented on the size of the value
				// for example, 32 bit integer should be incremented in an int, not in I64!
			I64 val = ValueToI64(value);
			val++;
			I64ToValue(value, val);

			break;
		}
		default:
		{
			DebugBreak();
		}
	}
}

static Value *
func ExecuteFunction(Runtime *runtime, Func *function, ValueList *param_values)
{
	RuntimeStackState stack_state = runtime->stack_state;

	runtime->stack_state.first_var = stack_state.var_n;

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
			// TODO: handle return instructions nested somewhere
				// add a flag to Runtime?
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

	runtime->stack_state = stack_state;

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

// *TODO: create a way to open a window and show texture generated in M64 code!
//	+show a Windows window
//  *show texture
//  -generate texture in interpreted M64 code
// TODO: command line interaction?

struct Bitmap
{
	int width;
	int height;
	unsigned int *memory;
	MemArena *arena;
};

static Bitmap global_bitmap;
static bool global_running;

static BITMAPINFO 
func GetBitmapInfo(Bitmap *bitmap)
{
	BITMAPINFO info = {};
	BITMAPINFOHEADER *header = &info.bmiHeader;
	header->biSize = sizeof(*header);
	header->biWidth = bitmap->width;
	header->biHeight = -bitmap->height;
	header->biPlanes = 1;
	header->biBitCount = 32;
	header->biCompression = BI_RGB;
	return info;
}

static void 
func ResizeBitmap(Bitmap *bitmap, int width, int height)
{
	bitmap->width = width;
	bitmap->height = height;

	bitmap->arena->used_size = 0;
	bitmap->memory = ArenaPushArray(bitmap->arena, width * height, unsigned int);
}

static LRESULT CALLBACK 
func WinCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;

	switch(message)
	{
		case WM_SIZE:
		{
			RECT rect = {};
			GetClientRect(window, &rect);

			int width = rect.right - rect.left;
			int height = rect.bottom - rect.top;

			ResizeBitmap(&global_bitmap, width, height);

			break;
		}
		case WM_DESTROY:
		case WM_CLOSE:
		{
			global_running = false;
			break;
		}
		default:
		{
			result = DefWindowProc(window, message, wparam, lparam);
			break;
		}
	}

	return result;
}

struct WindowTest
{
	Runtime runtime;
	Func *draw_func;
};

static WindowTest
func CreateWindowTest(DefinitionList *defs, MemArena *arena)
{
	WindowTest test = {};
	test.runtime = CreateRuntime(arena);
	test.draw_func = GetFuncByName(defs, "Draw");
	Assert(test.draw_func);
	return test;
}

static void 
func Draw(Bitmap *bitmap, WindowTest *test)
{
	Runtime *runtime = &test->runtime;
	
	Value *memory_value = PushUInt32PointerValue(runtime, bitmap->memory);
	Value *width_value = PushInt32Value(runtime, bitmap->width);
	Value *height_value = PushInt32Value(runtime, bitmap->height);

	// TODO: create helper functions for this
		// Maybe create a RuntimeFuncParams struct?
	ValueListElem *memory_elem = PushValueListElem(runtime, memory_value);
	ValueListElem *width_elem = PushValueListElem(runtime, width_value);
	ValueListElem *height_elem= PushValueListElem(runtime, height_value);

	memory_elem->next = width_elem;
	width_elem->next = height_elem;

	ExecuteFunction(runtime, test->draw_func, memory_elem);
}

static void 
func TestWindowsRuntime(HINSTANCE instance, MemArena *arena, DefinitionList *defs)
{
	WindowTest test = CreateWindowTest(defs, &runtime_arena);

	WNDCLASS wc = {};
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WinCallback;
	wc.hInstance = instance;
	wc.lpszClassName = "Test";

	global_bitmap.arena = arena;

	ATOM res = RegisterClass(&wc);
	Assert(res);

	HWND window = CreateWindowEx(
		0,
		wc.lpszClassName,
		"Test",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		256, 256,
		0, 0, instance, 0
	);
	Assert(window);

	HDC context = GetDC(window);
	MSG message = {};

	global_running = true;
	while(global_running)
	{
		BOOL result = GetMessageA(&message, 0, 0, 0);
		if(result > 0)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		else
		{
			break;
		}

		Bitmap *bitmap = &global_bitmap;
		Draw(bitmap, &test);

		BITMAPINFO bitmap_info = GetBitmapInfo(bitmap);

		RECT rect = {};
		GetClientRect(window, &rect);
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		HDC context = GetDC(window);
		StretchDIBits(context,
					  0, 0, bitmap->width, bitmap->height,
					  0, 0, width, height,
					  bitmap->memory,
					  &bitmap_info,
					  DIB_RGB_COLORS,
					  SRCCOPY
		);
		ReleaseDC(window, context);
	}
}


/*
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
*/