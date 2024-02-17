#include <stdio.h>
#include <stdlib.h>

#define func
#define decl
#define tdef

#define bool int
#define true 1
#define false 0

typedef struct tdef MemoryArena
{
	char *memory;
	size_t max_size;
	size_t used_size;
} MemoryArena;

static MemoryArena
func CreateArena(size_t max_size)
{
	MemoryArena arena = {};
	arena.max_size = max_size;
	arena.used_size = 0;
	arena.memory = malloc(max_size);
	return arena;
}

static char *
func ArenaPush(MemoryArena *arena, size_t size)
{
	char *memory = arena->memory + arena->used_size;
	arena->used_size += size;
	while(arena->used_size > arena->max_size)
	{
		printf("Arena ran out of memory!\n");
		return 0;
	}
	return memory;
}

#define ArenaPushType(arena, type) (type *)ArenaPush(arena, sizeof(type))
#define ArenaPushArray(arena, count, type) (type *)ArenaPush(arena, (count) * sizeof(type))

typedef struct tdef CodePosition
{
	char *at;
	size_t row;
	size_t col;
} CodePosition;

typedef enum tdef TokenId
{
	UnknownTokenId,
	
	AndEqualsTokenId,
	AtTokenId,
	CCodeTokenId,
	CloseBracesTokenId,
	CloseBracketsTokenId,
	CloseBracketsCloseBracketsTokenId,
	CloseParenTokenId,
	CommaTokenId,
	ColonColonTokenId,
	ColonEqualsTokenId,
	ColonTokenId,
	DotTokenId,
	EndOfFileTokenId,
	EqualsTokenId,
	ExternTokenId,
	FalseTokenId,
	FloatConstantTokenId,
	ForTokenId,
	FuncTokenId,
	GreaterThanTokenId,
	IfTokenId,
	IntegerConstantTokenId,
	LessThanTokenId,
	LessThanEqualTokenId,
	MinusTokenId,
	NameTokenId,
	OpenBracesTokenId,
	OpenBracketsTokenId,
	OpenBracketsOpenBracketsTokenId,
	OpenParenTokenId,
	PlusPlusTokenId,
	PlusTokenId,
	PoundCCodeTokenId,
	ReturnTokenId,
	SemiColonTokenId,
	StarTokenId,
	StructTokenId,
	ToTokenId,
	TrueTokenId
} TokenId;

typedef struct tdef Token
{
	TokenId id;
	char *text;
	size_t length;
	size_t row;
	size_t col;
} Token;

typedef enum tdef VarTypeId
{
	NoTypeId,
	ArrayTypeId,
	BaseTypeId,
	PointerTypeId,
	StructTypeId
} VarTypeId;

typedef struct tdef VarType
{
	VarTypeId id;
} VarType;

typedef struct tdef Var
{
	VarType *type;
	Token name;
} Var;

struct decl Expression;

typedef struct tdef ArrayType
{
	VarType type;
	
	struct Expression *size;
	VarType *element_type;
} ArrayType;

typedef enum tdef BaseVarTypeId
{
	BoolBaseTypeId,
	Int32BaseTypeId,
	Float32BaseTypeId,
	UInt32BaseTypeId
} BaseVarTypeId;

typedef struct tdef BaseType
{
	VarType type;
	
	BaseVarTypeId base_id;
} BaseType;

static BaseType
func BaseTypeInit(BaseVarTypeId id)
{
	BaseType type = {};
	type.type.id = BaseTypeId;
	type.base_id = id;
	return type;
}

typedef struct tdef PointerType
{
	VarType type;
	
	VarType *pointed_type;
} PointerType;

typedef struct tdef StructType
{
	VarType type;
	
	struct StructDefinition *def;
} StructType;

static bool
func IsBoolType(VarType *type)
{
	if(type->id == BaseTypeId)
	{
		BaseType *base = (BaseType *)type;
		return (base->base_id == BoolBaseTypeId);
	}
	
	return false;
}

static bool
func IsIntegerType(VarType *type)
{
	if(type->id == BaseTypeId)
	{
		BaseType *base = (BaseType *)type;
		switch(base->base_id)
		{
			case Int32BaseTypeId:
				return true;
		}
	}
	
	return false;
}

#define VarStackMaxSize 64
typedef struct tdef VarStack
{
	Var *vars;
	size_t size;
} VarStack;

typedef struct tdef CodeLine
{
	char *string;
	size_t length;
} CodeLine;

struct decl VarType;
struct decl FuncDefinition;

typedef struct tdef ParseInput
{
	MemoryArena arena;
	size_t line_n;
	CodeLine *lines;
	CodePosition *pos;
	VarStack var_stack;
	bool any_error;
	Token last_token;
	
	struct StructDefinition *first_struct_definition;
	
	struct FuncDefinition *func_definition;
	struct FuncDefinition *first_func_definition;
	
	VarType *bool_type;
	VarType *int_type;
	VarType *float_type;
	VarType *uint_type;
} ParseInput;

static char *
func ReadFileToMemory(FILE *file)
{
	size_t size = 0;
	size_t buffer_size = 1024;
	char *buffer = malloc(buffer_size);

	while(1)
	{
		if(size >= buffer_size)
		{
			buffer_size *= 2;
			buffer = realloc(buffer, buffer_size);
		}

		char c;
		int res = fscanf(file, "%c", &c);
		if(res != 1)
			break;

		buffer[size] = c;
		size++;
	}

	buffer = realloc(buffer, size + 1);
	buffer[size] = 0;
	return buffer;
}

static void
func PrintLine(CodeLine line)
{
	printf("%.*s\n", line.length, line.string);
}

static void
func PrintTokenInLine(ParseInput *input, Token token)
{
	CodeLine line = input->lines[token.row];
	PrintLine(input->lines[token.row]);
	for(size_t i = 0; i < token.col - 1; i++)
	{
		if(line.string[i] == '\t')
		{
			printf("\t");
		}
		else
		{
			printf(" ");
		}
	}
	for(int i = 0; i < token.length; i++)
	{
		printf("^");		
	}

	printf("\n");
}

static void
func SetErrorToken(ParseInput *input, char *description, Token token)
{
	if(input->any_error)
	{
		return;
	}
	
	printf("Error: %s\n", description);
	printf("In line %i\n", token.row);
	PrintTokenInLine(input, token);
	
	input->any_error = true;
}

static void decl WriteErrorVarType(VarType *);

static void
func WriteErrorMessageVarType(char *message, VarType *type)
{
	printf("%s", message);
	WriteErrorVarType(type);
	printf("\n");
}

static void
func SetError(ParseInput *input, char *description)
{
	SetErrorToken(input, description, input->last_token);
}

static bool
func TokensEqual(Token token1, Token token2)
{
	if(token1.id != token2.id)
		return false;
	if(token1.length != token2.length)
		return false;
	
	for(size_t i = 0; i < token1.length; i++)
	{
		if(token1.text[i] != token2.text[i])
			return false;
	}
	
	return true;
}



static bool 
func TokenEquals(Token token, char *text)
{
	bool length_matches = true;
	bool text_matches = true;
	for(int i = 0; i < token.length; i++)
	{
		if(text[i] == 0)
		{
			length_matches = false;
			break;
		}
		else if (text[i] != token.text[i])
		{
			text_matches = false;
			break;
		}
	}

	if(text_matches && length_matches)
	{
		if(text[token.length] != 0)
			length_matches = false;
	}

	return (text_matches && length_matches);
}

static bool
func IsDigit(char c)
{
	return (c >= '0' && c <= '9');
}

static bool
func IsHexadecimalDigit(char c)
{
	return (c >= '0' && c <= '9') || 
		   (c >= 'a' && c <= 'z') || 
		   (c >= 'A' && c <= 'Z');
}

static bool
func IsNewLine(char c)
{
	return (c == '\n' || c == '\r');
}

static bool 
func IsWhiteSpace(char c)
{
	return (c == ' ' || c == '\t' || IsNewLine(c));
}

static bool
func IsAlpha(char c)
{
	if(c >= 'a' && c <= 'z')
		return true;
	else if(c >= 'A' && c <= 'Z')
		return true;
	else if(c == '_')
		return true;
	else
		return false;
}

static void
func SkipWhiteSpace(CodePosition *pos)
{
	while(pos->at[0])
	{
		if(IsWhiteSpace(pos->at[0]))
		{
			if(IsNewLine(pos->at[0]))
			{
				pos->row++;
				pos->col = 1;
			}
			else
				pos->col++;
			pos->at++;
		}
		else
			break;
	}
}

static void
func ReadCodeLines(ParseInput *input)
{
	input->lines = ArenaPushArray(&input->arena, 2, CodeLine);
	input->line_n = 2;
	
	size_t row = 1;
	char *at = input->pos->at;
	input->lines[row].string = at;
	input->lines[row].length = 0;
	
	while(*at)
	{
		if(IsNewLine(*at))
		{
			ArenaPushType(&input->arena, CodeLine);
			row++;
			input->lines[row].string = at + 1;
			input->lines[row].length = 0;
			input->line_n++;
		}
		else
			input->lines[row].length++;
		
		at++;
	}
}

static Token
func ReadToken(ParseInput *input)
{
	CodePosition *pos = input->pos;
	SkipWhiteSpace(pos);

	Token token = {};
	token.id = UnknownTokenId;
	token.text = pos->at;
	token.length = 0;
	token.row = pos->row;
	token.col = pos->col;

	if(pos->at[0] == 0)
	{
		token.id = EndOfFileTokenId;
	}
	else if(pos->at[0] == '.')
	{
		token.id = DotTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '{')
	{
		token.id = OpenBracesTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '}')
	{
		token.id = CloseBracesTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '[')
	{
		if(pos->at[1] == '[')
		{
			token.id = OpenBracketsOpenBracketsTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else
		{
			token.id = OpenBracketsTokenId;
			token.length = 1;
			pos->at++;
		}
	}
	else if(pos->at[0] == ']')
	{
		if(pos->at[1] == ']')
		{
			token.id = CloseBracketsCloseBracketsTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else
		{
			token.id = CloseBracketsTokenId;
			token.length = 1;
			pos->at++;
		}
	}
	else if(pos->at[0] == '(')
	{
		token.id = OpenParenTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == ')')
	{
		token.id = CloseParenTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '<')
	{
		if(pos->at[1] == '=')
		{
			token.id = LessThanEqualTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else
		{
			token.id = LessThanTokenId;
			token.length = 1;
			pos->at++;
		}
	}
	else if(pos->at[0] == '>')
	{
		token.id = GreaterThanTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == ',')
	{
		token.id = CommaTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == ':')
	{
		if(pos->at[1] == '=')
		{
			token.id = ColonEqualsTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else if(pos->at[1] == ':')
		{
			token.id = ColonColonTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else
		{
			token.id = ColonTokenId;
			token.length = 1;
			pos->at++;
		}
	}
	else if(pos->at[0] == ';')
	{
		token.id = SemiColonTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '=')
	{
		token.id = EqualsTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '@')
	{
		token.id = AtTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '+')
	{
		if(pos->at[1] == '+')
		{
			token.id = PlusPlusTokenId;
			pos->at += 2;
			token.length = 2;
		}
		else
		{
			token.id = PlusTokenId;
			pos->at++;
			token.length = 1;
		}
	}
	else if(pos->at[0] == '-')
	{
		token.id = MinusTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '*')
	{
		token.id = StarTokenId;
		pos->at++;
		token.length = 1;
	}
	else if(pos->at[0] == '&' && pos->at[1] == '=')
	{
		token.id = AndEqualsTokenId;
		pos->at += 2;
		token.length = 2;
	}
	else if(pos->at[0] == '#')
	{
		pos->at++;
		token.length++;
		while(IsAlpha(pos->at[0]))
		{
			token.length++;
			pos->at++;
		}

		if(TokenEquals(token, "#c_code"))
			token.id = PoundCCodeTokenId;
	}
	else if(IsDigit(pos->at[0]))
	{
		if(pos->at[0] == '0' && pos->at[1] == 'x' && IsHexadecimalDigit(pos->at[2]))
		{
			pos->at += 2;
			token.length = 2;
			while(1)
			{
				if(IsHexadecimalDigit(pos->at[0]))
				{
					token.length++;
					pos->at++;
				}
				else
				{
					break;
				}
			}
			
			token.id = IntegerConstantTokenId;
		}
		else
		{
			while(1)
			{
				if(IsDigit(pos->at[0]))
				{
					token.length++;
					pos->at++;
				}
				else
				{
					break;
				}
			}
			
			if(pos->at[0] == '.')
			{
				pos->at++;
				token.length++;
				while(1)
				{
					if(IsDigit(pos->at[0]))
					{
						token.length++;
						pos->at++;
					
					}
					else
					{
						break;
					}
				}
				
				token.id = FloatConstantTokenId;
			}
			else
			{
				token.id = IntegerConstantTokenId;
			}
		}
	}
	else if(IsAlpha(pos->at[0]))
	{
		while(IsAlpha(pos->at[0]) || IsDigit(pos->at[0]))
		{
			token.length++;
			pos->at++;
		}
		
		if(TokenEquals(token, "extern"))
		{
			token.id = ExternTokenId;
		}
		else if(TokenEquals(token, "false"))
		{
			token.id = FalseTokenId;;
		}
		else if(TokenEquals(token, "for"))
		{
			token.id = ForTokenId;
		}
		else if(TokenEquals(token, "func"))
		{
			token.id = FuncTokenId;
		}
		else if(TokenEquals(token, "if"))
		{
			token.id = IfTokenId;
		}
		else if(TokenEquals(token, "return"))
		{
			token.id = ReturnTokenId;
		}
		else if(TokenEquals(token, "struct"))
		{
			token.id = StructTokenId;
		}
		else if(TokenEquals(token, "to"))
		{
			token.id = ToTokenId;
		}
		else if(TokenEquals(token, "true"))
		{
			token.id = TrueTokenId;
		}
		else
		{
			token.id = NameTokenId;
		}
	}
	else
	{
		while(!IsWhiteSpace(pos->at[0]))
		{
			token.length++;
			pos->at++;
		}
		token.id = UnknownTokenId;
	}
	
	input->pos->col += token.length;
	input->last_token = token;

	return token;
}

static bool
func ReadTokenId(ParseInput *input, TokenId id)
{
	SkipWhiteSpace(input->pos);
	
	CodePosition start_pos = *input->pos;
	
	Token token = ReadToken(input);
	if(token.id != id)
		*input->pos = start_pos;
	
	return (token.id == id);
}

static Token
func ReadTokenUntilClosingBraces(ParseInput *input)
{
	CodePosition *pos = input->pos;
	
	Token token = {};
	token.id = UnknownTokenId;
	token.text = pos->at;
	
	int open_braces_count = 1;
	while(1)
	{
		if(pos->at[0] == '{')
			open_braces_count++;

		if(pos->at[0] == '}')
		{
			open_braces_count--;
			if(open_braces_count == 0)
			{
				pos->at--;
				break;
			}
		}
		
		if(!pos->at[0])
			break;
		
		pos->at++;
		token.length++;
	}
	
	return token;
}

static Token
func PeekToken(ParseInput *input)
{
	CodePosition start_pos = *input->pos;
	Token token = ReadToken(input);
	*input->pos = start_pos;
	
	return token;
}

static bool
func PeekTokenId(ParseInput *input, TokenId id)
{
	Token token = PeekToken(input);
	return (token.id == id);
}

static bool
func PeekTwoTokenIds(ParseInput *input, TokenId id1, TokenId id2)
{
	CodePosition start_pos = *input->pos;
	Token token1 = ReadToken(input);
	Token token2 = ReadToken(input);
	*input->pos = start_pos;

	return (token1.id == id1 && token2.id == id2);	
}

static Var *
func GetVar(VarStack *stack, Token name)
{
	for(size_t i = 0; i < stack->size; i++)
	{
		Var *var = &stack->vars[i];
		if(TokensEqual(var->name, name))
			return var;
	}
	return 0;
}

static bool
func VarExists(VarStack *stack, Token name)
{
	Var *var = GetVar(stack, name);
	return (var != 0);
}

static void
func PushVar(VarStack *stack, Var var)
{
	if(stack->size >= VarStackMaxSize)
	{
		printf("Var stack is full!\n");
		return;
	}
	
	stack->vars[stack->size] = var;
	stack->size++;
}

static bool
func TypesEqual(VarType *type1, VarType *type2)
{
	if(!type1 || !type2)
		return (type1 == type2);
	
	if(type1->id != type2->id)
		return false;
	
	switch(type1->id)
	{
		case BaseTypeId:
		{
			BaseType *base1 = (BaseType *)type1;
			BaseType *base2 = (BaseType *)type2;
			return base1->base_id == base2->base_id;
		}
	}
	
	return true;
}


typedef enum tdef ExpressionId
{
	AddExpressionId,
	ArrayIndexExpressionId,
	BoolConstantExpressionId,
	CastExpressionId,
	DereferenceExpressionId,
	FloatConstantExpressionId,
	FuncCallExpressionId,
	IntegerConstantExpressionId,
	GreaterThanExpressionId,
	LessThanExpressionId,
	LessThanEqualExpressionId,
	MultiplyExpressionId,
	ParenExpressionId,
	StructVarExpressionId,
	SubtractExpressionId,
	VarExpressionId
} ExpressionId;

typedef struct tdef Expression
{
	ExpressionId id;
	VarType *type;
	bool modifiable;
} Expression;

typedef struct tdef AddExpression
{
	Expression e;
	
	Expression *left;
	Expression *right;
} AddExpression;

static AddExpression *
func PushAddExpression(MemoryArena *arena, Expression *left, Expression *right)
{
	AddExpression *e = ArenaPushType(arena, AddExpression);
	e->e.id = AddExpressionId;
	e->e.type = left->type;
	
	e->left = left;
	e->right = right;
	e->e.modifiable = false;
	return e;
}

typedef struct tdef ArrayIndexExpression
{
	Expression e;
	
	Expression *array;
	Expression *index;
} ArrayIndexExpression;

static ArrayIndexExpression *
func PushArrayIndexExpression(MemoryArena *arena, Expression *array, Expression *index)
{
	ArrayIndexExpression *e = ArenaPushType(arena, ArrayIndexExpression);
	e->e.id = ArrayIndexExpressionId;
	e->e.type = 0;
	if(array->type->id == PointerTypeId)
	{
		PointerType *type = (PointerType *)array->type;
		e->e.type = type->pointed_type;
	}
	else if(array->type->id == ArrayTypeId)
	{
		ArrayType *type = (ArrayType *)array->type;
		e->e.type = type->element_type;
	}
	
	e->array = array;
	e->index = index;
	e->e.modifiable = true;
	return e;
}

typedef struct tdef BoolConstantExpression
{
	Expression e;
	
	Token token;
} BoolConstantExpression;

static BoolConstantExpression *
func PushBoolConstantExpression(MemoryArena *arena, Token token, VarType *bool_type)
{
	BoolConstantExpression *e = ArenaPushType(arena, BoolConstantExpression);
	e->e.id = BoolConstantExpressionId;
	e->e.type = bool_type;
	
	e->token = token;
	return e;
}

typedef struct tdef CastExpression
{
	Expression e;
	
	VarType *type;
	Expression *value;
} CastExpression;

static CastExpression *
func PushCastExpression(MemoryArena *arena, VarType *type, Expression *value)
{
	CastExpression *e = ArenaPushType(arena, CastExpression);
	e->e.id = CastExpressionId;
	e->e.type = type;
	e->e.modifiable = value->modifiable;
	
	e->type = type;
	e->value = value;
	return e;
}

typedef struct tdef DereferenceExpression
{
	Expression e;
	
	Expression *pointer;
} DereferenceExpression;

static DereferenceExpression *
func PushDereferenceExpression(MemoryArena *arena, Expression *pointer)
{
	DereferenceExpression *e = ArenaPushType(arena, DereferenceExpression);
	e->e.id = DereferenceExpressionId;
	e->e.type = ((PointerType *)pointer->type)->pointed_type;
	e->e.modifiable = true;
	
	e->pointer = pointer;
	return e;
}

typedef struct tdef GreaterThanExpression
{
	Expression e;
	
	Expression *left;
	Expression *right;
} GreaterThanExpression;

static GreaterThanExpression *
func PushGreaterThanExpression(MemoryArena *arena, Expression *left, Expression *right, VarType *bool_type)
{
	GreaterThanExpression *e = ArenaPushType(arena, GreaterThanExpression);
	e->e.id = GreaterThanExpressionId;
	e->e.type = bool_type;
	
	e->left = left;
	e->right = right;
	e->e.modifiable = false;
	return e;
}

typedef struct tdef FloatConstantExpression
{
	Expression e;
	
	Token token;
} FloatConstantExpression;

static FloatConstantExpression *
func PushFloatConstantExpression(MemoryArena *arena, Token token, VarType *float_type)
{
	FloatConstantExpression *e = ArenaPushType(arena, FloatConstantExpression);
	e->e.id = FloatConstantExpressionId;
	e->e.type = float_type;
	
	e->token = token;
	return e;
}

typedef struct tdef FuncCallExpression
{
	Expression e;
	
	struct FuncDefinition *func_def;
	struct FuncCallArgument *first_call_arg;
} FuncCallExpression;

typedef struct tdef IntegerConstantExpression
{
	Expression e;
	
	Token token;
} IntegerConstantExpression;

static IntegerConstantExpression *
func PushIntegerConstantExpression(MemoryArena *arena, Token token, VarType *int_type)
{
	IntegerConstantExpression *e = ArenaPushType(arena, IntegerConstantExpression);
	e->e.id = IntegerConstantExpressionId;
	e->e.type = int_type;
	
	e->token = token;
	e->e.modifiable = false;
	return e;
}

typedef struct tdef LessThanExpression
{
	Expression e;
	
	Expression *left;
	Expression *right;
} LessThanExpression;

static LessThanExpression *
func PushLessThanExpression(MemoryArena *arena, Expression *left, Expression *right, VarType *bool_type)
{
	LessThanExpression *e = ArenaPushType(arena, LessThanExpression);
	e->e.id = LessThanExpressionId;
	e->e.type = bool_type;
	
	e->left = left;
	e->right = right;
	e->e.modifiable = false;
	return e;
}

typedef struct tdef LessThanEqualExpression
{
	Expression e;
	
	Expression *left;
	Expression *right;
} LessThanEqualExpression;

static LessThanEqualExpression *
func PushLessThanEqualExpression(MemoryArena *arena, Expression *left, Expression *right, VarType *bool_type)
{
	LessThanEqualExpression *e = ArenaPushType(arena, LessThanEqualExpression);
	e->e.id = LessThanEqualExpressionId;
	e->e.type = bool_type;
	
	e->left = left;
	e->right = right;
	e->e.modifiable = false;
	return e;
}

typedef struct tdef MultiplyExpression
{
	Expression e;
	
	Expression *left;
	Expression *right;
} MultiplyExpression;

static MultiplyExpression *
func PushMultiplyExpression(MemoryArena *arena, Expression *left, Expression *right)
{
	MultiplyExpression *e = ArenaPushType(arena, MultiplyExpression);
	e->e.id = MultiplyExpressionId;
	e->e.type = left->type;
	
	e->left = left;
	e->right = right;
	e->e.modifiable = false;
	return e;
}

typedef struct tdef ParenExpression
{
	Expression e;
	
	Expression *in;
} ParenExpression;

static ParenExpression *
func PushParenExpression(MemoryArena *arena, Expression *in)
{
	ParenExpression *e = ArenaPushType(arena, ParenExpression);
	e->e.id = ParenExpressionId;
	e->e.type = in->type;
	
	e->in = in;
	return e;
}

typedef struct tdef StructVarExpression
{
	Expression e;
	
	Expression *base;
	Token var_name;
} StructVarExpression;

typedef struct tdef SubtractExpression
{
	Expression e;
	
	Expression *left;
	Expression *right;
} SubtractExpression;

static SubtractExpression *
func PushSubtractExpression(MemoryArena *arena, Expression *left, Expression *right)
{
	SubtractExpression *e = ArenaPushType(arena, SubtractExpression);
	e->e.id = SubtractExpressionId;
	e->e.type = left->type;
	
	e->left = left;
	e->right = right;
	e->e.modifiable = false;
	return e;
}

typedef enum tdef DefinitionId
{
	CCodeDefinitionId,
	ExternFuncDefinitionId,
	FuncDefinitionId,
	StructDefinitionId
} DefinitionId;

typedef struct tdef Definition
{
	DefinitionId id;
} Definition;

typedef struct tdef FuncParam
{
	struct FuncParam *next;
	Token name;
	VarType *type;
} FuncParam;

typedef struct tdef FuncHeader
{
	Token name;
	
	FuncParam *first_param;
	VarType *return_type;
} FuncHeader;

typedef struct tdef FuncDefinition
{
	Definition def;
	
	struct FuncDefinition *next;
	
	FuncHeader header;
	struct BlockInstruction *body;
} FuncDefinition;

static FuncDefinition *
func GetFuncDefinition(ParseInput *input, Token name)
{
	for(FuncDefinition *f = input->first_func_definition; f; f = f->next)
	{
		if(TokensEqual(f->header.name, name))
		{
			return f;
		}
	}
	
	return 0;
}

typedef struct tdef FuncCallArgument
{
	struct FuncCallArgument *next;
	
	Expression *arg;
} FuncCallArgument;

static FuncCallExpression *
func PushFuncCallExpression(MemoryArena *arena, FuncDefinition *func_def, FuncCallArgument *first_call_arg)
{
	FuncCallExpression *e = ArenaPushType(arena, FuncCallExpression);
	e->e.id = FuncCallExpressionId;
	e->e.type = func_def->header.return_type;
	
	e->func_def = func_def;
	e->first_call_arg = first_call_arg;
	return e;
}

typedef struct tdef StructVar
{
	Token name;
	VarType *type;
	struct StructVar *next;
} StructVar;

typedef struct tdef StructDefinition
{
	Definition def;
	
	struct StructDefinition *next;

	Token name;
	StructVar *first_var;
} StructDefinition;

static StructVarExpression *
func PushStructVarExpression(MemoryArena *arena, Expression *base, StructVar *var)
{
	StructVarExpression *e = ArenaPushType(arena, StructVarExpression);
	e->e.id = StructVarExpressionId;
	
	e->e.type = var->type;
	e->base = base;
	e->var_name = var->name;
	e->e.modifiable = true;
	return e;
}

typedef struct tdef VarExpression
{
	Expression e;
	
	Var var;
} VarExpression;

static VarExpression *
func PushVarExpression(MemoryArena *arena, Var var)
{
	VarExpression *e = ArenaPushType(arena, VarExpression);
	e->e.id = VarExpressionId;
	e->e.type = var.type;
	
	e->var = var;
	e->e.modifiable = true;
	return e;
}

static StructVar *
func GetStructVar(StructDefinition *def, Token name)
{
	for(StructVar *var = def->first_var; var; var = var->next)
	{
		if(TokensEqual(var->name, name))
		{
			return var;
		}
	}
	
	return 0;
}

static Expression *decl ReadExpression(ParseInput *);
static VarType *decl ReadVarType(ParseInput *);

static FuncCallArgument *
func PushFuncCallArgument(MemoryArena *arena, Expression *expression)
{
	FuncCallArgument *arg = ArenaPushType(arena, FuncCallArgument);
	arg->next = 0;
	arg->arg = expression;
	return arg;
}

static Expression *
func ReadNumberLevelExpression(ParseInput *input)
{
	Expression *e = 0;
	
	VarType *type = ReadVarType(input);
	if(type)
	{
		if(!ReadTokenId(input, ColonColonTokenId))
		{
			SetError(input, "Expected '::' after variable type.");
			return 0;
		}
		
		Expression *value = ReadNumberLevelExpression(input);
		if(!value)
		{
			return 0;
		}

		e = (Expression *)PushCastExpression(&input->arena, type, value);
	}
	else if(ReadTokenId(input, IntegerConstantTokenId))
	{
		e = (Expression *)PushIntegerConstantExpression(&input->arena, input->last_token, input->int_type);
	}
	else if(ReadTokenId(input, FloatConstantTokenId))
	{
		e = (Expression *)PushFloatConstantExpression(&input->arena, input->last_token, input->float_type);
	}
	else if(ReadTokenId(input, FalseTokenId) || ReadTokenId(input, TrueTokenId))
	{
		e = (Expression *)PushBoolConstantExpression(&input->arena, input->last_token, input->bool_type);
	}
	else if(ReadTokenId(input, OpenParenTokenId))
	{
		Expression *in = ReadExpression(input);
		if(!in)
		{
			SetError(input, "Expected expression after '('.");
			return 0;
		}
		
		if(!ReadTokenId(input, CloseParenTokenId))
		{
			SetError(input, "Expected ')'.");
			return 0;
		}
		
		e = (Expression *)PushParenExpression(&input->arena, in);
	}
	else if(ReadTokenId(input, NameTokenId))
	{
		bool ok = false;
		Token name = input->last_token;
		
		if(!ok)
		{
			Var *var = GetVar(&input->var_stack, name);
			if(var)
			{
				e = (Expression *)PushVarExpression(&input->arena, *var);
				ok = true;
			}
		}
		
		if(!ok)
		{
			FuncDefinition *f = GetFuncDefinition(input, name);
			if(f)
			{
				if(!ReadTokenId(input, OpenParenTokenId))
				{
					SetError(input, "Expected '(' for function call!");
					return 0;
				}
				
				FuncCallArgument *first_call_arg = 0;
				FuncCallArgument *last_call_arg = 0;
				
				FuncHeader *header = &f->header;
				for(FuncParam *param = header->first_param; param; param = param->next)
				{
					if(last_call_arg != 0)
					{
						if(!ReadTokenId(input, CommaTokenId))
						{
							SetError(input, "Expected ',' between function call arguments.");
							return 0;
						}
					}
					
					Expression *arg = ReadExpression(input);
					if(!arg)
					{
						SetError(input, "Expected function call argument.");
						return 0;
					}
					
					if(!TypesEqual(arg->type, param->type))
					{
						SetError(input, "Types do not match for function call.");
						WriteErrorMessageVarType("Need: ", param->type);
						WriteErrorMessageVarType("Got:  ", arg->type);
						return 0;
					}
					
					FuncCallArgument *call_arg = PushFuncCallArgument(&input->arena, arg);
					if(!first_call_arg)
					{
						first_call_arg = call_arg;
						last_call_arg = call_arg;
					}
					else
					{
						last_call_arg->next = call_arg;
						last_call_arg = call_arg;
					}
				}
				
				if(!ReadTokenId(input, CloseParenTokenId))
				{
					SetError(input, "Expected ')' for function call!");
					return 0;
				}
				
				e = (Expression *)PushFuncCallExpression(&input->arena, f, first_call_arg);
				ok = true;
			}
		}
		
		if(!ok)
		{
			SetError(input, "Unexpected token!");
			return 0;
		}
	}
	else if(ReadTokenId(input, OpenBracketsOpenBracketsTokenId))
	{
		Expression *p = ReadExpression(input);
		if(p->type->id != PointerTypeId)
		{
			SetError(input, "Dereferencing non-pointer expression!");
			return 0;
		}
		
		e = (Expression *)PushDereferenceExpression(&input->arena, p);
		
		if(!ReadTokenId(input, CloseBracketsCloseBracketsTokenId))
		{
			SetError(input, "Expected ']]' for dereference expression.");
			return 0;
		}
	}
	else
	{
		SetError(input, "Unknown expression.");
		return 0;
	}
	
	while(1)
	{
		if(ReadTokenId(input, OpenBracketsTokenId))
		{
			if(!e || !e->type || (e->type->id != PointerTypeId && e->type->id != ArrayTypeId))
			{
				SetError(input, "Cannot index non-array expression.");
				return 0;
			}
			
			Expression *index = ReadExpression(input);
			
			if(!index)
			{
				SetError(input, "Expected array index expression.");
				return 0;
			}
			
			if(!index->type || !IsIntegerType(index->type))
			{
				SetError(input, "Array index is not integer.");
				return 0;
			}
			
			if(!ReadTokenId(input, CloseBracketsTokenId))
			{
				SetError(input, "Missing ']'.");
				return 0;
			}
			
			e = (Expression *)PushArrayIndexExpression(&input->arena, e, index);
		}
		else if(ReadTokenId(input, DotTokenId))
		{
			if(!e || !e->type)
			{
				SetError(input, "Expected expression before '.'.");
				return 0;
			}
			
			StructType *struct_type = 0;
			if(e->type->id == StructTypeId)
			{
				struct_type = (StructType *)e->type;
			}
			else if(e->type->id == PointerTypeId)
			{
				VarType *base_type = ((PointerType *)e->type)->pointed_type;
				if(base_type->id != StructTypeId)
				{
					SetError(input, "Non-struct pointer expression before '.'");
					return 0;
				}
				struct_type = (StructType *)base_type;
			}
			else
			{
				SetError(input, "Non-struct expression before '.'.");
				return 0;
			}
			
			StructDefinition *def = struct_type->def;
			if(!ReadTokenId(input, NameTokenId))
			{
				SetError(input, "Expected struct variable name after '.'.");
				return 0;
			}
			
			Token var_name = input->last_token;
			StructVar *var = GetStructVar(def, var_name);
			if(!var)
			{
				SetErrorToken(input, "Unknown struct variable name.", var_name);
				return 0;
			}
			
			e = (Expression *)PushStructVarExpression(&input->arena, e, var);
		}
		else
		{
			break;
		}
	}
	
	return e;
}

static Expression *
func ReadBitLevelExpression(ParseInput *input)
{
	Expression *e = ReadNumberLevelExpression(input);
	return e;
}

static Expression *
func ReadProductLevelExpression(ParseInput *input)
{
	Expression *e = ReadBitLevelExpression(input);
	while(1)
	{
		if(ReadTokenId(input, StarTokenId))
		{
			Expression *left = e;
			Expression *right = ReadBitLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '*'.");
				return 0;
			}
			if(!TypesEqual(left->type, right->type))
			{
				SetError(input, "Types do not match for '*'.");
				return 0;
			}
			
			e = (Expression *)PushMultiplyExpression(&input->arena, left, right);
		}
		else
		{
			break;
		}
	}
	return e;
}

static bool
func CanSubtractType(VarType *type)
{
	if(type->id == BaseTypeId)
	{
		BaseType *t = (BaseType *)type;
		switch(t->base_id)
		{
			case Int32BaseTypeId:
			case Float32BaseTypeId:
			case UInt32BaseTypeId:
			{
				return true;
			}
		}
	}
	
	return false;
}

static Expression *
func ReadSumLevelExpression(ParseInput *input)
{
	Expression *e = ReadProductLevelExpression(input);
	while(1)
	{
		if(ReadTokenId(input, PlusTokenId))
		{
			Expression *left = e;
			Expression *right = ReadProductLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '+'.");
				return 0;
			}
			if(!TypesEqual(left->type, right->type))
			{
				SetError(input, "Types do not match for '+'.");
				WriteErrorMessageVarType("Left:  ", left->type);
				WriteErrorMessageVarType("Right: ", right->type);
				return 0;
			}
			
			e = (Expression *)PushAddExpression(&input->arena, left, right);
		}
		else if(ReadTokenId(input, MinusTokenId))
		{
			Expression *left = e;
			Expression *right = ReadProductLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '-'.");
				return 0;
			}
			if(!TypesEqual(left->type, right->type))
			{
				SetError(input, "Types do not match for '-'.");
				WriteErrorMessageVarType("Left:  ", left->type);
				WriteErrorMessageVarType("Right: ", right->type);
				return 0;
			}
			if(!CanSubtractType(left->type))
			{
				SetError(input, "Cannot use '-' on type.");
				WriteErrorMessageVarType("Type: ", left->type);
				return 0;
			}
			
			e = (Expression *)PushSubtractExpression(&input->arena, left, right);
		}
		else
		{
			break;
		}
	}
	return e;
}

static Expression *
func ReadCompareLevelExpression(ParseInput *input)
{
	Expression *e = ReadSumLevelExpression(input);
	while(1)
	{
		if(ReadTokenId(input, LessThanTokenId))
		{
			Expression *right = ReadSumLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '<'.");
				return 0;
			}
			if(!TypesEqual(e->type, right->type))
			{
				SetError(input, "Types do not match for '<'.");
				return 0;
			}
			
			e = (Expression *)PushLessThanExpression(&input->arena, e, right, input->bool_type);
		}
		else if(ReadTokenId(input, LessThanEqualTokenId))
		{
			Expression *right = ReadSumLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '<='.");
				return 0;
			}
			if(!TypesEqual(e->type, right->type))
			{
				SetError(input, "Types do not match for '<='.");
				return 0;
			}
			
			e = (Expression *)PushLessThanEqualExpression(&input->arena, e, right, input->bool_type);
		}
		else if(ReadTokenId(input, GreaterThanTokenId))
		{
			Expression *right = ReadSumLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '>'.");
				return 0;
			}
			if(!TypesEqual(e->type, right->type))
			{
				SetError(input, "Types do not match for '>'.");
				return 0;
			}
			
			e = (Expression *)PushGreaterThanExpression(&input->arena, e, right, input->bool_type);
		}
		else
		{
			break;
		}
	}
	
	return e;
}

static Expression *
func ReadExpression(ParseInput *input)
{
	Expression *e = ReadCompareLevelExpression(input);
	return e;
}

typedef struct tdef StackState
{
	int var_stack_size;
} StackState;

static StackState
func GetStackState(ParseInput *input)
{
	StackState state = {};
	state.var_stack_size = input->var_stack.size;
	return state;
}

static void
func SetStackState(ParseInput *input, StackState state)
{
	input->var_stack.size = state.var_stack_size;
}

typedef enum tdef InstructionId
{
	AndEqualsInstructionId,
	AssignInstructionId,
	BlockInstructionId,
	CreateVariableInstructionId,
	FuncCallInstructionId,
	IfInstructionId,
	IncrementInstructionId,
	ForInstructionId,
	ReturnInstructionId
} InstructionId;

typedef struct tdef Instruction
{
	InstructionId id;
	struct Instruction *next;
} Instruction;

typedef struct tdef AndEqualsInstruction
{
	Instruction i;
	
	Expression *left;
	Expression *right;
} AndEqualsInstruction;

typedef struct tdef BlockInstruction
{
	Instruction i;

	Instruction *first;
} BlockInstruction;

static BlockInstruction * decl ReadBlock(ParseInput *);

typedef struct tdef AssignInstruction
{
	Instruction i;
	
	Expression *left;
	Expression *right;
} AssignInstruction;

typedef struct tdef CreateVariableInstruction
{
	Instruction i;
	
	Token name;
	VarType *type;
	Expression *init;
} CreateVariableInstruction;

typedef struct tdef IfInstruction
{
	Instruction i;
	
	Expression *condition;
	BlockInstruction *body;
} IfInstruction;

static IfInstruction *
func ReadIfInstruction(ParseInput *input)
{
	StackState stack_state = GetStackState(input);
	
	ReadTokenId(input, IfTokenId);
	Expression *condition = ReadExpression(input);
	if(!condition || !condition->type || !IsBoolType(condition->type))
	{
		SetError(input, "Expected bool expression after 'if'.");
		return 0;
	}
	
	BlockInstruction *body = ReadBlock(input);
	
	IfInstruction *i = ArenaPushType(&input->arena, IfInstruction);
	i->i.id = IfInstructionId;
	i->i.next = 0;
	
	i->condition = condition;
	i->body = body;
	
	SetStackState(input, stack_state);
	return i;
}

typedef struct tdef IncrementInstruction
{
	Instruction i;
	
	Expression *value;
} IncrementInstruction;

typedef struct tdef ForInstruction
{
	Instruction i;
	
	Instruction *init;
	Expression *condition;
	Instruction *update;
	
	BlockInstruction *body;
} ForInstruction;

static bool
func IsValidInitInstruction(Instruction *instruction)
{
	if(!instruction)
		return false;
	
	switch(instruction->id)
	{
		case BlockInstructionId:
		case IfInstructionId:
		case ForInstructionId:
		case ReturnInstructionId:
			return false;
	}
	
	return true;
}

static bool
func IsValidUpdateInstruction(Instruction *instruction)
{
	if(!instruction)
		return true;
	
	switch(instruction->id)
	{
		case BlockInstructionId:
		case IfInstructionId:
		case ForInstructionId:
		case ReturnInstructionId:
		case CreateVariableInstructionId:
			return false;		
	}
	
	return true;
}

static Instruction * decl ReadInstruction(ParseInput *);

static ForInstruction *
func ReadForInstruction(ParseInput *input)
{
	StackState stack_state = GetStackState(input);
	
	ReadTokenId(input, ForTokenId);
	
	Instruction *init = ReadInstruction(input);
	if(!IsValidInitInstruction(init))
	{
		SetError(input, "Invalid init instruction after 'for'.");
		return 0;
	}
	
	if(!ReadTokenId(input, SemiColonTokenId))
	{
		SetError(input, "Expected ';' after init instruction for 'for' loop.");
		return 0;
	}
	
	Expression *condition = ReadExpression(input);
	if(!condition)
	{
		SetError(input, "Expected condition for 'for' loop.");
		return 0;
	}
	else if(condition->type != input->bool_type)
	{
		SetError(input, "Condition for 'for' loop not boolean.");
		return 0;
	}
	
	if(!ReadTokenId(input, SemiColonTokenId))
	{
		SetError(input, "Expected ';' after condition for 'for' loop.");
		return 0;
	}
	
	Instruction *update = 0;
	if(!PeekTokenId(input, OpenBracesTokenId))
		update = ReadInstruction(input);
	
	if(!IsValidUpdateInstruction(update))
	{
		SetError(input, "Invalid update instruction for 'for' loop.");
		return 0;
	}
	
	ForInstruction *i = ArenaPushType(&input->arena, ForInstruction);
	i->i.id = ForInstructionId;
	i->init = init;
	i->condition = condition;
	i->update = update;
	
	i->body = ReadBlock(input);
		
	SetStackState(input, stack_state);
	return i;
}

typedef struct tdef FuncCallInstruction
{
	Instruction i;
	
	FuncCallExpression *e;
} FuncCallInstruction;

typedef struct tdef ReturnInstruction
{
	Instruction i;
	
	Expression *value;
} ReturnInstruction;

typedef struct tdef DefinitionList
{
	Definition *definition;
	struct DefinitionList *next;
} DefinitionList;

typedef DefinitionList tdef DefinitionListElem;

typedef struct tdef CCodeDefinition
{
	Definition def;
	
	Token code;
} CCodeDefinition;

static CCodeDefinition *
func ReadCCodeDefinition(ParseInput *input)
{
	CCodeDefinition *def = ArenaPushType(&input->arena, CCodeDefinition);
	def->def.id = CCodeDefinitionId;
	
	ReadTokenId(input, PoundCCodeTokenId);
	
	if(!ReadTokenId(input, OpenBracesTokenId))
	{
		SetError(input, "Expected '{' after '#c_code'!");
		return 0;
	}
	
	Token c_code_token = ReadTokenUntilClosingBraces(input);
	c_code_token.id = CCodeTokenId;
	def->code = c_code_token;

	if(!ReadTokenId(input, CloseBracesTokenId))
	{
		SetError(input, "No matching '}' after '#c_code'!");
		return 0;
	}
	
	return def;
}

static ArrayType *
func PushArrayType(MemoryArena *arena, Expression *size, VarType *element_type)
{
	ArrayType *type = ArenaPushType(arena, ArrayType);
	type->type.id = ArrayTypeId;
	type->size = size;
	type->element_type = element_type;
	return type;
}

static PointerType *
func PushPointerType(MemoryArena *arena, VarType *pointed_type)
{
	PointerType *type = ArenaPushType(arena, PointerType);
	type->type.id = PointerTypeId;
	type->pointed_type = pointed_type;
	return type;
}

static StructType *
func PushStructType(MemoryArena *arena, StructDefinition *def)
{
	StructType *type = ArenaPushType(arena, StructType);
	type->type.id = StructTypeId;
	type->def = def;
	return type;
}

static StructDefinition *
func GetStructDefinition(ParseInput *input, Token name)
{
	for(StructDefinition *def = input->first_struct_definition; def; def = def->next)
	{
		if(TokensEqual(def->name, name))
		{
			return def;
		}
	}
	
	return 0;
}

static VarType *
func ReadVarType(ParseInput *input)
{
	CodePosition start_pos = *input->pos;
	
	VarType *type = 0;
	
	if(ReadTokenId(input, AtTokenId))
	{
		VarType *pointed_type = ReadVarType(input);
		if(!pointed_type)
		{
			SetError(input, "Expected variable type after '@'.");
			return 0;
		}
		
		type = (VarType *)PushPointerType(&input->arena, pointed_type);
	}
	else if(ReadTokenId(input, OpenBracketsTokenId))
	{
		Expression *size = ReadExpression(input);
		if(!size)
		{
			SetError(input, "Expected array size expression.");
			return 0;
		}
		
		if(size->modifiable)
		{
			SetError(input, "Array size has to be a constant.");
			return 0;
		}
		
		if(!ReadTokenId(input, CloseBracketsTokenId))
		{
			SetError(input, "Expected ']' after array size.");
			return 0;
		}
		
		VarType *element_type = ReadVarType(input);
		if(!element_type)
		{
			SetError(input, "Expected array element type.");
			return 0;
		}
		
		type = (VarType *)PushArrayType(&input->arena, size, element_type);
	}
	else if(ReadTokenId(input, NameTokenId))
	{
		if(TokenEquals(input->last_token, "int"))
		{
			type = input->int_type;
		}
		else if(TokenEquals(input->last_token, "uint"))
		{
			type = input->uint_type;
		}
		else if(TokenEquals(input->last_token, "float"))
		{
			type = input->float_type;
		}
		else if(TokenEquals(input->last_token, "bool"))
		{
			type = input->bool_type;
		}
		else
		{			
			StructDefinition *def = GetStructDefinition(input, input->last_token);
			if(def)
			{
				type = (VarType *)PushStructType(&input->arena, def);
			}
		}
	}
	
	if(!type)
	{
		*input->pos = start_pos;
	}
	
	return type;
}

typedef struct tdef NameList
{
	size_t size;
	Token *names;
} NameList;

static NameList
func ReadNameList(ParseInput *input)
{
	NameList list = {};
	list.size = 0;
	list.names = ArenaPushArray(&input->arena, 0, Token);
	while(1)
	{
		if(ReadTokenId(input, NameTokenId))
		{
			Token *token = ArenaPushType(&input->arena, Token);
			*token = input->last_token;
			list.size++;
		}
		else
		{
			break;
		}
		
		if(!ReadTokenId(input, CommaTokenId))
		{
			break;
		}
	}
	
	return list;
}

static bool
func NeedsSemicolon(InstructionId id)
{
	switch(id)
	{
		case BlockInstructionId: 
		case IfInstructionId:
		case ForInstructionId:
		{
			return false;
		}
	}
	
	return true;
}

static ReturnInstruction *decl ReadReturnInstruction(ParseInput *);

static Instruction *
func ReadInstruction(ParseInput *input)
{
	Instruction *instruction = 0;
	if(PeekTokenId(input, ForTokenId))
	{
		instruction = (Instruction *)ReadForInstruction(input);
	}
	else if(PeekTokenId(input, IfTokenId))
	{
		instruction = (Instruction *)ReadIfInstruction(input);
	}
	else if(PeekTokenId(input, ReturnTokenId))
	{
		instruction = (Instruction *)ReadReturnInstruction(input);
	}
	else if(PeekTwoTokenIds(input, NameTokenId, ColonTokenId))
	{
		Token var_name = ReadToken(input);
		if(VarExists(&input->var_stack, var_name))
		{
			SetError(input, "Variable already exists.");
			return 0;
		}
		
		ReadTokenId(input, ColonTokenId);
		
		VarType *type = ReadVarType(input);
		if(!type)
		{
			SetError(input, "Expected variable type.");
			return 0;
		}
		
		CreateVariableInstruction *cv = ArenaPushType(&input->arena, CreateVariableInstruction);
		cv->i.id = CreateVariableInstructionId;
		cv->i.next = 0;
		cv->name = var_name;
		cv->init = 0;
		cv->type = type;
		instruction = (Instruction *)cv;
		
		Var var = {};
		var.name = var_name;
		var.type = type;
		PushVar(&input->var_stack, var);
	}
	else if(PeekTwoTokenIds(input, NameTokenId, ColonEqualsTokenId))
	{
		Token var_name = ReadToken(input);
		if(VarExists(&input->var_stack, var_name))
		{
			SetError(input, "Variable already exists.");
			return 0;
		}

		ReadTokenId(input, ColonEqualsTokenId);
		
		Expression *init = ReadExpression(input);
		if(!init)
		{
			SetError(input, "Expected expression after ':='.");
			return 0;
		}
		
		if(!init->type)
		{
			SetError(input, "Expression for variable initialization doesn't have type.");
			return 0;
		}
		
		CreateVariableInstruction *cv = ArenaPushType(&input->arena, CreateVariableInstruction);
		cv->i.id = CreateVariableInstructionId;
		cv->i.next = 0;
		cv->name = var_name;
		cv->init = init;
		cv->type = init->type;
		instruction = (Instruction *)cv;
		
		Var var = {};
		var.name = var_name;
		var.type = init->type;
		PushVar(&input->var_stack, var);
	}
	else
	{
		Expression *expression = ReadExpression(input);
		if(!expression)
		{
			return 0;	
		}
		
		if(expression->id == FuncCallExpressionId)
		{
			FuncCallInstruction *i = ArenaPushType(&input->arena, FuncCallInstruction);
			i->i.id = FuncCallInstructionId;
			
			i->e = (FuncCallExpression *)expression;
			instruction = (Instruction *)i;
		}
		else if(ReadTokenId(input, EqualsTokenId))
		{
			Expression *right = ReadExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '='.");
				return 0;
			}
			
			if(!TypesEqual(expression->type, right->type))
			{
				SetError(input, "Unmatching types for '='.");
				return 0;
			}
			
			AssignInstruction *i = ArenaPushType(&input->arena, AssignInstruction);
			i->i.id = AssignInstructionId;
			
			i->left = expression;
			i->right = right;
			instruction = (Instruction *)i;
		}
		else if(ReadTokenId(input, PlusPlusTokenId))
		{
			if(!expression->modifiable)
			{
				SetError(input, "Cannot increment a non-modifiable expression.");
				return 0;
			}
			
			bool can_increment = false;
			can_increment |= TypesEqual(expression->type, input->int_type);
			can_increment |= TypesEqual(expression->type, input->uint_type);
			can_increment |= expression->type->id == PointerTypeId;
			
			if(!can_increment)
			{
				SetError(input, "Invalid type for '++'.");
				return 0;
			}
			
			IncrementInstruction *i = ArenaPushType(&input->arena, IncrementInstruction);
			i->i.id = IncrementInstructionId;
			
			i->value = expression;
			instruction = (Instruction *)i;
		}
		else if(ReadTokenId(input, AndEqualsTokenId))
		{
			if(!expression->modifiable)
			{
				SetError(input, "Cannot apply '&=' to a non-modifiable expression.");
				return 0;
			}
			
			Expression *right = ReadExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '&='.");
				return 0;
			}
			
			if(!TypesEqual(expression->type, right->type))
			{
				SetError(input, "Unmatching types for '&='.");
				return 0;
			}
			
			AndEqualsInstruction *i = ArenaPushType(&input->arena, AndEqualsInstruction);
			i->i.id = AndEqualsInstructionId;
			
			i->left = expression;
			i->right = right;
			instruction = (Instruction *)i;
		}
		else
		{
			SetError(input, "Unexpected expression.");
			return 0;
		}
	}
	return instruction;
}

static BlockInstruction *
func ReadBlock(ParseInput *input)
{
	BlockInstruction *block = 0;
	if(!ReadTokenId(input, OpenBracesTokenId))
	{
		SetError(input, "Expected '{'");
		return 0;
	}
	
	StackState stack_state = GetStackState(input);
	
	Instruction *first_instruction = 0;
	Instruction *last_instruction = 0;
	while(1)
	{
		if(ReadTokenId(input, CloseBracesTokenId))
			break;
		
		Instruction *instruction = ReadInstruction(input);
		
		if(!instruction)
			break;
		
		if(NeedsSemicolon(instruction->id))
		{
			if(!ReadTokenId(input, SemiColonTokenId))
			{
				SetError(input, "Expected ';' after instruction.");
				return 0;
			}
		}
		
		if(!first_instruction)
		{
			first_instruction = instruction;
			last_instruction = instruction;
			last_instruction->next = 0;
		}
		else
		{
			last_instruction->next = instruction;
			last_instruction = instruction;
			last_instruction->next = 0;
		}
	}
	
	block = ArenaPushType(&input->arena, BlockInstruction);
	block->i.id = BlockInstructionId;
	block->i.next = 0;
	block->first = first_instruction;
	
	SetStackState(input, stack_state);
	
	return block;
}

static ReturnInstruction *
func ReadReturnInstruction(ParseInput *input)
{
	ReadTokenId(input, ReturnTokenId);
	
	Expression *value = 0;
	if(!PeekTokenId(input, SemiColonTokenId))
		value = ReadExpression(input);
	
	if(!input->func_definition)
	{
		SetError(input, "Found 'return' outside of function definition!");
		return 0;
	}
	else
	{
		if(!TypesEqual(input->func_definition->header.return_type, value->type))
		{
			SetError(input, "Found 'return' with invalid type");
			return 0;
		}
	}
	
	ReturnInstruction *i = ArenaPushType(&input->arena, ReturnInstruction);
	i->i.id = ReturnInstructionId;
	
	i->value = value;
	return i;
}

static FuncHeader
func ReadFuncHeader(ParseInput *input)
{
	Token name = ReadToken(input);
	if(name.id != NameTokenId)
	{
		SetErrorToken(input, "Invalid function name!", name);
	}
	
	if(!ReadTokenId(input, OpenParenTokenId))
	{
		SetError(input, "Expected '(' after function name!");
	}
	
	FuncParam *first_param = 0;
	FuncParam *last_param = 0;
	while(1)
	{
		if(ReadTokenId(input, CloseParenTokenId))
		{
			break;
		}
		
		if(first_param)
		{
			if(!ReadTokenId(input, CommaTokenId))
			{
				SetError(input, "Expected ',' between function parameters!");
			}
		}
		
		NameList name_list = ReadNameList(input);
		if(name_list.size == 0)
		{
			SetError(input, "Expected ',' or ')' after function parameter.");
			break;
		}
		if(!ReadTokenId(input, ColonTokenId))
		{
			SetError(input, "Expected ':' after function parameter name.");
			break;
		}
		
		VarType *param_type = ReadVarType(input);
		for(size_t i = 0; i < name_list.size; i++)
		{
			Token param_name = name_list.names[i];
			if(VarExists(&input->var_stack, param_name))
			{
				SetErrorToken(input, "Variable already exists, cannot be used as a function parameter ", param_name);
				break;
			}
			
			FuncParam *param = ArenaPushType(&input->arena, FuncParam);
			param->name = param_name;
			param->type = param_type;
			param->next = 0;
			if(last_param)
			{
				last_param->next = param;
				last_param = param;
			}
			else
			{
				first_param = param;
				last_param = param;
			}
			
			Var var = {};
			var.name = param_name;
			var.type = param_type;
			PushVar(&input->var_stack, var);
		}
	}
	
	VarType *return_type = ReadVarType(input);
	
	FuncHeader header = {};
	header.name = name;
	header.first_param = first_param;
	header.return_type = return_type;
	
	return header;
}

static FuncDefinition *
func ReadFuncDefinition(ParseInput *input)
{
	StackState stack_state = GetStackState(input);
	
	ReadTokenId(input, FuncTokenId);	
	
	FuncDefinition *prev_func_definition = input->func_definition;
	
	FuncDefinition *def = ArenaPushType(&input->arena, FuncDefinition);
	def->def.id = FuncDefinitionId;
	
	FuncHeader header = ReadFuncHeader(input);
	if(input->any_error)
	{
		return 0;
	}
	def->header = header;
	
	input->func_definition = def;
	BlockInstruction *body = ReadBlock(input);
	
	if(!body)
	{
		SetError(input, "Function doesn't have a body!");
		return 0;
	}
	
	def->body = body;
	
	input->func_definition = prev_func_definition;
	
	def->next = input->first_func_definition;
	input->first_func_definition = def;
	
	SetStackState(input, stack_state);
	
	return def;
}

static bool
func HasStruct(ParseInput *input, Token name)
{
	for(StructDefinition *def = input->first_struct_definition; def; def = def->next)
	{
		if(TokensEqual(def->name, name))
		{
			return true;
		}
	}
	
	return false;
}

static StructDefinition *
func ReadStructDefinition(ParseInput *input)
{
	ReadTokenId(input, StructTokenId);
	StructDefinition *def = ArenaPushType(&input->arena, StructDefinition);
	def->def.id = StructDefinitionId;
	
	Token name = ReadToken(input);
	
	if(HasStruct(input, name))
	{
		SetErrorToken(input, "Struct already exists!", name);
		return 0;
	}
	
	if(name.id != NameTokenId)
	{
		SetErrorToken(input, "Invalid struct name!", name);
		return 0;
	}
	
	if(!ReadTokenId(input, OpenBracesTokenId))
	{
		SetError(input, "Expected '{'");
		return 0;
	}
	
	StructVar *first_var = 0;
	StructVar *last_var = 0;
	while(1)
	{
		if(ReadTokenId(input, CloseBracesTokenId))
		{
			break;
		}
	
		NameList name_list = ReadNameList(input);
		if(name_list.size == 0)
		{
			SetError(input, "Expected struct variable name.");
			break;
		}
		
		if(!ReadTokenId(input, ColonTokenId))
		{
			SetError(input, "Expected ':' after struct variable name.");
			return 0;
		}
		
		VarType *type = ReadVarType(input);
		if(type == 0)
		{
			return 0;
		}
		
		for(size_t i = 0; i < name_list.size; i++)
		{
			Token name = name_list.names[i];
			StructVar *var = ArenaPushType(&input->arena, StructVar);
			var->name = name;
			var->type = type;
			var->next = 0;
			if(last_var)
			{
				last_var->next = var;
				last_var = var;
			}
			else
			{
				first_var = var;
				last_var = var;
			}
		}
		
		if(!ReadTokenId(input, SemiColonTokenId))
		{
			SetError(input, "Expected ';' after struct variable definition.");
			return 0;
		}
	}
	
	def->name = name;
	def->first_var = first_var;
	
	def->next = input->first_struct_definition;
	input->first_struct_definition = def;
	
	return def;
}

typedef struct tdef ExternFuncDefinition
{
	Definition def;
	
	Token name;
} ExternFuncDefinition;

static ExternFuncDefinition *
func ReadExternFuncDefinition(ParseInput *input)
{
	ReadTokenId(input, ExternTokenId);
	
	if(!ReadTokenId(input, FuncTokenId))
	{
		SetError(input, "Expected 'func' after 'extern'.");
		return 0;
	}
	
	Token name = ReadToken(input);
	if(name.id != NameTokenId)
	{
		SetError(input, "Expected name for extern function.");
		return 0;
	}
	
	if(!ReadTokenId(input, SemiColonTokenId))
	{
		SetError(input, "Expected ';' after extern function definition.");
		return 0;
	}
	
	ExternFuncDefinition *def = ArenaPushType(&input->arena, ExternFuncDefinition);
	def->def.id = ExternFuncDefinitionId;
	
	def->name = name;
	
	return def;
}

static Definition *
func ReadDefinition(ParseInput *input)
{
	Definition *def = 0;
	Token token = PeekToken(input);
	if(token.id == PoundCCodeTokenId)
		def = (Definition *)ReadCCodeDefinition(input);
	else if(token.id == FuncTokenId)
		def = (Definition *)ReadFuncDefinition(input);
	else if(token.id == ExternTokenId)
		def = (Definition *)ReadExternFuncDefinition(input);
	else if(token.id == StructTokenId)
		def = (Definition *)ReadStructDefinition(input);
	else
	{
		SetErrorToken(input, "Expected definition instead of ", token);
		ReadToken(input);
	}
	
	return def;
}

static DefinitionList *
func ReadDefinitionList(ParseInput *input)
{
	DefinitionListElem *first_elem = 0;
	DefinitionListElem *last_elem = 0;
	while(1)
	{
		if(PeekTokenId(input, EndOfFileTokenId))
			break;
		
		Definition *definition = ReadDefinition(input);
		
		if(definition)
		{
			DefinitionListElem *elem = ArenaPushType(&input->arena, DefinitionListElem);
			elem->next = 0;
			elem->definition = definition;
		
			if(!last_elem)
			{
				first_elem = elem;
				last_elem = elem;
			}
			else
			{
				last_elem->next = elem;
				last_elem = elem;
			}
		}
	}

	return first_elem;
}

static void
func WriteErrorVarType(VarType *type)
{
	switch(type->id)
	{
		case NoTypeId:
		{
			printf("No type");
			break;
		}
		case ArrayTypeId:
		{
			ArrayType *t = (ArrayType *)type;
			printf("[]");
			WriteErrorVarType(t->element_type);
			break;
		}
		case BaseTypeId:
		{
			BaseType *t = (BaseType *)type;
			switch(t->base_id)
			{
				case BoolBaseTypeId:
				{
					printf("bool");
					break;
				}
				case Int32BaseTypeId:
				{
					printf("int");
					break;
				}
				case Float32BaseTypeId:
				{
					printf("float");
					break;
				}
				case UInt32BaseTypeId:
				{
					printf("uint");
					break;
				}
				default:
				{
					printf("unknown_base_type_%i", (int)t->base_id);
					break;
				}
			}
			break;
		}
		case PointerTypeId:
		{
			PointerType *t = (PointerType *)type;
			printf("@");
			WriteErrorVarType(t->pointed_type);
			break;
		}
		case StructTypeId:
		{
			StructType *t = (StructType *)type;
			printf("struct %.*s", t->def->name.length, t->def->name.text);
			break;
		}
		default:
		{
			printf("unknown_type_%i", (int)type->id);
		}
	}
}

#include "WriteC.h"
#include "WriteFormatted.h"
#include "WriteX64.h"

int main(int arg_n, char **arg_v)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	
	if(arg_n != 3)
	{
		printf("Usage: M64.exe [m64_input_file] [c_output_file]\n");
		return -1;
	}

	FILE *in = fopen(arg_v[1], "r");
	if(!in)
	{
		printf("Cannot open file <%s>\n", arg_v[1]);
		return -1;
	}

	FILE *out = fopen(arg_v[2], "w");
	if(!out)
	{
		printf("Cannot create file to write to <%s>\n", arg_v[2]);
		return -1;
	}

	char *buffer = ReadFileToMemory(in);
	
	CodePosition pos = {};
	pos.at = buffer;
	pos.row = 1;
	pos.col = 1;
	ParseInput input = {};
	input.pos = &pos;
	
	input.arena = CreateArena((size_t)64 * 1024 * 1024);
	
	input.var_stack.vars = ArenaPushArray(&input.arena, VarStackMaxSize, Var);
	input.var_stack.size = 0;
	
	BaseType *bool_base = ArenaPushType(&input.arena, BaseType);
	bool_base->type.id = BaseTypeId;
	bool_base->base_id = BoolBaseTypeId;
	input.bool_type = (VarType *)bool_base;

	BaseType *int_base = ArenaPushType(&input.arena, BaseType);
	int_base->type.id = BaseTypeId;
	int_base->base_id = Int32BaseTypeId;
	input.int_type = (VarType *)int_base;
	
	BaseType *float_base = ArenaPushType(&input.arena, BaseType);
	float_base->type.id = BaseTypeId;
	float_base->base_id = Float32BaseTypeId;
	input.float_type = (VarType *)float_base;
	
	BaseType *uint_base = ArenaPushType(&input.arena, BaseType);
	uint_base->type.id = BaseTypeId;
	uint_base->base_id = UInt32BaseTypeId;
	input.uint_type = (VarType *)uint_base;
	
	ReadCodeLines(&input);
	
	DefinitionList *def_list = ReadDefinitionList(&input);
	
	if(input.any_error)
	{
		return -1;
	}
#if 0
	X64Output output = {};
	output.arena = CreateArena((size_t)64 * 1024);
	X64WriteDefinitionList(&output, def_list);
	
	for(size_t i = 0; i < output.arena.used_size; i++)
		fprintf(out, "%c", output.arena.memory[i]);
#endif
	Output output = {};
	output.arena = CreateArena((size_t)64 * 1024);
	output.tabs = 0;
	WriteDefinitionList(&output, def_list);
	if(output.error)
	{
		return -1;
	}
	
	for(size_t i = 0; i < output.arena.used_size; i++)
	{
		fprintf(out, "%c", output.arena.memory[i]);
	}
	
	return 0;
}
