#include <Windows.h>
#include <stdio.h>

#define Assert(condition) {if(!(condition)) DebugBreak();}
#define func
#define decl
#define tdef

struct tdef MemArena
{
	char *memory;
	int max_size;
	int used_size;
};

static MemArena 
func CreateArena(char *memory, int size)
{
	Assert(memory);
	Assert(size > 0);
	MemArena arena = {};
	arena.memory = memory;
	arena.max_size = size;
	arena.used_size = 0;
	return arena;
}

#define CreateStaticArena(name, size) char name##_buffer[size]; MemArena name = CreateArena(name##_buffer, size);

static char * 
func ArenaPush(MemArena *arena, int size)
{
	char *memory = arena->memory + arena->used_size;
	arena->used_size += size;
	Assert(arena->used_size <= arena->max_size);
	return memory;
}

#define ArenaPushType(arena, type) (type *)ArenaPush(arena, sizeof(type))
#define ArenaPushArray(arena, count, type) (type *)ArenaPush(arena, (count) * sizeof(type))

#define MaxCodeSize 1024 * 1024
static char global_code_buffer[MaxCodeSize];

struct tdef CodePosition
{
	char *at;
	int row;
	int col;
};

enum tdef TokenId
{
	NoTokenId,
	EndOfFileTokenId,
	IntegerConstantTokenId,
	RealConstantTokenId,
	OpenParenTokenId,
	CloseParenTokenId,
	OpenBracesTokenId,
	CloseBracesTokenId,
	OpenBracketsTokenId,
	CloseBracketsTokenId,
	ColonEqualsTokenId,
	ColonTokenId,
	SemiColonTokenId,
	PlusPlusTokenId,
	PlusEqualsTokenId,
	MinusEqualsTokenId,
	MinusMinusTokenId,
	StarEqualsTokenId,
	PlusTokenId,
	LessThanTokenId,
	LessThanOrEqualToTokenId,
	GreaterThanTokenId,
	GreaterThanOrEqualToTokenId,
	GreaterThanGreaterThanTokenId,
	LessThanLessThanTokenId,
	EqualsTokenId,
	EqualsEqualsTokenId,
	AtTokenId,
	CommaTokenId,
	ForTokenId,
	InTokenId,
	ToTokenId,
	FuncTokenId,
	NameTokenId,
	StructTokenId,
	StarTokenId,
	SlashTokenId,
	MinusTokenId,
	ReturnTokenId,
	DotTokenId,
	OrTokenId,
	OrOrTokenId,
	AndTokenId,
	AndAndTokenId,
	OrEqualsTokenId,
	QuestionMarkTokenId,
	IfTokenId,
	ElseTokenId,
	TrueTokenId,
	FalseTokenId,
	ExclamationMarkTokenId,
	ExclamationMarkEqualsTokenId,
	UseTokenId,
	PoundTokenId,
	WhileTokenId,
	BreakTokenId,
	ContinueTokenId,
	DeclTokenId,
	OperatorTokenId,
	EnumTokenId,
	ConstructorTokenId,
	NamespaceTokenId,
	CharacterConstantTokenId,
	StringConstantTokenId
};

struct tdef Token
{
	TokenId id;
	char *text;
	int length;
	int row;
	int col;
};

enum tdef VarTypeId
{
	NoTypeId,
	BaseTypeId,
	PointerTypeId,
	StructTypeId,
	ArrayTypeId,
	EnumTypeId
};

struct tdef VarType
{
	VarTypeId id;
};

enum tdef BaseVarTypeId
{
	NoBaseTypeId,
	Int8BaseTypeId,
	Int32BaseTypeId,
	UInt8BaseTypeId,
	UInt32BaseTypeId,
	Real32BaseTypeId,
	Bool32BaseTypeId
};

struct tdef BaseType
{
	VarType type;

	BaseVarTypeId base_id;
};

struct tdef PointerType
{
	VarType type;

	VarType *pointed_type;
};

struct tdef StructVar
{
	StructVar *next;
	VarType *type;
	Token name;
};

struct decl Expression;
struct tdef StructMetaVar
{
	StructMetaVar *next;
	Expression *expression;
	Token name;
};

struct tdef Struct
{
	Token name;

	Token namespace_name;
	StructVar *first_var;
	StructMetaVar *first_meta_var;
};

struct decl Expression;
// TODO: use [size1, ..., sizen] for multi-dimensional arrays
struct tdef ArrayType
{
	VarType type;

	Expression *size;
	// TODO: remove this, allow non-const size expression as long as array is passed through pointer
	Token meta_size_var_name;
	VarType *element_type;
};

struct tdef FuncParam
{
	FuncParam *next;
	Token name;
	VarType *type;
};

struct decl BlockInstruction;

struct tdef FuncHeader
{
	Token name;

	Token namespace_name;
	FuncParam *first_param;
	VarType *return_type;
};

struct tdef Func
{
	FuncHeader header;
	BlockInstruction* body;
};

#define FuncStackMaxSize 256
struct tdef FuncStack
{
	Func **funcs;
	int size;
};

struct tdef Operator
{
	Token left_name;
	Token right_name;
	VarType *left_type;
	VarType *right_type;
	Token token;
	VarType *return_type;
	BlockInstruction *body;
};

#define OperatorStackMaxSize 64
struct tdef OperatorStack
{
	Operator **operators;
	int size;
};

struct tdef Constructor
{
	VarType *type;
	FuncHeader header;
	BlockInstruction *body;
};

#define ConstructorStackMaxSize 64
struct tdef ConstructorStack
{
	Constructor **constructors;
	int size;
};

struct tdef Enum
{
	Token name;

	Token namespace_name;
	Token *tokens;
	int size;
};

struct tdef EnumMember
{
	Enum *e;
	int index;
};

#define EnumStackMaxSize 64
struct tdef EnumStack
{
	Enum **enums;
	int size;
};

static bool decl TypesEqual(VarType *type1, VarType* type2);

static bool
func IsValidOperatorTokenId(TokenId token_id)
{
	bool is_valid = false;
	switch(token_id)
	{
		case PlusTokenId:
		case MinusTokenId:
		case StarTokenId:
		case EqualsEqualsTokenId:
		{
			is_valid = true;
		}
	}

	return is_valid;
}

static Operator *
func GetOperator(OperatorStack *stack, VarType *left_type, VarType *right_type, TokenId token_id)
{
	Assert(left_type != 0 && right_type != 0);
	Assert(IsValidOperatorTokenId(token_id));

	Operator *result = 0;
	for(int i = 0; i < stack->size; i++)
	{
		Operator *op = stack->operators[i];
		if(TypesEqual(op->left_type, left_type) && TypesEqual(op->right_type, right_type) && op->token.id == token_id)
		{
			result = op;
			break;
		}
	}
	return result;
}

static void
func PushOperator(OperatorStack *stack, Operator *op)
{
	Assert(stack->size < OperatorStackMaxSize);
	stack->operators[stack->size] = op;
	stack->size++;
}

static void
func PushConstructor(ConstructorStack *stack, Constructor *ctor)
{
	Assert(stack->size < ConstructorStackMaxSize);
	stack->constructors[stack->size] = ctor;
	stack->size++;
}

static bool 
func TokensEqual(Token token1, Token token2)
{
	bool equal = false;
	if(token1.id != token2.id)
	{
		equal = false;
	}
	else if(token1.length != token2.length)
	{
		equal = false;
	}
	else
	{
		equal = true;
		for(int i = 0; i < token1.length; i++)
		{
			if(token1.text[i] != token2.text[i])
			{
				equal = false;
				break;
			}
		}
	}
	return equal;
}

/*
func GetEnumMember(stack:@EnumStack, name:Token):EnumMember
{
	result:EnumMember;
	for e := @in stack.enums
	{
		for index := [] in e.members
		{
			if TokensEqual(e.members[index], name)
			{
				result.e = e;
				result.index = index;
				break;
			}
		}

		if result.e
		{
			break;
		}
	}
	return result;
}
*/

static EnumMember
func GetEnumMember(EnumStack *stack, Token name)
{
	EnumMember result = {};
	for(int i = 0; i < stack->size; i++)
	{
		Enum *e = stack->enums[i];
		for(int index = 0; index < e->size; index++)
		{
			if(TokensEqual(e->tokens[index], name))
			{
				result.e = e;
				result.index = index;
				break;
			}
		}
		
		if(result.e)
		{
			break;
		}
	}

	return result;
}

static bool
func EnumMemberExists(EnumStack *stack, Token name)
{
	EnumMember member = GetEnumMember(stack, name);
	bool exists = (member.e != 0);
	return exists;
}

static Enum *
func GetEnum(EnumStack *stack, Token name)
{
	Enum *result = 0;
	for(int i = 0; i < stack->size; i++)
	{
		Enum *e = stack->enums[i];
		if(TokensEqual(e->name, name))
		{
			result = e;
			break;
		}
	}
	return result;
}

static void
func PushEnum(EnumStack *stack, Enum *e)
{
	Assert(stack->size < EnumStackMaxSize);
	stack->enums[stack->size] = e;
	stack->size++;
}

/*
struct Var
{
	type:@VarType;
	name:Token;
}

#VarStackMaxSize := 64;
struct VarStack
{
	vars:@[#size]Var;
	size:Int32;
}
*/

struct decl Expression;
struct decl CreateVariableInstruction;
struct tdef Var
{
	VarType *type;
	Token name;
	Expression *use_from;
	bool has_final_type;

	CreateVariableInstruction *create_instruction;
};

#define VarStackMaxSize 64
struct tdef VarStack
{
	Var *vars;
	int size;
};

struct decl CreateMetaVariableInstruction;
struct tdef MetaVar
{
	VarType *type;
	Token name;
	Expression *expression;
	Expression *use_from;
	CreateMetaVariableInstruction *create_instruction;
};

#define MetaVarStackMaxSize 64
struct tdef MetaVarStack
{
	MetaVar *vars;
	int size;
};

enum tdef ExpressionId
{
	NoExpressionId,

	AddExpressionId,
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
	ProductExpressionId,
	RealConstantExpressionId,
	RightShiftExpressionId,
	StringConstantExpressionId,
	StructVarExpressionId,
	StructDefVarExpressionId,
	StructMetaVarExpressionId,
	SubtractExpressionId,
	TernaryOperatorExpressionId,
	VarExpressionId,
};

struct tdef Expression
{
	ExpressionId id;
	bool is_const;
	bool has_final_type;
	VarType *type;
};

struct tdef IntegerConstantExpression
{
	Expression expr;

	Token token;
};

struct tdef CharacterConstantExpression
{
	Expression expr;

	Token token;
};

struct tdef RealConstantExpression
{
	Expression expr;

	Token token;
};

struct tdef VarExpression
{
	Expression expr;

	Var var;
};

struct tdef MetaVarExpression
{
	Expression expr;

	MetaVar var;
};

struct tdef LessThanExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef GreaterThanExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef EqualExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef NotEqualExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef GreaterThanOrEqualToExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef LessThanOrEqualToExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef DereferenceExpression
{
	Expression expr;

	Expression *base;
};

struct tdef ArrayIndexExpression
{
	Expression expr;

	Expression *array;
	Expression *index;
};

struct tdef ProductExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef DivideExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef ParenExpression
{
	Expression expr;

	Expression *in;
};

struct tdef AddExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef SubtractExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef StructVarExpression
{
	Expression expr;

	Expression *struct_expression;
	Token var_name;
};

struct tdef StructDefVarExpression
{
	Expression expr;

	Struct *struct_def;
	Token var_name;
};

struct tdef StructMetaVarExpression
{
	Expression expr;

	Expression *struct_expression;
	Expression *meta_expression;
	Token var_name;
};

struct tdef FuncCallParam
{
	FuncCallParam *next;

	Expression *expression;
};

struct tdef FuncCallExpression
{
	Expression expr;

	Func *f;
	FuncCallParam *first_param;
};

struct tdef CastExpression
{
	Expression expr;

	VarType *type;
	Expression *expression;
};

struct tdef LeftShiftExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef RightShiftExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef TernaryOperatorExpression
{
	Expression expr;

	Expression *condition;
	Expression *left;
	Expression *right;
};

struct tdef AddressExpression
{
	Expression expr;

	Expression *addressed;
};

struct tdef OrExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef AndExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef InvertExpression
{
	Expression expr;

	Expression *inverted;
};

struct tdef BoolConstantExpression
{
	Expression expr;

	bool value;
};

struct tdef NegateExpression
{
	Expression expr;

	Expression *negated;
};

struct tdef BitAndExpression
{
	Expression expr;

	Expression *left;
	Expression *right;
};

struct tdef OperatorCallExpression
{
	Expression expr;

	Operator *op;
	Expression *left;
	Expression *right;
};

struct tdef EnumMemberExpression
{
	Expression expr;

	EnumMember member;
};

struct tdef ConstructorCallExpression
{
	Expression expr;

	Constructor *ctor;
	FuncCallParam *first_param;
};

struct tdef StringConstantExpression
{
	Expression expr;

	Token token;
};

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
	{
		return true;
	}
	else if(c >= 'A' && c <= 'Z')
	{
		return true;
	}
	else if(c == '_')
	{
		return true;
	}
	else
	{
		return false;
	}
}

static bool 
func IsDigit(char c)
{
	return (c >= '0' && c <= '9');
}

static bool 
func IsBinaryDigit(char c)
{
	return (c == '0' || c == '1');
}

static bool 
func IsOctalDigit(char c)
{
	return (c >= '0' && c <= '7');
}

static bool 
func IsHexadecimalDigit(char c)
{
	return (IsDigit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
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
		{
			length_matches = false;
		}
	}

	return (text_matches && length_matches);
}

static void 
func SkipWhiteSpaceAndComments(CodePosition *pos)
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
			{
				pos->col++;
			}
			pos->at++;
		}
		else if(pos->at[0] == '/' && pos->at[1] == '/')
		{
			pos->at += 2;
			pos->col += 2;
			while(pos->at[0] && !IsNewLine(pos->at[0]))
			{
				pos->at++;
				pos->col++;
			}
		}
		else if(pos->at[0] == '/' && pos->at[1] == '*')
		{
			int comment_level = 1;

			pos->at++;
			pos->col++;
			while(1)
			{
				if(pos->at[0] == '*' && pos->at[1] == '/')
				{
					pos->at += 2;
					pos->col += 2;

					comment_level--;
					Assert(comment_level >= 0);
					if(comment_level == 0)
					{
						break;
					}
				}
				else if(pos->at[0] == '/' && pos->at[1] == '*')
				{
					pos->at += 2;
					pos->col += 2;

					comment_level++;
				}
				else if(pos->at[0] == 0)
				{
					break;
				}
				else
				{
					if(IsNewLine(pos->at[0]))
					{
						pos->row++;
						pos->col = 1;
					}
					else
					{
						pos->col++;
					}

					pos->at++;
				}
			}
		}
		else
		{
			break;
		}
	}
}

/*
enum BaseTypeId
{
	No,
	Int32,
	UInt32,
	Bool32
}

typeenum VarType
{
	No,
	BaseType(base_id:BaseVarTypeId),
	PointerType(pointed_type:@VarType),
	StructType(name:Token, first_var:@StructVar, first_meta_var:@StructMetaVar),
	ArrayType(size:Token, element_type:@VarType)
}

func WriteType(output:@Output, type:@VarType)
{
	Assert(type != 0);
	switch(type)
	{
		case BaseType:
		{
			switch(type.base)
			{
				case Int32:
				{
					WriteString(output, "int");
				}
				case UInt32:
				{
					WriteString(output, "unsigned int");
				}
				case Real32:
				{
					WriteString(output, "float");
				}
				case Bool32:
				{
					WriteString(output, "bool");
				}
				default:
				{
					DebugBreak();
				}
			}
		}
		case PointerType:
		{
			WriteType(output, type.pointed_type);
			WriteString(output, "*");
		}
		case StructType:
		{
			WriteToken(output, type.str.name);
		}
		case ArrayType:
		{
			WriteString(output, "(");
			WriteType(output, type.element_type);
			WriteString(output, ")[");
			WriteToken(output, type.size);
			WriteString(output, "]");
		}
		default:
		{
			DebugBreak();
		}
	}
}

func ReadVarType(input:@ParseInput):@VarType
{
	use input;

	type:@VarType;

	if ReadTokenType(input, AtTokenId)
	{
		pointed_type := ReadVarType(input);
		if !pointed_type
		{
			SetError(input, ExpectedVarTypeErrorId);
		}

		if !error
		{
			type = PushPointerType(arena, pointed_type);
		}
	}
	else if ReadTokenType(input, OpenBracketsTokenId)
	{
		if !ReadTokenType(input, IntegerConstantTokenId)
		{
			SetError(input, InvalidArraySizeErrorId);
		}

		size := last_token;

		if !ReadTokenType(input, CloseBracketsTokenId)
		{
			SetError(input, NoCloseBracketsErrorId);
		}

		element_type := ReadVarType(input);
		
		if !error
		{
			type = PushArrayType(arena, size, element_type);
		}
	}
	else if ReadTokenType(input, NameTokenId)
	{
		#def CheckType(base_type_id: BaseTypeId)
		(
			if TokenEquals(last_token, #string(base_type_id))
			{
				type = PushBaseType(arena, base_type_id);
			}
		)

		CheckType(Int32);
		CheckType(UInt32);
		CheckType(Real32);
		CheckType(Bool32);
		
		if !type
		{
			str := GetStruct(@struct_stack, last_token);
			if str
			{
				type = PushStructType(arena, str);
			}
		}
	}

	if !type
	{
		SetError(input, InvalidTypeErrorId);
	}

	return type;
}

typeenum Expression 
	#typename(name) := "Expression" + name;
	#enumname(name) := typename(name) + "Id";
{
	#All:
	{
		#enumid
		type:@VarType;
		is_const:Bool32;
		has_final_type:Bool32;
	}
	No,
	IntegerConstant(value:Token),
	RealConstant(value:Token),
	Name(value:Token),
	Var(name:Token),
	LessThan(expr1, expr2 :@Expression),
	GreaterThan(expr1, expr2 :@Expression),
	LessThanOrEqualTo(expr1, expr2 :@Expression)
	Dereference(expr:@Expression),
	ArrayIndex(array:@Expression, index:@Expression),
	Product(expr1, expr2 :@Expression),
	Divide(expr1, expr2 :@Expresion),
	Paren(expr:@Expression),
	Add(expr1, expr2 :@Expression),
	Subtract(expr1, expr2: @Expression),
	StructVar(str: @Expression, name:Token),
	StructMetaVar(str: @Expression, name:Token),
	FuncCall(name:Token, param_list:@FuncCallParamList),
	Cast(type:@VarType, expr:@Expression),
	LeftShift(expr1, expr2 :@Expression),
	RightShift(expr1, expr2: :@Expression),
	TernaryOperator(condition, expr1, expr2 :@Expression),
	Address(expr:@Expression),
	Or(expr1, expr2 :@Expression),
	Invert(expr:@Expression),
	BoolConstant(value:Token),
	Negate(expr:@Expression),
	BitAnd(expr1, expr2: @Expression)
};

func WriteExpression(output:@Output, expression:@Expression)
{
	#command Write str:#String
	(
		for token: #Parse(str)
		{
			#typeswitch token
			{
				case Int32, UInt32, Real32, Bool32, Token, @Expression
				{
					Write(output, token);
				}
				case #String
				{
					Write(output, token.string);
				}
				default
				{
					DebugBreak();
				}
			}
		}
	)

	typeswitch use expression
	{
		case IntegerConstant
		{
			#Write "{value}";
		}
		case RealConstant
		{
			
			#Write "{value}f";
		}
		case Name
		{
			#Write "{name}";
		}
		case Var
		{
			#Write "{name}";
		}
		case LessThan
		{
			#Write "{left} < {right}";
		}
		case GreaterThan
		{
			#Write "{left} > {right}";
		}
		case LessThanOrEqualTo
		{
			#Write "{left} <= {right}";
		}
		case Dereference
		{
			#Write "(*{expr})";
		}
		case Add
		{
			#Write "{left} + {right}";
		}
		case Subtract
		{
			Write "{left} - {right}";
		}
		case Product
		{
			#Write "{left} * {right}";
		}
		case Divide
		{
			#Write "{left} / {right}";
		}
		case Paren
		{
			#Write "({expr})";
		}
		case StructVar
		{
			#Write "{expr}.{name}";
		}
		case FuncCall
		{
			param := func.first_param;
			call_param = first_param;

			#write "{func.name}(");

			is_first_param := true;
			while param
			{
				if !is_first_param
				{
					#Write ", ";
				}
				is_first_param = false;

				Assert(call_param != 0 && call_param.expression != 0);
				Assert(TypesEqual(param.type, call_param.type));

				WriteExpression(output, call_param.expression);

				param = param.next;
				call_param = call_param.next;
			}

			#Write ")";
		}
		case Cast
		{
			#Write "({type})({expr})";
		}
		case LeftShift
		{
			#Write "{expr1} << {expr2}";
		}
		case RightShift
		{
			#Write "{expr1} >> {expr2}";
		}
		case TernaryOperator
		{
			#Write "{condition} ? {expr1} : {expr2}";
		}
		case Address
		{
			#Write "&{expr}";
		}
		case Or
		{
			#Write "{expr1} || {expr2}";
		}
		case Invert
		{
			#Write "-{expr}";
		}
		case BoolConstant
		{
			#Write "{value}";
		}
		case Negate
		{
			#Write "!{expr}";
		}
		case BitAnd
		{
			#Write "{expr1} & {expr2}";
		}
		case ArrayIndex
		{
			#Write "({array})[{index}]";
		}
	}
}
*/

static StructVar *
func GetStructVar(Struct *str, Token name)
{
	StructVar *result = 0;
	StructVar *var = str->first_var;
	while(var)
	{
		if(TokensEqual(var->name, name))
		{
			result = var;
			break;
		}

		var = var->next;
	}
	return result;
}

static bool 
func StructHasVar(Struct *str, Token name)
{
	StructVar *var = GetStructVar(str, name);
	bool has_var = (var != 0);
	return has_var;
}

static StructMetaVar *
func GetStructMetaVar(Struct *str, Token name)
{
	StructMetaVar *result = 0;
	StructMetaVar *var = str->first_meta_var;
	while(var)
	{
		if(TokensEqual(var->name, name))
		{
			result = var;
			break;
		}

		var = var->next;
	}
	return result;
}

static bool 
func StructHasMetaVar(Struct *str, Token name)
{
	StructMetaVar *var = GetStructMetaVar(str, name);
	bool has_var = (var != 0);
	return has_var;
}

struct tdef StructType
{
	VarType type;

	Struct *str;
};

struct tdef EnumType
{
	VarType type;

	Enum *e;
};

static bool 
func TypesEqual(VarType *type1, VarType* type2)
{
	bool equal = false;
	Assert(type1 != 0 && type2 != 0);
	if(type1->id == type2->id)
	{
		switch(type1->id)
		{
			case BaseTypeId:
			{
				BaseType *base_type1 = (BaseType *)type1;
				BaseType *base_type2 = (BaseType *)type2;
				equal = (base_type1->base_id == base_type2->base_id);
				break;
			}
			case PointerTypeId:
			{
				PointerType *pointer_type1 = (PointerType *)type1;
				PointerType *pointer_type2 = (PointerType *)type2;
				equal = (TypesEqual(pointer_type1->pointed_type, pointer_type2->pointed_type));
				break;
			}
			case StructTypeId:
			{
				StructType *struct_type1 = (StructType *)type1;
				StructType *struct_type2 = (StructType *)type2;
				equal = (struct_type1->str == struct_type2->str);
				break;
			}
			case EnumTypeId:
			{
				EnumType *enum_type1 = (EnumType *)type1;
				EnumType *enum_type2 = (EnumType *)type2;
				equal = (enum_type1->e == enum_type2->e);
				break;
			}
			default:
			{
				DebugBreak();
			}
		}
	}
	return equal;
}

static bool 
func IsNumericalType(VarType *type)
{
	bool is_numerical = false;
	if(type != 0 && type->id == BaseTypeId)
	{
		BaseType *base_type = (BaseType *)type;
		switch(base_type->base_id)
		{
			case Bool32BaseTypeId:
			case Int8BaseTypeId:
			case Int32BaseTypeId:
			case UInt8BaseTypeId:
			case UInt32BaseTypeId:
			case Real32BaseTypeId:
			{
				is_numerical = true;
				break;
			}
			default:
			{
				is_numerical = false;
			}
		}
	}
	return is_numerical;
}

static int 
func GetNumericalTypePriority(VarType *type)
{
	int priority = 0;
	Assert(IsNumericalType(type));
	Assert(type->id == BaseTypeId);
	BaseType *base_type = (BaseType *)type;
	switch(base_type->base_id)
	{
		case Real32BaseTypeId:
		{
			priority = 1;
			break;
		}
		case Int8BaseTypeId:
		{
			priority = 2;
			break;
		}
		case UInt8BaseTypeId:
		{
			priority = 2;
			break;
		}
		case Int32BaseTypeId:
		{
			priority = 3;
			break;
		}
		case UInt32BaseTypeId:
		{
			priority = 4;
			break;
		}
		case Bool32BaseTypeId:
		{
			priority = 5;
			break;
		}
		default:
		{
			DebugBreak();
		}
	}
	Assert(priority > 0);
	return priority;
}

/*
func IsPointerType(Type:@VarType) :Bool32
{
	is_pointer := (type && type.id == Pointer);
	return is_pointer;
}
*/

static bool 
func IsPointerType(VarType *type)
{
	bool is_pointer = false;
	if(type != 0)
	{
		is_pointer = (type->id == PointerTypeId);
	}
	return is_pointer;
}

/*
func IsIntegerType(type:@VarType) :Bool32
{
	is_integer := false;

	#typeif type == Base
	{
		switch type.base_id
		{
			case UInt32, Int32
			{
				is_integer = true;
			}
			default
			{
				is_integer = false;
			}
		}
	}

	return is_integer;
}
*/

static bool
func IsBoolType(VarType *type)
{
	bool is_bool = false;
	if(type && type->id == BaseTypeId)
	{
		BaseType *base_type = (BaseType *)type;
		if(base_type->base_id == Bool32BaseTypeId)
		{
			is_bool = true;
		}
	}
	
	return is_bool;
}

static bool
func IsRealType(VarType *type)
{
	bool is_real_type = false;
	if(type && type->id == BaseTypeId)
	{
		BaseType *base_type = (BaseType *)type;
		if(base_type->base_id == Real32BaseTypeId)
		{
			is_real_type = true;
		}
	}

	return is_real_type;
}

static bool
func IsIntegerType(VarType *type)
{
	bool is_integer = false;
	if(type && type->id == BaseTypeId)
	{
		BaseType *base_type = (BaseType *)type;
		switch(base_type->base_id)
		{
			case Int8BaseTypeId:
			case Int32BaseTypeId:
			case UInt8BaseTypeId:
			case UInt32BaseTypeId:
			{
				is_integer = true;
				break;
			}
			default:
			{
				is_integer = false;
				break;
			}
		}
	}

	return is_integer;
}

/*
func GetVar(var_stack:@VarStack, name:Token) :@Var
{
	Assert(name.id == Name);
	result:@Var;
	for var: @in var_stack
	{
		if TokensEqual(var.name, name)
		{
			result = var;
			break;
		}
	}

	return result;
}
*/

static Var *
func GetVar(VarStack *var_stack, Token name)
{
	Assert(name.id == NameTokenId);
	Var *result = 0;
	for(int i = 0; i < var_stack->size; i++)
	{
		Var *var = &var_stack->vars[i];
		if(TokensEqual(var->name, name))
		{
			result = var;
			break;
		}
	}
	return result;
}

static bool
func VarExists(VarStack *var_stack, Token name)
{
	Var *var = GetVar(var_stack, name);
	bool exists = (var != 0);
	return exists;
}

static void
func PushVar(VarStack *var_stack, Var var)
{
	Assert(var.name.id == NameTokenId);
	Assert(var.type != 0);
	Assert(!VarExists(var_stack, var.name));
	Assert(var_stack->size < VarStackMaxSize);
	var_stack->vars[var_stack->size] = var;
	var_stack->size++;
}

static MetaVar *
func GetMetaVar(MetaVarStack *stack, Token name)
{
	Assert(name.id == NameTokenId);
	MetaVar *result = 0;
	for(int i = 0; i < stack->size; i++)
	{
		MetaVar *var = &stack->vars[i];
		if(TokensEqual(var->name, name))
		{
			result = var;
			break;
		}
	}
	return result;
}

static bool
func MetaVarExists(MetaVarStack *stack, Token name)
{
	MetaVar *var = GetMetaVar(stack, name);
	bool exists = (var != 0);
	return exists;
}

static void
func PushMetaVar(MetaVarStack *stack, MetaVar var)
{
	Assert(var.name.id == NameTokenId);
	Assert(var.type != 0);
	Assert(!MetaVarExists(stack, var.name));
	Assert(stack->size < MetaVarStackMaxSize);
	stack->vars[stack->size] = var;
	stack->size++;
}

#define StructStackMaxSize 64
struct tdef StructStack
{
	Struct **structs;
	int size;
};

static Struct *
func GetStruct(StructStack *stack, Token name)
{
	Struct *result = 0;
	for(int i = 0; i < stack->size; i++)
	{
		Struct *str = stack->structs[i];
		if(TokensEqual(str->name, name))
		{
			result = str;
			break;
		}
	}
	return result;
}

static bool
func HasStruct(StructStack *stack, Token name)
{
	Struct *str = GetStruct(stack, name);
	bool has_struct = (str != 0);
	return has_struct;
}

static void
func PushStruct(StructStack *stack, Struct *str)
{
	Assert(!HasStruct(stack, str->name));
	Assert(stack->size < StructStackMaxSize);
	stack->structs[stack->size] = str;
	stack->size++;
}

static Func *
func GetFunc(FuncStack *stack, Token name)
{
	Func *result = 0;
	for(int i = 0; i < stack->size; i++)
	{
		Func *f = stack->funcs[i];
		if(TokensEqual(f->header.name, name))
		{
			result = f;
			break;
		}
	}
	return result;
}

static bool
func FuncExists(FuncStack *stack, Token name)
{
	Func *f = GetFunc(stack, name);
	bool exists = (f != 0);
	return exists;
}

static void
func PushFunc(FuncStack *stack, Func *f)
{
	Assert(f != 0);
	Assert(!FuncExists(stack, f->header.name));
	
	Assert(stack->size < FuncStackMaxSize);
	stack->funcs[stack->size] = f;
	stack->size++;
}

struct tdef CodeLine
{
	char *string;
	int length;
};

struct tdef ParseInput
{
	int line_n;
	CodeLine *lines;
	CodePosition *pos;
	MemArena *arena;
	Token last_token;
	VarStack var_stack;
	MetaVarStack meta_var_stack;
	StructStack struct_stack;
	FuncStack func_stack;
	OperatorStack operator_stack;
	EnumStack enum_stack;
	ConstructorStack constructor_stack;
	MemArena *token_name_arena;
	Token namespace_name;
	Struct *in_struct_definition;
};

static void
func ReadCodeLines(ParseInput *input)
{
	input->lines = ArenaPushArray(input->arena, 2, CodeLine);
	input->line_n = 2;

	int row = 1;
	char *at = input->pos->at;
	input->lines[row].string = at;
	input->lines[row].length = 0;
		
	while(*at)
	{
		if(IsNewLine(*at))
		{
			ArenaPushType(input->arena, CodeLine);
			row++;
			input->lines[row].string = at + 1;
			input->lines[row].length = 0;
			input->line_n++;
		}
		else
		{
			input->lines[row].length++;
		}

		at++;
	}
}

struct tdef StackState
{
	int var_stack_size;
	int meta_var_stack_size;
	int struct_stack_size;
	int func_stack_size;
	int operator_stack_size;
	int enum_stack_size;
	int constructor_stack_size;
};

static StackState
func GetStackState(ParseInput *input)
{
	StackState state = {};
	state.var_stack_size = input->var_stack.size;
	state.meta_var_stack_size = input->meta_var_stack.size;
	state.struct_stack_size = input->struct_stack.size;
	state.func_stack_size = input->func_stack.size;
	state.operator_stack_size = input->operator_stack.size;
	state.enum_stack_size = input->enum_stack.size;
	state.constructor_stack_size = input->constructor_stack.size;
	return state;
}

static void
func SetStackState(ParseInput *input, StackState stack_state)
{
	input->var_stack.size = stack_state.var_stack_size;
	input->meta_var_stack.size = stack_state.meta_var_stack_size;
	input->struct_stack.size = stack_state.struct_stack_size;
	input->func_stack.size = stack_state.func_stack_size;
	input->operator_stack.size = stack_state.operator_stack_size;
	input->enum_stack.size = stack_state.enum_stack_size;
	input->constructor_stack.size = stack_state.constructor_stack_size;
}

#include <stdio.h>

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
	for(int i = 0; i < token.col - 1; i++)
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
	printf("%s\n", description);
	printf("In line %i\n", token.row);
	PrintTokenInLine(input, token);
	printf("\n");

	DebugBreak();
}

static void
func SetError(ParseInput *input, char *description)
{
	SetErrorToken(input, description, input->last_token);
}

static Token
func CreatePrefixToken(ParseInput *input, char *prefix, Token token)
{
	MemArena *arena = input->token_name_arena;
	Token result = {};
	result.id = NameTokenId;
	result.length = 0;
	result.text = arena->memory + arena->used_size;

	for(int i = 0; prefix[i]; i++)
	{
		ArenaPushType(arena, char);
		result.text[result.length] = prefix[i];
		result.length++;
	}

	for(int i = 0; i < token.length; i++)
	{
		ArenaPushType(arena, char);
		result.text[result.length] = token.text[i];
		result.length++;
	}

	return result;
}

static Token
func ReadToken(ParseInput *input)
{
	CodePosition *pos = input->pos;
	SkipWhiteSpaceAndComments(pos);

	Token token = {};
	token.id = NoTokenId;
	token.text = pos->at;
	token.row = pos->row;
	token.col = pos->col;
	token.length = 0;
	
	if(pos->at[0] == 0)
	{
		token.id = EndOfFileTokenId;
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
		token.id = OpenBracketsTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == ']')
	{
		token.id = CloseBracketsTokenId;
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
		else
		{
			token.id = ColonTokenId;
			token.length = 1;
			pos->at++;
		}
	}
	else if(pos->at[0] == '?')
	{
		token.id = QuestionMarkTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '!')
	{
		if(pos->at[1] == '=')
		{
			token.id = ExclamationMarkEqualsTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else
		{
			token.id = ExclamationMarkTokenId;
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
	else if(pos->at[0] == '+')
	{
		if(pos->at[1] == '+')
		{
			token.id = PlusPlusTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else if(pos->at[1] == '=')
		{
			token.id = PlusEqualsTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else
		{
			token.id = PlusTokenId;
			token.length = 1;
			pos->at++;
		}
	}
	else if(pos->at[0] == '-')
	{
		if(pos->at[1] == '=')
		{
			token.id = MinusEqualsTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else if(pos->at[1] == '-')
		{
			token.id = MinusMinusTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else
		{
			token.id = MinusTokenId;
			token.length = 1;
			pos->at++;
		}
	}
	else if(pos->at[0] == '|')
	{
		if(pos->at[1] == '=')
		{
			token.id = OrEqualsTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else if(pos->at[1] == '|')
		{
			token.id = OrOrTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else
		{
			token.id = OrTokenId;
			token.length = 1;
			pos->at++;
		}
	}
	else if(pos->at[0] == '&')
	{
		if(pos->at[1] == '&')
		{
			token.id = AndAndTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else
		{
			token.id = AndTokenId;
			token.length = 1;
			pos->at++;
		}
	}
	else if(pos->at[0] == '<')
	{
		if(pos->at[1] == '<')
		{
			token.id = LessThanLessThanTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else if(pos->at[1] == '=')
		{
			token.id = LessThanOrEqualToTokenId;
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
		if(pos->at[1] == '>')
		{
			token.id = GreaterThanGreaterThanTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else if(pos->at[1] == '=')
		{
			token.id = GreaterThanOrEqualToTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else
		{
			token.id = GreaterThanTokenId;
			token.length = 1;
			pos->at++;
		}
	}
	else if(pos->at[0] == '=')
	{
		if(pos->at[1] == '=')
		{
			token.id = EqualsEqualsTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else
		{
			token.id = EqualsTokenId;
			token.length = 1;
			pos->at++;
		}
	}
	else if(pos->at[0] == '@')
	{
		token.id = AtTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '#')
	{
		token.id = PoundTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == ',')
	{
		token.id = CommaTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '.')
	{
		token.id = DotTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '*')
	{
		if(pos->at[1] == '=')
		{
			token.id = StarEqualsTokenId;
			token.length = 2;
			pos->at += 2;
		}
		else
		{
			token.id = StarTokenId;
			token.length = 1;
			pos->at++;
		}
	}
	else if(pos->at[0] == '/')
	{
		token.id = SlashTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '\'')
	{
		token.id = CharacterConstantTokenId;
		token.length = 1;

		if(pos->at[1] == '\\')
		{
			token.length++;
			if(pos->at[2] == 'n')
			{
				token.length++;
			}
			else
			{
				SetError(input, "Invalid character after '\\' in character constant.");
			}
		}
		else
		{
			token.length++;
		}

		if(pos->at[token.length] == '\'')
		{
			token.length++;
		}
		else
		{
			SetError(input, "Expected closing '\'\' for character constant.");
		}

		pos->at += token.length;
	}
	else if(pos->at[0] == '"')
	{
		token.id = StringConstantTokenId;
		token.length++;
		while(1)
		{
			char c = pos->at[token.length];
			if(!c)
			{
				SetError(input, "Unexpected end of file in string constant.");
			}
			else if(IsNewLine(c))
			{
				SetError(input, "Unexpected new line in string constant.");
			}

			token.length++;
			if(c == '"')
			{
				break;
			}
		}

		pos->at += token.length;
	}
	else if(IsDigit(pos->at[0]))
	{
		if(pos->at[0] == '0' && (pos->at[1] == 'x' || pos->at[1] == 'X'))
		{
			if(IsHexadecimalDigit(pos->at[2]))
			{
				token.id = IntegerConstantTokenId;
				token.length = 2;
				pos->at += 2;
				while(IsHexadecimalDigit(pos->at[0]))
				{
					token.length++;
					pos->at++;
				}
			}
		}
		else if(pos->at[0] == '0' && (pos->at[1] == 'b' || pos->at[1] == 'B'))
		{
			if(IsBinaryDigit(pos->at[2]))
			{
				token.id = IntegerConstantTokenId;
				token.length = 2;
				pos->at += 2;
				while(IsBinaryDigit(pos->at[0]))
				{
					token.length++;
					pos->at++;
				}
			}
		}
		else if(pos->at[0] == '0' && IsOctalDigit(pos->at[1]))
		{
			token.id = IntegerConstantTokenId;
			token.length = 1;
			pos->at += 1;
			while(IsOctalDigit(pos->at[0]))
			{
				token.length++;
				pos->at++;
			}
		}
		else
		{
			bool is_integer = true;
			while(1)
			{
				if(IsDigit(pos->at[0]))
				{
					token.length++;
					pos->at++;
				}
				else if(pos->at[0] == '.')
				{
					if(is_integer)
					{
						is_integer = false;
						if(IsDigit(pos->at[1]))
						{
							token.length += 2;
							pos->at += 2;
						}
						else
						{
							SetError(input, "Non-number character in real constant.");
							break;
						}
					}
					else
					{
						SetError(input, "Non-integer found before '.' of real constant.");
						break;
					}
				}
				else
				{
					break;
				}
			}

			token.id = is_integer ? IntegerConstantTokenId : RealConstantTokenId;
		}
	}
	else if(IsAlpha(pos->at[0]))
	{
		while(IsAlpha(pos->at[0]) || IsDigit(pos->at[0]))
		{
			token.length++;
			pos->at++;
		}

		if(TokenEquals(token, "func"))
		{
			token.id = FuncTokenId;
		}
		else if(TokenEquals(token, "for"))
		{
			token.id = ForTokenId;
		}
		else if(TokenEquals(token, "in"))
		{
			token.id = InTokenId;
		}
		else if(TokenEquals(token, "to"))
		{
			token.id = ToTokenId;
		}
		else if(TokenEquals(token, "struct"))
		{
			token.id = StructTokenId;
		}
		else if(TokenEquals(token, "return"))
		{
			token.id = ReturnTokenId;
		}
		else if(TokenEquals(token, "if"))
		{
			token.id = IfTokenId;
		}
		else if(TokenEquals(token, "else"))
		{
			token.id = ElseTokenId;
		}
		else if(TokenEquals(token, "while"))
		{
			token.id = WhileTokenId;
		}
		else if(TokenEquals(token, "use"))
		{
			token.id = UseTokenId;
		}
		else if(TokenEquals(token, "true"))
		{
			token.id = TrueTokenId;
		}
		else if(TokenEquals(token, "false"))
		{
			token.id = FalseTokenId;
		}
		else if(TokenEquals(token, "break"))
		{
			token.id = BreakTokenId;
		}
		else if(TokenEquals(token, "continue"))
		{
			token.id = ContinueTokenId;
		}
		else if(TokenEquals(token, "decl"))
		{
			token.id = DeclTokenId;
		}
		else if(TokenEquals(token, "operator"))
		{
			token.id = OperatorTokenId;
		}
		else if(TokenEquals(token, "enum"))
		{
			token.id = EnumTokenId;
		}
		else if(TokenEquals(token, "constructor"))
		{
			token.id = ConstructorTokenId;
		}
		else if(TokenEquals(token, "namespace"))
		{
			token.id = NamespaceTokenId;
		}
		else
		{
			token.id = NameTokenId;
		}
	}
	else
	{
		DebugBreak();
	}

	input->pos->col += token.length;
	input->last_token = token;
	Assert(token.id != NoTokenId);
	return token;
}

static bool
func PeekToken(ParseInput *input, TokenId type)
{
	bool result = false;

	CodePosition start_pos = *input->pos;
	Token token = ReadToken(input);

	result = (token.id == type);
	*input->pos = start_pos;
	return result;
}

static bool
func PeekTwoTokens(ParseInput *input, TokenId type1, TokenId type2)
{
	bool result = false;

	CodePosition start_pos = *input->pos;
	Token token1 = ReadToken(input);
	Token token2 = ReadToken(input);

	result = (token1.id == type1 && token2.id == type2);
	*input->pos = start_pos;
	return result;
}

static bool
func PeekThreeTokens(ParseInput *input, TokenId type1, TokenId type2, TokenId type3)
{
	bool result = false;

	CodePosition start_pos = *input->pos;
	Token token1 = ReadToken(input);
	Token token2 = ReadToken(input);
	Token token3 = ReadToken(input);

	result = (token1.id == type1 && token2.id == type2 && token3.id == type3);
	*input->pos = start_pos;
	return result;
}

static bool 
func ReadTokenType(ParseInput *input, TokenId type)
{
	bool result = false;
	SkipWhiteSpaceAndComments(input->pos);

	CodePosition start_pos = *input->pos;
	Token token = ReadToken(input);

	if(token.id != type)
	{
		*input->pos = start_pos;
	}

	result = (token.id == type);
	return result;
}

struct tdef NameList
{
	int size;
	Token *names;
};

static NameList
func ReadNameList(ParseInput *input)
{
	NameList list = {};
	list.size = 0;
	list.names = ArenaPushArray(input->arena, 0, Token);
	while(true)
	{
		if(ReadTokenType(input, NameTokenId))
		{
			Token *token = ArenaPushType(input->arena, Token);
			*token = input->last_token;
			list.size++;
		}
		else
		{
			break;
		}

		if(!ReadTokenType(input, CommaTokenId))
		{
			break;
		}
	}

	return list;
}

static BaseType *
func PushBaseType(MemArena *arena, BaseVarTypeId base_id)
{
	BaseType *base_type = ArenaPushType(arena, BaseType);
	base_type->type.id = BaseTypeId;
	base_type->base_id = base_id;
	return base_type;
}

static PointerType *
func PushPointerType(MemArena *arena, VarType *pointed_type)
{
	Assert(pointed_type != 0);
	PointerType *type = ArenaPushType(arena, PointerType);
	type->type.id = PointerTypeId;
	type->pointed_type = pointed_type;
	return type;
}

static StructType *
func PushStructType(MemArena *arena, Struct* str)
{
	StructType *type = ArenaPushType(arena, StructType);
	type->type.id = StructTypeId;
	type->str = str;
	return type;
}

static EnumType *
func PushEnumType(MemArena *arena, Enum *e)
{
	EnumType *type = ArenaPushType(arena, EnumType);	
	type->type.id = EnumTypeId;
	type->e = e;
	return type;
}

static ArrayType *
func PushArrayType(MemArena *arena, Expression *size, Token meta_size_var_name, VarType *element_type)
{
	Assert(size && IsIntegerType(size->type) && size->is_const);
	Assert(element_type != 0);
	ArrayType *type = ArenaPushType(arena, ArrayType);
	type->type.id = ArrayTypeId;
	type->size = size;
	type->meta_size_var_name = meta_size_var_name;
	type->element_type = element_type;
	return type;
}

static Expression * decl ReadExpression(ParseInput *input);

static VarType *
func ReadVarType(ParseInput *input)
{
	VarType *type = 0;

	if(ReadTokenType(input, AtTokenId))
	{
		VarType *pointed_type = ReadVarType(input);
		if(!pointed_type)
		{
			SetError(input, "Expected var type after '@'.");
		}

		type = (VarType *)PushPointerType(input->arena, pointed_type);
	}
	else if(ReadTokenType(input, OpenBracketsTokenId))
	{
		Expression *size_expression = ReadExpression(input);
		Token meta_size_var_name = {};
		if(!size_expression)
		{
			SetError(input, "Expected array size expression.");
		}

		if(!IsIntegerType(size_expression->type))
		{
			SetError(input, "Array size expression is not integer.");
		}

		if(!size_expression->is_const)
		{
			SetError(input, "Array size expression is not constant.");
		}

		if(ReadTokenType(input, PoundTokenId))
		{
			if(!ReadTokenType(input, NameTokenId))
			{
				SetError(input, "Expected meta size variable name.");
			}

			meta_size_var_name = input->last_token;
		}

		if(!ReadTokenType(input, CloseBracketsTokenId))
		{
			SetError(input, "Expected ']' after array size expression.");
		}

		VarType *element_type = ReadVarType(input);
		type = (VarType *)PushArrayType(input->arena, size_expression, meta_size_var_name, element_type);
	}
	else if(ReadTokenType(input, NameTokenId))
	{
		if(TokenEquals(input->last_token, "Int8"))
		{
			type = (VarType *)PushBaseType(input->arena, Int8BaseTypeId);
		}
		else if(TokenEquals(input->last_token, "Int32"))
		{
			type = (VarType *)PushBaseType(input->arena, Int32BaseTypeId);
		}
		else if(TokenEquals(input->last_token, "UInt8"))
		{
			type = (VarType *)PushBaseType(input->arena, UInt8BaseTypeId);
		}
		else if(TokenEquals(input->last_token, "UInt32"))
		{
			type = (VarType *)PushBaseType(input->arena, UInt32BaseTypeId);
		}
		else if(TokenEquals(input->last_token, "Real32"))
		{
			type = (VarType *)PushBaseType(input->arena, Real32BaseTypeId);
		}
		else if(TokenEquals(input->last_token, "Bool32"))
		{
			type = (VarType *)PushBaseType(input->arena, Bool32BaseTypeId);
		}
		else
		{
			Struct *str = GetStruct(&input->struct_stack, input->last_token);
			if(str)
			{
				type = (VarType *)PushStructType(input->arena, str);
			}

			Enum *e = GetEnum(&input->enum_stack, input->last_token);
			if(e)
			{
				type = (VarType *)PushEnumType(input->arena, e);
			}
		}
	}

	if(!type)
	{
		SetError(input, "Expected var type description.");
	}

	return type;
}

static bool
func VarTypeHasMetaData(VarType *type)
{
	bool has_meta_data = false;

	if(type->id == ArrayTypeId)
	{
		ArrayType *array_type = (ArrayType *)type;
		has_meta_data = (array_type->meta_size_var_name.id != NoTokenId);

		if(!has_meta_data)
		{
			has_meta_data = VarTypeHasMetaData(array_type->element_type);
		}
	}
	else if(type->id == PointerTypeId)
	{
		PointerType *pointer_type = (PointerType *)type;
		has_meta_data = VarTypeHasMetaData(pointer_type->pointed_type);
	}

	return has_meta_data;
}

/*
func GetMatchingType(type1, type2 :@VarType) :@VarType
{
	type:@VarType;
	Assert(type1 && type2);

	if TypesEqual(type1, type2)
	{
		type = type1;
	}
	else
	{
		if IsNumericalType(type1) && IsNumericalType(type2)
		{
			priority1 := GetNumericalTypePriority(type1);
			priority2 := GetNumericalTypePriority(type2);
			type = (priority1 < priority2) ? type1 : type2;
		}
	}
}
*/

static VarType *
func GetMatchingType(VarType *type1, VarType *type2)
{
	VarType *type = 0;
	Assert(type1 != 0 && type2 != 0);
	if(TypesEqual(type1, type2))
	{
		type = type1;
	}
	else
	{
		if(IsNumericalType(type1) && IsNumericalType(type2))
		{
			int priority1 = GetNumericalTypePriority(type1);
			int priority2 = GetNumericalTypePriority(type2);
			type = (priority1 < priority2) ? type1 : type2;
		}
	}
	return type;
}

static bool decl MatchVarWithType(Var *var, VarType *type);

static bool
func CanMatchExpressionWithType(Expression *expr, VarType *type)
{
	bool can_match = false;
	if(TypesEqual(expr->type, type))
	{
		can_match = true;
	}
	else
	{
		if(!expr->has_final_type)
		{
			VarType *matching_type = GetMatchingType(expr->type, type);
			can_match = (matching_type == type);
		}
	}

	return can_match;
}

static bool 
func MatchExpressionWithType(ParseInput *input, Expression *expr, VarType *type)
{
	bool can_match = false;
	if(TypesEqual(expr->type, type))
	{
		can_match = true;
	}
	else if(expr->has_final_type)
	{
		can_match = TypesEqual(expr->type, type);
	}
	else
	{
		VarType *matching_type = GetMatchingType(expr->type, type);
		if(TypesEqual(matching_type, type))
		{
			expr->type = type;
			can_match = true;

			switch(expr->id)
			{
				case VarExpressionId:
				{
					VarExpression *var_expr = (VarExpression *)expr;
					Var *var = GetVar(&input->var_stack, var_expr->var.name);
					Assert(var);
					can_match &= MatchVarWithType(var, type);
					var_expr->var = *var;
					break;
				}
				case LessThanExpressionId:
				case GreaterThanExpressionId:
				case GreaterThanOrEqualToExpressionId:
				case LessThanOrEqualToExpressionId:
				case EqualExpressionId:
				{
					can_match = IsBoolType(type);
					break;
				}
				case ProductExpressionId:
				{
					ProductExpression *pr_expr = (ProductExpression *)expr;
					can_match &= MatchExpressionWithType(input, pr_expr->left, type);
					can_match &= MatchExpressionWithType(input, pr_expr->right, type);
					break;
				}
				case DivideExpressionId:
				{
					DivideExpression *dv_expr = (DivideExpression *)expr;
					can_match &= MatchExpressionWithType(input, dv_expr->left, type);
					can_match &= MatchExpressionWithType(input, dv_expr->right, type);
					break;
				}
				case ParenExpressionId:
				{
					ParenExpression *prn_expr = (ParenExpression *)expr;
					can_match &= MatchExpressionWithType(input, prn_expr->in, type);
					break;
				}
				case AddExpressionId:
				{
					AddExpression *add_expr = (AddExpression *)expr;
					can_match &= MatchExpressionWithType(input, add_expr->left, type);
					can_match &= MatchExpressionWithType(input, add_expr->right, type);
					break;
				}
				case SubtractExpressionId:
				{
					SubtractExpression *sub_expr = (SubtractExpression *)expr;
					can_match &= MatchExpressionWithType(input, sub_expr->left, type);
					can_match &= MatchExpressionWithType(input, sub_expr->right, type);
					break;
				}
				case LeftShiftExpressionId:
				{
					LeftShiftExpression *ls_expr = (LeftShiftExpression *)expr;
					if(IsIntegerType(type))
					{
						can_match &= MatchExpressionWithType(input, ls_expr->left, type);
					}
					else
					{
						can_match = false;
					}
					break;
				}
				case RightShiftExpressionId:
				{
					RightShiftExpression *rs_expr = (RightShiftExpression *)expr;
					if(IsIntegerType(type))
					{
						can_match &= MatchExpressionWithType(input, rs_expr->right, type);
					}
					else
					{
						can_match = false;
					}
					break;
				}
				case TernaryOperatorExpressionId:
				{
					TernaryOperatorExpression *to_expr = (TernaryOperatorExpression *)expr;
					can_match &= MatchExpressionWithType(input, to_expr->left, type);
					can_match &= MatchExpressionWithType(input, to_expr->right, type);
					break;
				}
				case OrExpressionId:
				{
					OrExpression *or_expr = (OrExpression *)expr;
					can_match &= MatchExpressionWithType(input, or_expr->left, type);
					can_match &= MatchExpressionWithType(input, or_expr->right, type);
					break;
				}
				case AndExpressionId:
				{
					AndExpression *and_expr = (AndExpression *)expr;
					can_match &= MatchExpressionWithType(input, and_expr->left, type);
					can_match &= MatchExpressionWithType(input, and_expr->right, type);
					break;
				}
				case InvertExpressionId:
				{
					InvertExpression *inv_expr = (InvertExpression *)expr;
					can_match &= MatchExpressionWithType(input, inv_expr->inverted, type);
					break;
				}
				case NegateExpressionId:
				{
					NegateExpression *ng_expr = (NegateExpression *)expr;
					can_match &= MatchExpressionWithType(input, ng_expr->negated, type);
					break;
				}
				case BitAndExpressionId:
				{
					BitAndExpression *ba_expr = (BitAndExpression *)expr;
					if(IsIntegerType(type))
					{
						can_match &= MatchExpressionWithType(input, ba_expr->left, type);
						can_match &= MatchExpressionWithType(input, ba_expr->right, type);
					}
					else
					{
						can_match = false;
					}
					break;
				}
				default:
				{
					can_match = true;
				}
			}
		}
	}

	return can_match;
}

static bool
func MatchExpressionTypes(ParseInput *input, Expression *expr1, Expression *expr2)
{
	Assert(expr1 != 0 && expr2 != 0);
	VarType *type = GetMatchingType(expr1->type, expr2->type);
	bool match1 = MatchExpressionWithType(input, expr1, type);
	bool match2 = MatchExpressionWithType(input, expr2, type);
	bool match = (match1 && match2);
	return match;
}

static bool
func MatchExpressionTypesToAdd(ParseInput *input, Expression *expr1, Expression *expr2)
{
	Assert(expr1 != 0 && expr2 != 0);

	bool can_match = false;
	if(IsNumericalType(expr1->type) && IsNumericalType(expr2->type))
	{
		can_match = MatchExpressionTypes(input, expr1, expr2);
	}
	else if(IsPointerType(expr1->type) && IsNumericalType(expr2->type))
	{
		can_match = true;
	}
	return can_match;
}

static bool 
func IsConditionExpression(Expression *expression)
{
	bool is_condition = false;
	if(expression)
	{
		if(IsNumericalType(expression->type) || IsPointerType(expression->type))
		{
			is_condition = true;
		}
	}
	return is_condition;
}

static VarType * 
func GetType(ParseInput *input, Token name)
{
	VarType *type = 0;
	if(TokenEquals(name, "Int8"))
	{
		type = (VarType *)PushBaseType(input->arena, Int8BaseTypeId);
	}
	else if(TokenEquals(name, "Int32"))
	{
		type = (VarType *)PushBaseType(input->arena, Int32BaseTypeId);
	}
	else if(TokenEquals(name, "UInt8"))
	{
		type = (VarType *)PushBaseType(input->arena, UInt8BaseTypeId);
	}
	else if(TokenEquals(name, "UInt32"))
	{
		type = (VarType *)PushBaseType(input->arena, UInt32BaseTypeId);
	}
	else if(TokenEquals(name, "Real32"))
	{
		type = (VarType *)PushBaseType(input->arena, Real32BaseTypeId);
	}
	else
	{
		Struct *str = GetStruct(&input->struct_stack, name);
		if(str)
		{
			type = (VarType *)PushStructType(input->arena, str);
		}
	}
	return type;
}

static bool 
func TypeExists(ParseInput *input, Token name)
{
	VarType *type = GetType(input, name);
	bool exists = (type != 0);
	return exists;
}

static bool 
func CanCastTypeTo(VarType *type_from, VarType *type_to)
{
	bool can_cast = false;
	if(IsNumericalType(type_from) && IsNumericalType(type_to))
	{
		can_cast = true;
	}
	return can_cast;
}

static bool 
func ExpressionHasAddress(Expression *expression)
{
	bool has_address = false;
	if(expression)
	{
		switch(expression->id)
		{
			case VarExpressionId:
			case DereferenceExpressionId:
			case ArrayIndexExpressionId:
			case StructVarExpressionId:
			{
				has_address = true;
				break;
			}
			case StructMetaVarExpressionId:
			{
				// TODO: fix case when it actually has address
				has_address = false;
				break;
			}
			case TernaryOperatorExpressionId:
			{
				TernaryOperatorExpression *ternary_op = (TernaryOperatorExpression *)expression;
				has_address = ExpressionHasAddress(ternary_op->left) && ExpressionHasAddress(ternary_op->right);
				break;
			}
			default:
			{
				has_address = false;
			}
		}
	}
	return has_address;
}

static IntegerConstantExpression *
func PushIntegerConstantExpression(MemArena *arena, Token token)
{
	IntegerConstantExpression *result = ArenaPushType(arena, IntegerConstantExpression);
	result->expr.id = IntegerConstantExpressionId;
	result->expr.is_const = true;
	result->expr.has_final_type = false;

	VarType *type = 0;
	Assert(token.length > 0);
	if(token.text[0] == '-')
	{
		type = (VarType *)PushBaseType(arena, Int32BaseTypeId);
	}
	else
	{
		type = (VarType *)PushBaseType(arena, UInt32BaseTypeId);
	}
	result->expr.type = type;
	result->token = token;
	return result;
}

static CharacterConstantExpression *
func PushCharacterConstantExpression(MemArena *arena, Token token)
{
	CharacterConstantExpression *result = ArenaPushType(arena, CharacterConstantExpression);
	result->expr.id = CharacterConstantExpressionId;
	result->expr.is_const = true;
	result->expr.has_final_type = false;

	result->expr.type = (VarType *)PushBaseType(arena, Int8BaseTypeId);
	result->token = token;
	return result;
}

static RealConstantExpression *
func PushRealConstantExpression(MemArena *arena, Token token)
{
	RealConstantExpression *result = ArenaPushType(arena, RealConstantExpression);
	result->expr.id = RealConstantExpressionId;
	result->expr.is_const = true;
	result->expr.has_final_type = false;

	result->expr.type = (VarType *)PushBaseType(arena, Real32BaseTypeId);
	result->token = token;
	return result;
}

static VarExpression * 
func PushVarExpression(MemArena *arena, Var var)
{
	VarExpression *result = ArenaPushType(arena, VarExpression);
	result->expr.id = VarExpressionId;
	result->expr.is_const = false;
	result->expr.has_final_type = var.has_final_type;
	result->expr.type = var.type;
	result->var = var;
	Assert(result != 0);
	return result;
}

static MetaVarExpression *
func PushMetaVarExpression(MemArena *arena, MetaVar var)
{
	MetaVarExpression *result = ArenaPushType(arena, MetaVarExpression);
	result->expr.id = MetaVarExpressionId;
	result->expr.is_const = var.expression->is_const;
	result->expr.has_final_type = var.expression->has_final_type;
	result->expr.type = var.type;
	result->var = var;
	Assert(result != 0);
	return result;
}

static LessThanExpression * 
func PushLessThanExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);
	Assert(TypesEqual(left->type, right->type));
	LessThanExpression *result = ArenaPushType(arena, LessThanExpression);
	Assert(result != 0);
	result->expr.id = LessThanExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->expr.type = (VarType *)PushBaseType(arena, Bool32BaseTypeId);
	result->left = left;
	result->right = right;
	return result;
}

static EqualExpression * 
func PushEqualExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);
	Assert(TypesEqual(left->type, right->type));
	EqualExpression *result = ArenaPushType(arena, EqualExpression);
	Assert(result != 0);
	result->expr.id = EqualExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->expr.type = (VarType *)PushBaseType(arena, Bool32BaseTypeId);
	result->left = left;
	result->right = right;
	return result;
}

static NotEqualExpression *
func PushNotEqualExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);
	Assert(TypesEqual(left->type, right->type));
	NotEqualExpression *result = ArenaPushType(arena, NotEqualExpression);
	Assert(result != 0);
	result->expr.id = NotEqualExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->expr.type = (VarType *)PushBaseType(arena, Bool32BaseTypeId);
	result->left = left;
	result->right = right;
	return result;
}

static GreaterThanExpression * 
func PushGreaterThanExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);
	Assert(TypesEqual(left->type, right->type));
	GreaterThanExpression *result = ArenaPushType(arena, GreaterThanExpression);
	Assert(result != 0);
	result->expr.id = GreaterThanExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->expr.type = (VarType *)PushBaseType(arena, Bool32BaseTypeId);
	result->left = left;
	result->right = right;
	return result;
}

static GreaterThanOrEqualToExpression * 
func PushGreaterThanOrEqualToExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);
	Assert(TypesEqual(left->type, right->type));
	GreaterThanOrEqualToExpression *result = ArenaPushType(arena, GreaterThanOrEqualToExpression);
	Assert(result != 0);
	result->expr.id = GreaterThanOrEqualToExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->expr.type = (VarType *)PushBaseType(arena, Bool32BaseTypeId);
	result->left = left;
	result->right = right;
	return result;
}

static LessThanOrEqualToExpression * 
func PushLessThanOrEqualToExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);
	Assert(TypesEqual(left->type, right->type));
	LessThanOrEqualToExpression *result = ArenaPushType(arena, LessThanOrEqualToExpression);
	result->expr.id = LessThanOrEqualToExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->expr.type = (VarType *)PushBaseType(arena, Bool32BaseTypeId);
	result->left = left;
	result->right = right;
	Assert(result != 0);
	return result;
}

static DereferenceExpression * 
func PushDereferenceExpression(MemArena *arena, Expression *base)
{
	Assert(base != 0);
	DereferenceExpression *result = ArenaPushType(arena, DereferenceExpression);
	Assert(result != 0);
	result->expr.id = DereferenceExpressionId;
	result->expr.is_const = false;
	result->expr.has_final_type = true;
	Assert(base->type->id == PointerTypeId);
	PointerType *pointer_type = (PointerType *)base->type;
	result->expr.type = pointer_type->pointed_type;
	result->base = base;
	return result;
}

static ArrayIndexExpression * 
func PushArrayIndexExpression(MemArena *arena, Expression *array, Expression *index)
{
	Assert(array != 0 && index != 0);
	Assert(array->type->id == ArrayTypeId);
	ArrayType *array_type = (ArrayType *)array->type;
	Assert(IsIntegerType(index->type));
	ArrayIndexExpression *result = ArenaPushType(arena, ArrayIndexExpression);
	Assert(result != 0);
	result->expr.id = ArrayIndexExpressionId;
	result->expr.is_const = (array->is_const && index->is_const);
	result->expr.has_final_type = true;
	result->expr.type = array_type->element_type;
	result->array = array;
	result->index = index;
	return result;
}

static ProductExpression * 
func PushProductExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);
	Assert(TypesEqual(left->type, right->type));
	Assert(IsNumericalType(left->type));

	ProductExpression *result = ArenaPushType(arena, ProductExpression);
	Assert(result != 0);
	result->expr.type = left->type;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->expr.id = ProductExpressionId;
	result->left = left;
	result->right = right;
	return result;
}

static DivideExpression * 
func PushDivideExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);
	Assert(TypesEqual(left->type, right->type));
	Assert(IsNumericalType(left->type));

	DivideExpression *result = ArenaPushType(arena, DivideExpression);
	Assert(result != 0);
	result->expr.id = DivideExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->expr.type = left->type;
	result->left = left;
	result->right = right;
	return result;
}

static ParenExpression * 
func PushParenExpression(MemArena *arena, Expression *in)
{
	Assert(in != 0);
	ParenExpression *result = ArenaPushType(arena, ParenExpression);
	Assert(result != 0);
	result->expr.id = ParenExpressionId;
	result->expr.is_const = in->is_const;
	result->expr.has_final_type = in->has_final_type;
	result->expr.type = in->type;
	result->in = in;
	return result;
}

static AddExpression * 
func PushAddExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);

	AddExpression *result = ArenaPushType(arena, AddExpression);
	Assert(result != 0);
	if(TypesEqual(left->type, right->type))
	{
		Assert(IsNumericalType(left->type));
		result->expr.type = left->type;
	}
	else
	{
		Assert(IsPointerType(left->type));
		Assert(IsIntegerType(right->type));
		result->expr.type = left->type;
	}

	result->expr.id = AddExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->left = left;
	result->right = right;
	return result;
}

static SubtractExpression * 
func PushSubtractExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);

	SubtractExpression *result = ArenaPushType(arena, SubtractExpression);
	Assert(result != 0);
	if(TypesEqual(left->type, right->type))
	{
		if(IsNumericalType(left->type))
		{
			result->expr.type = left->type;
		}
		else if(IsPointerType(left->type))
		{
			result->expr.type = (VarType *)PushBaseType(arena, UInt32BaseTypeId);
		}
		else
		{
			DebugBreak();
		}
	}
	else
	{
		Assert(IsPointerType(left->type));
		Assert(IsIntegerType(right->type));
		result->expr.type = left->type;
	}

	result->expr.id = SubtractExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (right->has_final_type || right->has_final_type);
	result->left = left;
	result->right = right;
	return result;
}

static StructVarExpression *
func PushStructVarExpression(MemArena *arena, Expression *struct_expression, Token var_name)
{
	Assert(struct_expression != 0);
	Assert(struct_expression->type != 0 && struct_expression->type->id == StructTypeId);
	StructType *struct_type = (StructType *)struct_expression->type;
	Struct *str = struct_type->str;
	StructVar *struct_var = GetStructVar(str, var_name);
	Assert(struct_var != 0);

	StructVarExpression *result = ArenaPushType(arena, StructVarExpression);
	Assert(result != 0);
	result->expr.id = StructVarExpressionId;
	result->expr.is_const = struct_expression->is_const;
	result->expr.has_final_type = true;
	result->expr.type = struct_var->type;
	result->struct_expression = struct_expression;
	result->var_name = var_name;
	return result;
}

static StructDefVarExpression *
func PushStructDefVarExpression(MemArena *arena, Struct *struct_def, Token var_name)
{
	Assert(struct_def != 0);
	StructVar *struct_var = GetStructVar(struct_def, var_name);
	Assert(struct_var != 0);

	StructDefVarExpression *result = ArenaPushType(arena, StructDefVarExpression);
	Assert(result != 0);
	result->expr.id = StructDefVarExpressionId;
	result->expr.is_const = false;
	result->expr.type = struct_var->type;
	result->expr.has_final_type = true;
	result->struct_def = struct_def;
	result->var_name = var_name;
	return result;
}

static StructMetaVarExpression *
func PushStructMetaVarExpression(MemArena *arena, Expression *struct_expression, Token var_name)
{
	Assert(struct_expression != 0);
	Assert(struct_expression->type != 0 && struct_expression->type->id == StructTypeId);
	StructType *struct_type = (StructType *)struct_expression->type;
	Struct *str = struct_type->str;
	StructMetaVar *var = GetStructMetaVar(str, var_name);
	Assert(var != 0);

	StructMetaVarExpression *result = ArenaPushType(arena, StructMetaVarExpression);
	Assert(result != 0);
	result->expr.id = StructMetaVarExpressionId;
	result->expr.is_const = var->expression->is_const;
	result->expr.type = var->expression->type;
	result->expr.has_final_type = var->expression->has_final_type;
	result->struct_expression = struct_expression;
	result->meta_expression = var->expression;
	result->var_name = var_name;
	return result;
};

static FuncCallExpression * 
func PushFuncCallExpression(MemArena *arena, Func *f, FuncCallParam *first_param)
{
	Assert(f != 0);

	FuncCallExpression *result = ArenaPushType(arena, FuncCallExpression);
	result->expr.id = FuncCallExpressionId;
	result->expr.is_const = false;
	result->expr.has_final_type = true;
	result->expr.type = f->header.return_type;
	result->f = f;
	result->first_param = first_param;
	return result;
}

static OperatorCallExpression *
func PushOperatorCallExpression(MemArena *arena, Operator *op, Expression *left, Expression *right)
{
	Assert(op != 0 && left != 0 && right != 0);
	Assert(TypesEqual(op->left_type, left->type) && TypesEqual(op->right_type, right->type));

	OperatorCallExpression *result = ArenaPushType(arena, OperatorCallExpression);
	result->expr.id = OperatorCallExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = true;
	result->expr.type = op->return_type;
	result->op = op;
	result->left = left;
	result->right = right;
	return result;
}

static EnumMemberExpression *
func PushEnumMemberExpression(MemArena *arena, EnumMember member)
{
	Assert(member.e != 0);

	EnumMemberExpression *result = ArenaPushType(arena, EnumMemberExpression);
	result->expr.id = EnumMemberExpressionId;
	result->expr.is_const = true;
	result->expr.has_final_type = true;
	result->expr.type = (VarType *)PushEnumType(arena, member.e);
	result->member = member;
	return result;
}

static CastExpression * 
func PushCastExpression(MemArena *arena, VarType *type, Expression *expression)
{
	Assert(type != 0 && expression != 0);

	CastExpression *result = ArenaPushType(arena, CastExpression);
	Assert(result != 0);
	result->expr.id = CastExpressionId;
	result->expr.is_const = expression->is_const;
	result->expr.has_final_type = true;
	result->expr.type = type;
	result->type = type;
	result->expression = expression;
	return result;
}

static LeftShiftExpression * 
func PushLeftShiftExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);

	LeftShiftExpression *result = ArenaPushType(arena, LeftShiftExpression);
	Assert(result != 0);
	result->expr.id = LeftShiftExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = left->has_final_type;
	result->expr.type = left->type;
	result->left = left;
	result->right = right;
	return result;
}

static RightShiftExpression * 
func PushRightShiftExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);

	RightShiftExpression *result = ArenaPushType(arena, RightShiftExpression);
	Assert(result != 0);

	result->expr.id = RightShiftExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = left->has_final_type;
	result->expr.type = left->type;
	result->left = left;
	result->right = right;
	return result;
}

static TernaryOperatorExpression *
func PushTernaryOperatorExpression(MemArena *arena, Expression *condition, Expression *left, Expression *right)
{
	Assert(condition != 0 && left != 0 && right != 0);
	Assert(IsConditionExpression(condition));
	Assert(TypesEqual(left->type, right->type));
	TernaryOperatorExpression *result = ArenaPushType(arena, TernaryOperatorExpression);
	Assert(result != 0);
	result->expr.id = TernaryOperatorExpressionId;
	result->expr.is_const = (condition->is_const && left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->expr.type = left->type;
	result->condition = condition;
	result->left = left;
	result->right = right;
	return result;
}

static AddressExpression * 
func PushAddressExpression(MemArena *arena, Expression *addressed)
{
	Assert(addressed != 0);
	Assert(ExpressionHasAddress(addressed));

	AddressExpression *result = ArenaPushType(arena, AddressExpression);
	Assert(result != 0);
	result->expr.id = AddressExpressionId;
	result->expr.is_const = (addressed->is_const);
	result->expr.type = (VarType *)PushPointerType(arena, addressed->type);
	result->addressed = addressed;
	return result;
}

static OrExpression * 
func PushOrExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);
	
	OrExpression *result = ArenaPushType(arena, OrExpression);
	Assert(result != 0);
	result->expr.id = OrExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->expr.type = (VarType *)PushBaseType(arena, Bool32BaseTypeId);
	result->left = left;
	result->right = right;
	return result;
}

static AndExpression *
func PushAndExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);

	AndExpression *result = ArenaPushType(arena, AndExpression);
	Assert(result != 0);
	result->expr.id = AndExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->expr.type = (VarType *)PushBaseType(arena, Bool32BaseTypeId);
	result->left = left;
	result->right = right;
	return result;
}

static InvertExpression * 
func PushInvertExpression(MemArena *arena, Expression *inverted)
{
	Assert(inverted != 0);

	InvertExpression *result = ArenaPushType(arena, InvertExpression);
	Assert(result != 0);
	result->expr.id = InvertExpressionId;
	result->expr.is_const = inverted->is_const;
	result->expr.has_final_type = inverted->has_final_type;
	result->expr.type = inverted->type;
	result->inverted = inverted;
	return result;
}

static BoolConstantExpression * 
func PushBoolConstantExpression(MemArena *arena, bool value)
{
	BoolConstantExpression *result = ArenaPushType(arena, BoolConstantExpression);
	Assert(result != 0);
	result->expr.id = BoolConstantExpressionId;
	result->expr.is_const = true;
	result->expr.has_final_type = false;
	result->expr.type = (VarType*)PushBaseType(arena, Bool32BaseTypeId);
	result->value = value;
	return result;
}

static NegateExpression * 
func PushNegateExpression(MemArena *arena, Expression *negated)
{
	Assert(negated != 0);
	NegateExpression *result = ArenaPushType(arena, NegateExpression);
	Assert(result != 0);

	result->expr.id = NegateExpressionId;
	result->expr.is_const = negated->is_const;
	result->expr.has_final_type = negated->has_final_type;
	result->expr.type = (VarType *)PushBaseType(arena, Bool32BaseTypeId);
	result->negated = negated;
	return result;
}

static BitAndExpression * 
func PushBitAndExpression(MemArena *arena, Expression *left, Expression *right)
{
	Assert(left != 0 && right != 0);
	Assert(TypesEqual(left->type, right->type));

	BitAndExpression *result = ArenaPushType(arena, BitAndExpression);
	Assert(result != 0);

	result->expr.id = BitAndExpressionId;
	result->expr.is_const = (left->is_const && right->is_const);
	result->expr.has_final_type = (left->has_final_type || right->has_final_type);
	result->expr.type = left->type;
	result->left = left;
	result->right = right;
	return result;
}

static ConstructorCallExpression *
func PushConstructorCallExpression(MemArena *arena, Constructor *ctor, FuncCallParam *first_param)
{
	Assert(ctor != 0 && first_param != 0);

	ConstructorCallExpression *result = ArenaPushType(arena, ConstructorCallExpression);
	result->expr.id = ConstructorCallExpressionId;
	result->expr.is_const = true;
	for(FuncCallParam *param = first_param; param; param = param->next)
	{
		if(!param->expression->is_const)
		{
			result->expr.is_const = false;
		}
	}
	result->expr.has_final_type = true;
	result->expr.type = ctor->type;

	result->ctor = ctor;
	result->first_param = first_param;
	return result;
}

static StringConstantExpression *
func PushStringConstantExpression(MemArena *arena, Token token)
{
	StringConstantExpression *result = ArenaPushType(arena, StringConstantExpression);
	result->expr.id = StringConstantExpressionId;
	result->expr.is_const = true;
	VarType *base_type = (VarType *)PushBaseType(arena, Int8BaseTypeId);
	result->expr.type = (VarType *)PushPointerType(arena, base_type);
	result->expr.has_final_type = true;

	result->token = token;
	return result;
}

static Expression * decl ReadExpression(ParseInput *input);

static bool
func ConstructorMatchesParams(Constructor *ctor, FuncCallParam *first_call_param)
{
	FuncParam *param = ctor->header.first_param;
	FuncCallParam *call_param = first_call_param;

	bool match = true;
	while(1)
	{
		if(!param && !call_param)
		{
			match = true;
			break;
		}

		if(param && !call_param)
		{
			match = false;
			break;
		}
		else if(!param && call_param)
		{
			match = false;
			break;
		}

		if(!CanMatchExpressionWithType(call_param->expression, param->type))
		{
			match = false;
			break;
		}

		param = param->next;
		call_param = call_param->next;
	}

	return match;
}

static Constructor *
func GetConstructor(ConstructorStack *stack, FuncCallParam *first_param)
{
	Constructor *result = 0;
	for(int i = 0; i < stack->size; i++)
	{
		Constructor *ctor = stack->constructors[i];
		if(ConstructorMatchesParams(ctor, first_param))
		{
			result = ctor;
		}
	}

	return result;
}

static ConstructorCallExpression *
func ReadConstructorCallExpression(ParseInput *input, VarType *type)
{
	Assert(type->id == StructTypeId);
	if(!ReadTokenType(input, OpenParenTokenId))
	{
		SetError(input, "Expected '(' after struct name (for constructor call).");
	}

	FuncCallParam *first_param = 0;
	FuncCallParam *last_param = 0;

	bool is_first = true;

	while(1)
	{
		if(ReadTokenType(input, CloseParenTokenId))
		{
			break;
		}

		if(!is_first)
		{
			if(!ReadTokenType(input, CommaTokenId))
			{
				SetError(input, "Expected ',' between constructor parameters.");
			}
		}
		is_first = false;

		Expression *value = ReadExpression(input);
		FuncCallParam *param = ArenaPushType(input->arena, FuncCallParam);
		param->expression = value;
		param->next = 0;
		if(!first_param)
		{
			Assert(!last_param);
			first_param = param;
			last_param = param;
		}
		else
		{
			Assert(last_param);
			last_param->next = param;
			last_param = param;
		}
	}

	Constructor *ctor = GetConstructor(&input->constructor_stack, first_param);
	if(!ctor)
	{
		SetError(input, "No constructors match call list.");
	}

	ConstructorCallExpression *result = PushConstructorCallExpression(input->arena, ctor, first_param);
	return result;
}

static Expression *
func ReadNumberLevelExpression(ParseInput *input)
{
	Expression *expr = 0;

	if(ReadTokenType(input, ExclamationMarkTokenId))
	{
		Expression *negated = ReadNumberLevelExpression(input);
		expr = (Expression *)PushNegateExpression(input->arena, negated);
	}
	else if(ReadTokenType(input, CharacterConstantTokenId))
	{
		expr = (Expression *)PushCharacterConstantExpression(input->arena, input->last_token);
	}
	else if(ReadTokenType(input, StringConstantTokenId))
	{
		expr = (Expression *)PushStringConstantExpression(input->arena, input->last_token);
	}
	else if(ReadTokenType(input, IntegerConstantTokenId))
	{
		expr = (Expression *)PushIntegerConstantExpression(input->arena, input->last_token);
	}
	else if(ReadTokenType(input, RealConstantTokenId))
	{
		expr = (Expression *)PushRealConstantExpression(input->arena, input->last_token);
	}
	else if(ReadTokenType(input, AtTokenId))
	{
		Expression *addressed_expression = ReadExpression(input);
		if(!ExpressionHasAddress(addressed_expression))
		{
			SetError(input, "Expression after '@' doesn't have an address.");
		}

		expr = (Expression *)PushAddressExpression(input->arena, addressed_expression);
	}
	else if(ReadTokenType(input, NameTokenId))
	{
		Token name = input->last_token;
		if(VarExists(&input->var_stack, name))
		{
			Var *var = GetVar(&input->var_stack, name);
			Assert(var != 0);
			expr = (Expression *)PushVarExpression(input->arena, *var);
		}
		else if(MetaVarExists(&input->meta_var_stack, name))
		{
			MetaVar *var = GetMetaVar(&input->meta_var_stack, name);
			Assert(var != 0);
			expr = (Expression *)PushMetaVarExpression(input->arena, *var);
		}
		else if(EnumMemberExists(&input->enum_stack, name))
		{
			EnumMember member = GetEnumMember(&input->enum_stack, name);
			Assert(member.e != 0);
			expr = (Expression *)PushEnumMemberExpression(input->arena, member);
		}
		else if(FuncExists(&input->func_stack, name))
		{
			Func *f = GetFunc(&input->func_stack, name);
			Assert(f != 0);
			if(!ReadTokenType(input, OpenParenTokenId))
			{
				SetError(input, "Expected '(' after function name.");
			}

			FuncCallParam *first_param = 0;
			FuncCallParam *last_param = 0;

			bool is_first_param = true;
			FuncParam *param = f->header.first_param;
			while(param)
			{
				if(!is_first_param)
				{
					if(!ReadTokenType(input, CommaTokenId))
					{
						SetError(input, "Expected ',' between function parameters.");
						break;
					}
				}
				is_first_param = false;

				Expression *expression = ReadExpression(input);
				if(expression == 0)
				{
					SetError(input, "Expected function parameter expression.");
					break;
				}

				if(!MatchExpressionWithType(input, expression, param->type))
				{
					SetError(input, "Expression type is incompatible with function parameter.");
					break;
				}

				FuncCallParam *call_param = ArenaPushType(input->arena, FuncCallParam);
				call_param->next = 0;
				call_param->expression = expression;

				if(first_param == 0)
				{
					Assert(last_param == 0);
					first_param = call_param;
					last_param = call_param;
				}
				else
				{
					Assert(last_param != 0);
					last_param->next = call_param;
					last_param = call_param;
				}

				param = param->next;
			}

			if(!ReadTokenType(input, CloseParenTokenId))
			{
				SetError(input, "Expected ')' after function parameters.");
			}

			expr = (Expression *)PushFuncCallExpression(input->arena, f, first_param);
		}
		else if(TypeExists(input, name))
		{
			VarType *type = GetType(input, name);

			if(type->id == StructTypeId)
			{
				expr = (Expression *)ReadConstructorCallExpression(input, type);
			}
			else
			{
				if(!ReadTokenType(input, OpenParenTokenId))
				{
					SetError(input, "Expected '(' after var type (for casting).");
				}

				Expression *expression = ReadExpression(input);
				if(expression == 0 || !CanCastTypeTo(expression->type, type))
				{
					SetError(input, "Cannot cast type of expression.");
				}

				if(!ReadTokenType(input, CloseParenTokenId))
				{
					SetError(input, "Expected ')' after expression (for casting).");
				}

				expr = (Expression *)PushCastExpression(input->arena, type, expression);
			}
		}
		else if(input->in_struct_definition)
		{
			Struct *in_struct = input->in_struct_definition;
			if(StructHasVar(in_struct, name))
			{
				expr = (Expression *)PushStructDefVarExpression(input->arena, in_struct, name);
			}
			else
			{
				SetError(input, "Unknown name (not a func, var, struct var, or type).");
			}
		}
		else
		{
			SetError(input, "Unknown name (not a func, var, or type).");
		}
	}
	else if(ReadTokenType(input, TrueTokenId))
	{
		expr = (Expression *)PushBoolConstantExpression(input->arena, true);
	}
	else if(ReadTokenType(input, FalseTokenId))
	{
		expr = (Expression *)PushBoolConstantExpression(input->arena, false);
	}
	else if(ReadTokenType(input, OpenParenTokenId))
	{
		Expression *in = ReadExpression(input);
		if(ReadTokenType(input, CloseParenTokenId))
		{
			Assert(in != 0);
			expr = (Expression *)PushParenExpression(input->arena, in);
		}
		else
		{
			SetError(input, "Expected ')'.");
		}
	}
	else
	{
		SetError(input, "Unknown expression.");
	}

	while(true)
	{
		if(ReadTokenType(input, AtTokenId))
		{
			if(expr->type == 0 || expr->type->id != PointerTypeId)
			{
				SetError(input, "Cannot dereference non-pointer expression.");
				break;
			}

			expr = (Expression *)PushDereferenceExpression(input->arena, expr);	
		}
		else if(ReadTokenType(input, OpenBracketsTokenId))
		{
			if(expr->type != 0 && expr->type->id == PointerTypeId)
			{
				expr = (Expression *)PushDereferenceExpression(input->arena, expr);
			}

			if(expr->type == 0 || expr->type->id != ArrayTypeId)
			{
				SetError(input, "Cannot index non-array expression.");
				break;
			}

			Expression *index = ReadExpression(input);
			if(index == 0 || !IsIntegerType(index->type))
			{
				SetError(input, "Array index is not integer.");
				break;
			}

			if(!ReadTokenType(input, CloseBracketsTokenId))
			{
				SetError(input, "Expected ']' after array index expression.");
				break;
			}

			expr = (Expression *)PushArrayIndexExpression(input->arena, expr, index);
		}
		else if(ReadTokenType(input, DotTokenId))
		{
			if(expr->type != 0 && expr->type->id == PointerTypeId)
			{
				expr = (Expression *)PushDereferenceExpression(input->arena, expr);
			}

			if(expr->type != 0 && expr->type->id == StructTypeId)
			{
				StructType *struct_type = (StructType *)expr->type;
				Struct *str = struct_type->str;
				Assert(str != 0);
				if(ReadTokenType(input, NameTokenId))
				{
					Token var_name = input->last_token;
					if(StructHasVar(str, var_name))
					{
						expr = (Expression *)PushStructVarExpression(input->arena, expr, var_name);
					}
					else if(StructHasMetaVar(str, var_name))
					{
						expr = (Expression *)PushStructMetaVarExpression(input->arena, expr, var_name);
					}
					else
					{
						SetError(input, "Struct doesn't have a member with that name.");
						break;
					}
				}
				else
				{
					SetError(input, "Invalid struct member name (keyword or not name).");
					break;
				}
			}
			else if(expr->type != 0 && expr->type->id == ArrayTypeId)
			{
				ArrayType *array = (ArrayType *)expr->type;
				Assert(array != 0);
				if(ReadTokenType(input, NameTokenId))
				{
					Token member_name = input->last_token;
					if(TokenEquals(member_name, "size"))
					{
						expr = array->size;
					}
					else
					{
						SetError(input, "Unknown array member.");
						break;
					}
				}
			}
			else
			{
				SetError(input, "Unexpected '.' (left side is not struct or array).");
				break;
			}
		}
		else
		{
			break;
		}
	}

	Assert(expr);
	return expr;
}

static Expression * 
func ReadBitLevelExpression(ParseInput *input)
{
	Expression *expr = ReadNumberLevelExpression(input);
	while(true)
	{
		if(ReadTokenType(input, LessThanLessThanTokenId))
		{
			Expression *right = ReadNumberLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression for left shift.");
			}

			if(!IsIntegerType(expr->type) || !IsIntegerType(right->type))
			{
				SetError(input, "Left shift expression not integer.");
			}

			expr = (Expression *)PushLeftShiftExpression(input->arena, expr, right);
		}
		else if(ReadTokenType(input, GreaterThanGreaterThanTokenId))
		{
			Expression *right = ReadNumberLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression for left shift.");
			}

			if(!IsIntegerType(expr->type) || !IsIntegerType(right->type))
			{
				SetError(input, "Left shift expression not integer.");
			}

			expr = (Expression *)PushRightShiftExpression(input->arena, expr, right);
		}
		else if(ReadTokenType(input, AndTokenId))
		{
			Expression *right = ReadNumberLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '&'.");
			}

			if(!MatchExpressionTypes(input, expr, right))
			{
				SetError(input, "Types do not match for '&' operation.");
			}

			if(!IsIntegerType(expr->type))
			{
				SetError(input, "Non-integer type for '&' operation.");
			}

			expr = (Expression *)PushBitAndExpression(input->arena, expr, right);
		}
		else
		{
			break;
		}
	}

	Assert(expr);
	return expr;
}

static Expression * 
func ReadProductLevelExpression(ParseInput *input)
{
	Expression *expr = ReadBitLevelExpression(input);

	while(true)
	{
		if(ReadTokenType(input, StarTokenId))
		{
			Expression *right = ReadBitLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '*'.");
			}

			Operator *op = GetOperator(&input->operator_stack, expr->type, right->type, StarTokenId);
			if(op)
			{
				expr = (Expression *)PushOperatorCallExpression(input->arena, op, expr, right);
			}
			else if(MatchExpressionTypes(input, expr, right))
			{
				expr = (Expression *)PushProductExpression(input->arena, expr, right);
			}
			else
			{
				SetError(input, "Invalid types for '*'.");
			}
		}
		else if(ReadTokenType(input, SlashTokenId))
		{
			Expression *right = ReadBitLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '/'.");
			}

			if(!MatchExpressionTypes(input, expr, right))
			{
				SetError(input, "Types do not match for '/'.");
			}

			expr = (Expression *)PushDivideExpression(input->arena, expr, right);
		}
		else
		{
			break;
		}
	}

	Assert(expr);
	return expr;
}

static Expression * 
func ReadSumLevelExpression(ParseInput *input)
{
	Expression *expr = 0;
	if(ReadTokenType(input, MinusTokenId))
	{
		Expression *right = ReadProductLevelExpression(input);
		expr = (Expression *)PushInvertExpression(input->arena, right);
	}
	else
	{
		expr = ReadProductLevelExpression(input);
	}

	while(true)
	{
		if(ReadTokenType(input, MinusTokenId))
		{
			Expression *right = ReadProductLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '-'.");
			}

			Operator *op = GetOperator(&input->operator_stack, expr->type, right->type, MinusTokenId);
			if(op)
			{
				expr = (Expression *)PushOperatorCallExpression(input->arena, op, expr, right);
			}
			else if(MatchExpressionTypes(input, expr, right))
			{
				expr = (Expression *)PushSubtractExpression(input->arena, expr, right);
			}
			else
			{
				SetError(input, "Types do not match for '-'.");
			}
		}
		else if(ReadTokenType(input, PlusTokenId))
		{
			Expression *right = ReadProductLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '+'.");
			}

			Operator *op = GetOperator(&input->operator_stack, expr->type, right->type, PlusTokenId);
			if(op)
			{
				expr = (Expression *)PushOperatorCallExpression(input->arena, op, expr, right);
			}
			else if(MatchExpressionTypesToAdd(input, expr, right))
			{
				expr = (Expression *)PushAddExpression(input->arena, expr, right);
			}
			else
			{
				SetError(input, "Types do not match for '+'.");
			}
		}
		else
		{
			break;
		}
	}

	Assert(expr);
	return expr;
}

static Expression * 
func ReadCompareLevelExpression(ParseInput *input)
{
	Expression *expr = ReadSumLevelExpression(input);
	while(true)
	{
		if(ReadTokenType(input, EqualsEqualsTokenId))
		{
			Expression *right = ReadSumLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '=='.");
			}

			Operator *op = GetOperator(&input->operator_stack, expr->type, right->type, EqualsEqualsTokenId);
			if(op)
			{
				expr = (Expression *)PushOperatorCallExpression(input->arena, op, expr, right);
			}
			else if(MatchExpressionTypes(input, expr, right))
			{
				expr = (Expression *)PushEqualExpression(input->arena, expr, right);
			}
			else
			{
				SetError(input, "Types do not match for '=='.");
			}			
		}
		else if(ReadTokenType(input, ExclamationMarkEqualsTokenId))
		{
			Expression *right = ReadSumLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '!='.");
			}

			if(!MatchExpressionTypes(input, expr, right))
			{
				SetError(input, "Types do not match for '!='.");
			}

			expr = (Expression *)PushNotEqualExpression(input->arena, expr, right);
		}
		else if(ReadTokenType(input, LessThanTokenId))
		{
			Expression *right = ReadSumLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '<'.");
			}

			if(!MatchExpressionTypesToAdd(input, expr, right))
			{
				SetError(input, "Types do not match for '<'.");
			}

			expr = (Expression *)PushLessThanExpression(input->arena, expr, right);
		}
		else if(ReadTokenType(input, LessThanOrEqualToTokenId))
		{
			Expression *right = ReadSumLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '<='.");
			}

			if(!MatchExpressionTypesToAdd(input, expr, right))
			{
				SetError(input, "Types do not match for '<='.");
			}

			expr = (Expression *)PushLessThanOrEqualToExpression(input->arena, expr, right);
		}
		else if(ReadTokenType(input, GreaterThanTokenId))
		{
			Expression *right = ReadSumLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '>'.");
			}

			if(!MatchExpressionTypesToAdd(input, expr, right))
			{
				SetError(input, "Types do not match for '>'.");
			}

			expr = (Expression *)PushGreaterThanExpression(input->arena, expr, right);
		}
		else if(ReadTokenType(input, GreaterThanOrEqualToTokenId))
		{
			Expression *right = ReadSumLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '>='.");
			}

			if(!MatchExpressionTypesToAdd(input, expr, right))
			{
				SetError(input, "Types do not match for '>='.");
			}

			expr = (Expression *)PushGreaterThanOrEqualToExpression(input->arena, expr, right);
		}
		else if(ReadTokenType(input, OrOrTokenId))
		{
			Expression *right = ReadCompareLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '||'.");
			}

			expr = (Expression *)PushOrExpression(input->arena, expr, right);
		}
		else if(ReadTokenType(input, AndAndTokenId))
		{
			Expression *right = ReadCompareLevelExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '&&'.");
			}

			expr = (Expression *)PushAndExpression(input->arena, expr, right);
		}
		else
		{
			break;
		}
	}

	Assert(expr);
	return expr;
}

static Expression * 
func ReadExpression(ParseInput *input)
{
	Expression *expr = ReadCompareLevelExpression(input);

	if(ReadTokenType(input, QuestionMarkTokenId))
	{
		Expression *condition = expr;
		if(!IsConditionExpression(condition))
		{
			SetError(input, "Expression is not condition.");
		}

		Expression *left = ReadCompareLevelExpression(input);
		if(!left)
		{
			SetError(input, "Expected expression after '?'.");
		}

		if(!ReadTokenType(input, ColonTokenId))
		{
			SetError(input, "Expected ':' after '?' and expression.");
		}

		Expression *right = ReadCompareLevelExpression(input);
		if(!right)
		{
			SetError(input, "Expected expression after ':'.");
		}

		if(!MatchExpressionTypes(input, left, right))
		{
			SetError(input, "Left and right expression types do not match for '?' ':' operator.");
		}

		expr = (Expression *)PushTernaryOperatorExpression(input->arena, condition, left, right);
	}
	
	Assert(expr);
	return expr;
}

enum tdef InstructionId
{
	NoInstructionId,

	AssignInstructionId,
	BlockInstructionId,
	BreakInstructionId,
	ContinueInstructionId,
	CreateMetaVariableInstructionId,
	CreateVariableInstructionId,
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
};

struct tdef Instruction
{
	InstructionId id;
	Instruction *next;
};

struct tdef BlockInstruction
{
	Instruction inst;

	Instruction *first;
};

struct tdef ForInstruction
{
	Instruction inst;

	Instruction *init;
	Expression *condition;
	Instruction *advance;
	BlockInstruction *body;
};

struct tdef IfInstruction
{
	Instruction instr;

	Expression *condition;
	BlockInstruction *body;
	Instruction *first_else;
};

struct tdef ElseInstruction
{
	Instruction instr;

	IfInstruction *if_instruction;
	BlockInstruction *body;
};

struct tdef WhileInstruction
{
	Instruction instr;

	Expression *condition;
	BlockInstruction *body;
};

struct tdef CreateVariableInstruction
{
	Instruction instr;

	Token var_name;
	VarType *type;
	Expression *init;
};

struct tdef CreateMetaVariableInstruction
{
	Instruction instr;
	
	Token var_name;
	VarType *type;
	Expression *expression;
};

struct tdef IncrementInstruction
{
	Instruction instr;

	Expression *expression;
};

struct tdef DecrementInstruction
{
	Instruction instr;

	Expression *expression;
};

struct tdef AssignInstruction
{
	Instruction instr;

	Expression *left;
	Expression *right;
};

struct tdef ReturnInstruction
{
	Instruction instr;

	Expression *value;
};

struct tdef OrEqualsInstruction
{
	Instruction instr;

	Expression *left;
	Expression *right;
};

struct tdef PlusEqualsInstruction
{
	Instruction instr;

	Expression *left;
	Expression *right;
};

struct tdef MinusEqualsInstruction
{
	Instruction instr;

	Expression *left;
	Expression *right;
};

struct tdef StarEqualsInstruction
{
	Instruction instr;

	Expression *left;
	Expression *right;
};

struct tdef FuncCallInstruction
{
	Instruction instr;

	FuncCallExpression *call_expression;
};

struct tdef UseInstruction
{
	Instruction instr;

	Expression *expression;
};

struct tdef BreakInstruction
{
	Instruction instr;
};

struct tdef ContinueInstruction
{
	Instruction instr;
};

struct tdef DeclInstruction
{
	Instruction instr;

	FuncHeader func_header;
};

static Instruction * decl ReadInstruction(ParseInput *input);
static BlockInstruction * decl ReadBlock(ParseInput *input);

static IncrementInstruction * 
func PushIncrementInstruction(MemArena *arena, Expression *expression)
{
	IncrementInstruction *result = ArenaPushType(arena, IncrementInstruction);
	result->instr.next = 0;
	result->instr.id = IncrementInstructionId;
	result->expression = expression;
	Assert(result != 0);
	return result;
}

static DecrementInstruction *
func PushDecrementInstruction(MemArena *arena, Expression *expression)
{
	DecrementInstruction *result = ArenaPushType(arena, DecrementInstruction);
	result->instr.next = 0;
	result->instr.id = DecrementInstructionId;
	result->expression = expression;
	Assert(result != 0);
	return result;
}

static Token global_zero_token = 
{
	IntegerConstantTokenId,
	"0",
	1
};

static Token global_for_iterator_name =
{
	NameTokenId,
	"_i",
	2
};

static Expression *
func GetArraySizeForIteration(Expression *expression, MemArena* arena)
{
	Assert(expression && expression->type->id == ArrayTypeId);
	ArrayType *array_type = (ArrayType *)expression->type;
	Expression *size_expression = array_type->size;
	Token meta_size_var_name = array_type->meta_size_var_name;
	if(meta_size_var_name.id != NoTokenId)
	{
		Assert(array_type->meta_size_var_name.id == NameTokenId);
		Assert(expression->id == StructVarExpressionId);
		StructVarExpression *struct_var_expression = (StructVarExpression *)expression;
		Expression *struct_expression = struct_var_expression->struct_expression;
		StructVarExpression *meta_size_expression = PushStructVarExpression(arena, struct_expression, 
																			meta_size_var_name);
		Assert(IsIntegerType(meta_size_expression->expr.type));
		size_expression = (Expression *)meta_size_expression;
	}

	return size_expression;
}

static BreakInstruction *
func ReadBreakInstruction(ParseInput *input)
{
	MemArena* arena = input->arena;
	BreakInstruction *instruction = 0;

	Assert(ReadTokenType(input, BreakTokenId));
	instruction = ArenaPushType(input->arena, BreakInstruction);
	instruction->instr.id = BreakInstructionId;
	instruction->instr.next = 0;
	return instruction;
}

static ContinueInstruction *
func ReadContinueInstruction(ParseInput *input)
{
	MemArena *arena = input->arena;
	ContinueInstruction *instruction = 0;

	Assert(ReadTokenType(input, ContinueTokenId));
	instruction = ArenaPushType(input->arena, ContinueInstruction);
	instruction->instr.id = ContinueInstructionId;
	instruction->instr.next = 0;
	return instruction;
}

static WhileInstruction *
func ReadWhileInstruction(ParseInput *input)
{
	MemArena *arena = input->arena;
	WhileInstruction *instruction = 0;
	Assert(ReadTokenType(input, WhileTokenId))
	StackState stack_state = GetStackState(input);
	Expression *condition = ReadExpression(input);
	BlockInstruction *body = ReadBlock(input);
		
	instruction = ArenaPushType(input->arena, WhileInstruction);
	instruction->instr.id = WhileInstructionId;
	instruction->instr.next = 0;

	instruction->condition = condition;
	instruction->body = body;

	SetStackState(input, stack_state);
	return instruction;
}

static ForInstruction * 
func ReadForInstruction(ParseInput *input)
{
	MemArena *arena = input->arena;
	ForInstruction *instruction = 0;

	Assert(ReadTokenType(input, ForTokenId));
	StackState stack_state = GetStackState(input);

	Instruction *init = 0;
	Expression *condition = 0;
	Instruction *advance = 0;
	BlockInstruction *body = 0;
	Instruction *block_prefix = 0;

	Token iterator_name = {};
	if(ReadTokenType(input, NameTokenId))
	{
		iterator_name = input->last_token;
		Assert(!VarExists(&input->var_stack, iterator_name));

		if(!ReadTokenType(input, ColonEqualsTokenId))
		{
			SetError(input, "Expected ':=' for 'for' iterator initialization.");
		}
	}
	else
	{
		SetError(input, "Expected name of 'for' iterator.");
	}

	if(ReadTokenType(input, OpenBracketsTokenId))
	{
		if(!ReadTokenType(input, CloseBracketsTokenId))
		{
			SetError(input, "Expected ']' after '[' for index-based ranged for loop.");
		}

		if(!ReadTokenType(input, InTokenId))
		{
			SetError(input, "Expected 'in' after '[]' for index-based ranged for loop.");
		}

		Expression *array_expression = ReadExpression(input);
		if(!array_expression || array_expression->type->id != ArrayTypeId)
		{
			SetError(input, "Expected array expression for index-based ranged for loop.");
		}

		Expression *array_size = GetArraySizeForIteration(array_expression, arena);
		ArrayType *array = (ArrayType *)array_expression->type;

		CreateVariableInstruction *create_variable = ArenaPushType(arena, CreateVariableInstruction);
		create_variable->instr.id = CreateVariableInstructionId;
		create_variable->instr.next = 0;

		create_variable->var_name = iterator_name;
		create_variable->type = array_size->type;
		create_variable->init = (Expression *)PushIntegerConstantExpression(input->arena, global_zero_token);

		init = (Instruction *)create_variable;

		Var var = {};
		var.name = create_variable->var_name;
		var.type = create_variable->type;
		PushVar(&input->var_stack, var);

		Expression *var_expression = (Expression *)PushVarExpression(arena, var);
		condition = (Expression *)PushLessThanExpression(arena, var_expression, array_size);

		advance = (Instruction *)PushIncrementInstruction(arena, var_expression);
	}
	else if(ReadTokenType(input, AtTokenId))
	{
		if(!ReadTokenType(input, InTokenId))
		{
			SetError(input, "Expected 'in' after '@' for address-based ranged for loop.");
		}

		Expression *array_expression = ReadExpression(input);
		if(!array_expression || array_expression->type->id != ArrayTypeId)
		{
			SetError(input, "Expected array expression for address-based ranged for loop.");
		}

		Expression *array_size = GetArraySizeForIteration(array_expression, arena);
		ArrayType *array = (ArrayType *)array_expression->type;

		CreateVariableInstruction *create_variable = ArenaPushType(arena, CreateVariableInstruction);
		create_variable->instr.id = CreateVariableInstructionId;
		create_variable->instr.next = 0;

		create_variable->var_name = global_for_iterator_name;
		create_variable->type = array_size->type;
		create_variable->init = (Expression *)PushIntegerConstantExpression(input->arena, global_zero_token);

		init = (Instruction *)create_variable;

		Var var = {};
		var.name = create_variable->var_name;
		var.type = create_variable->type;
		PushVar(&input->var_stack, var);

		Expression *var_expression = (Expression *)PushVarExpression(arena, var);
		condition = (Expression *)PushLessThanExpression(arena, var_expression, array_size);

		advance = (Instruction *)PushIncrementInstruction(arena, var_expression);

		CreateVariableInstruction *prefix = ArenaPushType(arena, CreateVariableInstruction);
		prefix->instr.id = CreateVariableInstructionId;
		prefix->instr.next = 0;

		prefix->var_name = iterator_name;
		prefix->type = (VarType *)PushPointerType(input->arena, array->element_type);
		Expression *array_index_expression = (Expression *)PushArrayIndexExpression(input->arena, 
																					array_expression, 
																					var_expression);
		prefix->init = (Expression *)PushAddressExpression(input->arena, array_index_expression);

		var.name = iterator_name;
		var.type = prefix->type;
		PushVar(&input->var_stack, var);

		block_prefix = (Instruction *)prefix;
	}
	else if(ReadTokenType(input, InTokenId))
	{
		Expression *array_expression = ReadExpression(input);
		if(!array_expression || array_expression->type->id != ArrayTypeId)
		{
			SetError(input, "Expected array expression for in for loop.");
		}

		Expression *array_size = GetArraySizeForIteration(array_expression, arena);
		ArrayType *array = (ArrayType *)array_expression->type;

		CreateVariableInstruction *create_variable = ArenaPushType(arena, CreateVariableInstruction);
		create_variable->instr.id = CreateVariableInstructionId;
		create_variable->instr.next = 0;

		create_variable->var_name = global_for_iterator_name;
		create_variable->type = array_size->type;
		create_variable->init = (Expression *)PushIntegerConstantExpression(input->arena, global_zero_token);

		init = (Instruction *)create_variable;

		Var var = {};
		var.name = create_variable->var_name;
		var.type = create_variable->type;
		PushVar(&input->var_stack, var);

		Expression *var_expression = (Expression *)PushVarExpression(arena, var);
		condition = (Expression *)PushLessThanExpression(arena, var_expression, array_size);

		advance = (Instruction *)PushIncrementInstruction(arena, var_expression);

		CreateVariableInstruction *prefix = ArenaPushType(arena, CreateVariableInstruction);
		prefix->instr.id = CreateVariableInstructionId;
		prefix->instr.next = 0;

		prefix->var_name = iterator_name;
		prefix->type = array->element_type;
		prefix->init = (Expression *)PushArrayIndexExpression(input->arena, array_expression, var_expression);

		var.name = iterator_name;
		var.type = prefix->type;
		PushVar(&input->var_stack, var);

		block_prefix = (Instruction *)prefix;
	}
	else
	{
		Expression *start_at = ReadExpression(input);
		if(!start_at)
		{
			SetError(input, "Expected for loop iterator start expression.");
		}

		CreateVariableInstruction *create_variable = ArenaPushType(arena, CreateVariableInstruction);
		create_variable->instr.id = CreateVariableInstructionId;
		create_variable->instr.next = 0;

		create_variable->var_name = iterator_name;
		create_variable->type = start_at->type;
		create_variable->init = start_at;

		init = (Instruction *)create_variable;
		Assert(init != 0);
		if(ReadTokenType(input, ToTokenId))
		{
			bool include_right = true;
			if(ReadTokenType(input, LessThanTokenId))
			{
				include_right = false;
			}

			Expression *increment_to = ReadExpression(input);
			if(!MatchExpressionTypes(input, create_variable->init, increment_to))
			{
				SetError(input, "Initial and final expression types for ranged-for loop do not match.");
			}
			else
			{
				create_variable->type = create_variable->init->type;
			}

			Var var = {};
			var.type = create_variable->type;
			var.name = create_variable->var_name;
			var.use_from = 0;
			PushVar(&input->var_stack, var);
			
			Expression *var_expression = (Expression *)PushVarExpression(arena, var);
			if(include_right)
			{
				condition = (Expression *)PushLessThanOrEqualToExpression(arena, var_expression, increment_to);
			}
			else
			{
				condition = (Expression *)PushLessThanExpression(arena, var_expression, increment_to);
			}
			advance = (Instruction *)PushIncrementInstruction(arena, var_expression);
		}
		else if(ReadTokenType(input, SemiColonTokenId))
		{
			Var var = {};
			var.type = create_variable->type;
			var.name = create_variable->var_name;
			var.use_from = 0;
			PushVar(&input->var_stack, var);

			condition = (Expression *)ReadExpression(input);
			if(!ReadTokenType(input, SemiColonTokenId))
			{
				SetError(input, "Expected ';' after 'for' condition.");
			}

			advance = (Instruction *)ReadInstruction(input);
		}
		else
		{
			SetError(input, "Expected 'to' or ';' after 'for' variable initialization.");
		}
	}

	body = ReadBlock(input);

	if(block_prefix)
	{
		block_prefix->next = body->first;
		body->first = block_prefix;
	}

	instruction = ArenaPushType(input->arena, ForInstruction);
	instruction->inst.next = 0;
	instruction->inst.id = ForInstructionId;
	instruction->init = init;
	instruction->condition = condition;
	instruction->advance = advance;
	instruction->body = body;

	SetStackState(input, stack_state);
	return instruction;
}

static IfInstruction * decl ReadIfInstruction(ParseInput *input);

static ElseInstruction *
func ReadElseInstruction(ParseInput *input)
{
	ElseInstruction *instruction = 0;
	Assert(ReadTokenType(input, ElseTokenId));
	
	IfInstruction *if_instruction = 0;
	BlockInstruction *body = 0;
	if(PeekToken(input, IfTokenId))
	{
		if_instruction = ReadIfInstruction(input);
	}
	else
	{
		body = ReadBlock(input);
	}

	instruction = ArenaPushType(input->arena, ElseInstruction);
	instruction->instr.id = ElseInstructionId;
	instruction->instr.next = 0;
	instruction->if_instruction = if_instruction;
	instruction->body = body;

	return instruction;
}

static IfInstruction * 
func ReadIfInstruction(ParseInput *input)
{
	MemArena *arena = input->arena;
	IfInstruction *instruction = 0;
	Assert(ReadTokenType(input, IfTokenId))
	StackState stack_state = GetStackState(input);

	Expression *condition = ReadExpression(input);
	BlockInstruction *body = ReadBlock(input);

	instruction = ArenaPushType(arena, IfInstruction);
	instruction->instr.id = IfInstructionId;
	instruction->instr.next = 0;
	instruction->condition = condition;
	instruction->body = body;

	SetStackState(input, stack_state);

	if(PeekToken(input, ElseTokenId))
	{
		instruction->first_else = (Instruction *)ReadElseInstruction(input);
	}
	else
	{
		instruction->first_else = 0;
	}

	return instruction;
}

static ReturnInstruction * 
func ReadReturnInstruction(ParseInput *input)
{
	ReturnInstruction *instruction = 0;
	Assert(ReadTokenType(input, ReturnTokenId));

	Expression *value = 0;
	if(!PeekToken(input, SemiColonTokenId))
	{
		value = ReadExpression(input);
	}

	instruction = ArenaPushType(input->arena, ReturnInstruction);
	instruction->instr.next = 0;
	instruction->instr.id = ReturnInstructionId;
	instruction->value = value;
	return instruction;
}

static bool 
func CanUseExpression(Expression *expression)
{
	bool can_use = false;
	if(expression->type->id == StructTypeId)
	{
		can_use = true;
	}
	else if(expression->type->id == PointerTypeId)
	{
		VarType *pointed_type = ((PointerType *)expression->type)->pointed_type;
		if(pointed_type->id == StructTypeId)
		{
			can_use = true;
		}
	}

	return can_use;
}

static void 
func UseExpression(ParseInput *input, Expression *expression)
{
	Assert(CanUseExpression(expression));
	if(IsPointerType(expression->type))
	{
		expression = (Expression *)PushDereferenceExpression(input->arena, expression);
	}
	
	VarStack *var_stack = &input->var_stack;
	MetaVarStack *meta_var_stack = &input->meta_var_stack;

	Assert(expression->type->id == StructTypeId);
	Struct *str = ((StructType *)expression->type)->str;
	StructVar *str_var = str->first_var;
	while(str_var)
	{
		Token name = str_var->name;
		if(VarExists(var_stack, name))
		{
			SetError(input, "Variable already exists.");
			break;
		}
		if(MetaVarExists(meta_var_stack, name))
		{
			SetError(input, "A meta variable with this name already exists.");
			break;
		}
		
		Var var = {};
		var.name = str_var->name;
		var.type = str_var->type;
		var.use_from = expression;
		PushVar(var_stack, var);

		str_var = str_var->next;
	}

	StructMetaVar *meta_var = str->first_meta_var;
	while(meta_var)
	{
		Token name = str_var->name;
		if(VarExists(var_stack, name))
		{
			SetError(input, "Variable already exists.");
			break;
		}
		if(MetaVarExists(meta_var_stack, name))
		{
			SetError(input, "A meta variable with this name already exists.");
			break;
		}

		MetaVar var = {};
		var.type = meta_var->expression->type;
		var.expression = meta_var->expression;
		var.use_from = expression;
		PushMetaVar(meta_var_stack, var);

		meta_var = meta_var->next;
	}
}

static UseInstruction * 
func ReadUseInstruction(ParseInput *input)
{
	MemArena *arena = input->arena;
	UseInstruction *instruction = 0;

	Assert(ReadTokenType(input, UseTokenId));
	Expression *expression = ReadExpression(input);
	if(!expression)
	{
		SetError(input, "Unknown expression for 'use' instruction.");
	}

	if(!CanUseExpression(expression))
	{
		SetError(input, "Cannot 'use' expression.");
	}

	instruction = ArenaPushType(arena, UseInstruction);
	instruction->instr.id = UseInstructionId;
	instruction->instr.next = 0;
	instruction->expression = expression;

	UseExpression(input, expression);
	return instruction;
}

static FuncHeader
func ReadFuncHeaderWithoutFuncKeyword(ParseInput *input)
{
	Token func_name = ReadToken(input);
	if(func_name.id != NameTokenId)
	{
		SetError(input, "Invalid function name.");
	}

	if(!ReadTokenType(input, OpenParenTokenId))
	{
		SetError(input, "Expected '(' after function name.");
	}

	FuncParam *first_param = 0;
	FuncParam *last_param = 0;
	while(true)
	{
		if(ReadTokenType(input, CloseParenTokenId))
		{
			break;
		}

		if(first_param != 0)
		{
			if(!ReadTokenType(input, CommaTokenId))
			{
				SetError(input, "Expected ',' between function parameters.");
			}
		}

		NameList name_list = ReadNameList(input);
		if(name_list.size == 0)
		{
			SetError(input, "Expected ',' or ')' after function parameter.");
		}

		if(!ReadTokenType(input, ColonTokenId))
		{
			SetError(input, "Expected ':' after function parameter name.");
		}

		VarType *param_type = ReadVarType(input);
		Assert(param_type != 0);
		if(VarTypeHasMetaData(param_type))
		{
			SetError(input, "Function parameter type cannot have meta data.");
		}

		for(int i = 0; i < name_list.size; i++)
		{
			Token param_name = name_list.names[i];
			if(VarExists(&input->var_stack, param_name))
			{
				SetError(input, "Variable already exists, cannot be used as function parameter name.");
			}

			FuncParam *param = ArenaPushType(input->arena, FuncParam);
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
			var.has_final_type = true;
			PushVar(&input->var_stack, var);
		}
	}

	VarType *return_type = 0;
	if(ReadTokenType(input, ColonTokenId))
	{
		return_type = ReadVarType(input);
		if(VarTypeHasMetaData(return_type))
		{
			SetError(input, "Function return type cannot have meta data.");
		}
	}

	FuncHeader header = {};
	header.namespace_name = input->namespace_name;
	header.name = func_name;
	header.first_param = first_param;
	header.return_type = return_type;
	return header;
}

static FuncHeader
func ReadFuncHeader(ParseInput *input)
{
	Assert(ReadTokenType(input, FuncTokenId));
	FuncHeader header = ReadFuncHeaderWithoutFuncKeyword(input);
	return header;
}

static DeclInstruction *
func ReadDeclInstruction(ParseInput *input)
{
	DeclInstruction *instruction = 0;
	Assert(ReadTokenType(input, DeclTokenId));

	if(PeekToken(input, FuncTokenId))
	{
		StackState stack_state = GetStackState(input);

		FuncHeader func_header = ReadFuncHeader(input);
		instruction = ArenaPushType(input->arena, DeclInstruction);
		instruction->instr.id = DeclInstructionId;
		instruction->instr.next = 0;
		instruction->func_header = func_header;

		SetStackState(input, stack_state);
	}
	else
	{
		SetError(input, "Unexpected token after 'decl' (expected 'func').");
	}

	return instruction;
}

static Instruction * 
func ReadInstruction(ParseInput *input)
{
	Instruction *instruction = 0;
	if(PeekToken(input, OpenBracesTokenId))
	{
		instruction = (Instruction *)ReadBlock(input);
	}
	else if(PeekToken(input, ReturnTokenId))
	{
		instruction = (Instruction *)ReadReturnInstruction(input);
	}
	else if(PeekToken(input, ForTokenId))
	{
		instruction = (Instruction *)ReadForInstruction(input);
	}
	else if(PeekToken(input, WhileTokenId))
	{
		instruction = (Instruction *)ReadWhileInstruction(input);
	}
	else if(PeekToken(input, IfTokenId))
	{
		instruction = (Instruction *)ReadIfInstruction(input);
	}
	else if(PeekToken(input, UseTokenId))
	{
		instruction = (Instruction *)ReadUseInstruction(input);
	}
	else if(PeekToken(input, DeclTokenId))
	{
		instruction = (Instruction *)ReadDeclInstruction(input);
	}
	else if(PeekToken(input, BreakTokenId))
	{
		instruction = (Instruction *)ReadBreakInstruction(input);
	}
	else if(PeekToken(input, ContinueTokenId))
	{
		instruction = (Instruction *)ReadContinueInstruction(input);
	}
	else if(PeekTwoTokens(input, NameTokenId, ColonTokenId))
	{
		Token var_name = ReadToken(input);
		Assert(var_name.id == NameTokenId);
		Assert(ReadTokenType(input, ColonTokenId));
		if(VarExists(&input->var_stack, var_name))
		{
			SetError(input, "Variable already exists.");
		}
		if(MetaVarExists(&input->meta_var_stack, var_name))
		{
			SetError(input, "A meta variable with this name already exists.");
		}

		VarType *var_type = ReadVarType(input);
		if(VarTypeHasMetaData(var_type))
		{
			SetError(input, "Locally defined variable cannot have meta data.");
		}

		Expression *init = 0;

		if(ReadTokenType(input, OpenParenTokenId))
		{
			if(var_type->id != StructTypeId)
			{
				SetError(input, "Only structs can be initialized with constructors.");
			}

			FuncCallParam *first_param = 0;
			FuncCallParam *last_param = 0;
			bool is_first = true;
			while(1)
			{
				if(ReadTokenType(input, CloseParenTokenId))
				{
					break;
				}

				if(!is_first)
				{
					if(!ReadTokenType(input, CommaTokenId))
					{
						SetError(input, "Expected ',' between call parameters.");
					}
				}
				is_first = false;

				Expression *value = ReadExpression(input);
				FuncCallParam *param = ArenaPushType(input->arena, FuncCallParam);
				param->expression = value;
				param->next = 0;

				if(!first_param)
				{
					Assert(!last_param);
					first_param = param;
					last_param = param;
				}
				else
				{
					Assert(last_param);
					last_param->next = param;
					last_param = param;
				}
			}

			Constructor *ctor = GetConstructor(&input->constructor_stack, first_param);
			if(!ctor)
			{
				SetError(input, "No constructor matches call parameters.");
			}

			init = (Expression *)PushConstructorCallExpression(input->arena, ctor, first_param);
		}

		CreateVariableInstruction *create_var = ArenaPushType(input->arena, CreateVariableInstruction);
		create_var->instr.next = 0;
		create_var->instr.id = CreateVariableInstructionId;
		create_var->var_name = var_name;
		create_var->init = init;
		create_var->type = var_type;

		Var var = {};
		var.name = var_name;
		var.type = var_type;
		var.has_final_type = true;
		var.create_instruction = create_var;
		PushVar(&input->var_stack, var);

		instruction = (Instruction *)create_var;
	}
	else if(PeekTwoTokens(input, NameTokenId, ColonEqualsTokenId))
	{
		Token var_name = ReadToken(input);
		Assert(var_name.id == NameTokenId);
		Assert(ReadTokenType(input, ColonEqualsTokenId));
		if(VarExists(&input->var_stack, var_name))
		{
			SetError(input, "Variable already exists.");
		}
		if(MetaVarExists(&input->meta_var_stack, var_name))
		{
			SetError(input, "A meta variable with this name already exists.");
		}

		Expression *init = ReadExpression(input);
		if(!init)
		{
			SetError(input, "Expected expression after ':='.");
		}

		if(!init->type)
		{
			SetError(input, "Expected expression for variable initialization.");
		}

		CreateVariableInstruction *create_var = ArenaPushType(input->arena, CreateVariableInstruction);
		create_var->instr.next = 0;
		create_var->instr.id = CreateVariableInstructionId;
		create_var->var_name = var_name;
		create_var->init = init;
		Assert(init->type != 0);
		VarType *var_type = init->type;
		create_var->type = var_type;
		instruction = (Instruction *)create_var;
		Assert(!VarExists(&input->var_stack, var_name));

		Var var = {};
		var.name = var_name;
		var.type = var_type;
		var.has_final_type = init->has_final_type;
		var.create_instruction = create_var;
		PushVar(&input->var_stack, var);
	}
	else if(PeekThreeTokens(input, PoundTokenId, NameTokenId, ColonEqualsTokenId))
	{
		Assert(ReadTokenType(input, PoundTokenId));
		Token var_name = ReadToken(input);
		Assert(var_name.id == NameTokenId);
		Assert(ReadTokenType(input, ColonEqualsTokenId));
		if(MetaVarExists(&input->meta_var_stack, var_name))
		{
			SetError(input, "Meta variable already exists.");
		}
		if(VarExists(&input->var_stack, var_name))
		{
			SetError(input, "A variable with this name already exists.");
		}
		
		Expression *expression = ReadExpression(input);
		if(!expression)
		{
			SetError(input, "Expected expression after ':='");
		}
		if(!expression->type)
		{
			SetError(input, "Expected expression for meta variable");
		}
		
		CreateMetaVariableInstruction *create_meta_var = ArenaPushType(input->arena, CreateMetaVariableInstruction);
		create_meta_var->instr.next = 0;
		create_meta_var->instr.id = CreateMetaVariableInstructionId;
		create_meta_var->var_name = var_name;
		create_meta_var->expression = expression;
		create_meta_var->type = expression->type;
		instruction = (Instruction *)create_meta_var;
		Assert(!MetaVarExists(&input->meta_var_stack, var_name));
		
		MetaVar var = {};
		var.name = var_name;
		var.type = expression->type;
		var.expression = expression;
		var.create_instruction = create_meta_var;
		PushMetaVar(&input->meta_var_stack, var);
	}
	else
	{
		Expression *expression = ReadExpression(input);
		if(expression->id == FuncCallExpressionId && expression->type == 0)
		{
			FuncCallInstruction *func_call = ArenaPushType(input->arena, FuncCallInstruction);
			func_call->instr.id = FuncCallInstructionId;
			func_call->instr.next = 0;
			func_call->call_expression = (FuncCallExpression *)expression;
			instruction = (Instruction *)func_call;
		}
		else if(ReadTokenType(input, EqualsTokenId))
		{
			Expression *right = ReadExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '='.");
			}

			if(!MatchExpressionTypes(input, expression, right))
			{
				SetError(input, "Unmatching types for '=' instruction.");
			}

			AssignInstruction *assign = ArenaPushType(input->arena, AssignInstruction);
			assign->instr.next = 0;
			assign->instr.id = AssignInstructionId;
			assign->left = expression;
			assign->right = right;
			instruction = (Instruction *)assign;
		}
		else if(ReadTokenType(input, PlusEqualsTokenId))
		{
			Expression *right = ReadExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '+='.");
			}

			if(!MatchExpressionTypes(input, expression, right))
			{
				SetError(input, "Unmatching types for '+=' instruction.");
			}

			if(!IsNumericalType(expression->type))
			{
				SetError(input, "Expected numerical expression before '+='.");
			}

			PlusEqualsInstruction *plus_equals = ArenaPushType(input->arena, PlusEqualsInstruction);
			plus_equals->instr.id = PlusEqualsInstructionId;
			plus_equals->instr.next = 0;
			plus_equals->left = expression;
			plus_equals->right = right;
			instruction = (Instruction *)plus_equals;
		}
		else if(ReadTokenType(input, MinusEqualsTokenId))
		{
			Expression *right = ReadExpression(input);
			if(!right)
			{
				SetError(input, "Expected expression after '-='.");
			}

			if(!MatchExpressionTypes(input, expression, right))
			{
				SetError(input, "Unmatching types for '-=' instruction.");
			}

			if(!IsNumericalType(expression->type))
			{
				SetError(input, "Expected numerical expression before '-='.");
			}

			MinusEqualsInstruction *minus_equals = ArenaPushType(input->arena, MinusEqualsInstruction);
			minus_equals->instr.id = MinusEqualsInstructionId;
			minus_equals->instr.next = 0;
			minus_equals->left = expression;
			minus_equals->right = right;
			instruction = (Instruction *)minus_equals;
		}
		else if(ReadTokenType(input, StarEqualsTokenId))
		{
			Expression *right = ReadExpression(input);
			if(!MatchExpressionTypes(input, expression, right))
			{
				SetError(input, "Unmatching types for '*=' instruction.");
			}

			if(!IsNumericalType(expression->type))
			{
				SetError(input, "Expected numerical type before '*='.");
			}

			StarEqualsInstruction *star_equals = ArenaPushType(input->arena, StarEqualsInstruction);
			star_equals->instr.id = StarEqualsInstructionId;
			star_equals->instr.next = 0;
			star_equals->left = expression;
			star_equals->right = right;
			instruction = (Instruction *)star_equals;
		}
		else if(ReadTokenType(input, OrEqualsTokenId))
		{
			Expression *right = ReadExpression(input);

			if(!MatchExpressionTypes(input, expression, right))
			{
				SetError(input, "Unmatching types for '|=' instruction.");
			}

			if(!IsNumericalType(expression->type))
			{
				SetError(input, "Expected numerical type before '|='.");
			}

			OrEqualsInstruction *or_equals = ArenaPushType(input->arena, OrEqualsInstruction);
			or_equals->instr.id = OrEqualsInstructionId;
			or_equals->instr.next = 0;
			or_equals->left = expression;
			or_equals->right = right;
			instruction = (Instruction *)or_equals;
		}
		else if(ReadTokenType(input, PlusPlusTokenId))
		{
			IncrementInstruction *increment = ArenaPushType(input->arena, IncrementInstruction);
			increment->instr.next = 0;
			increment->instr.id = IncrementInstructionId;
			increment->expression = expression;
			instruction = (Instruction *)increment;
		}
		else if(ReadTokenType(input, MinusMinusTokenId))
		{
			DecrementInstruction *decrement = ArenaPushType(input->arena, DecrementInstruction);
			decrement->instr.next = 0;
			decrement->instr.id = DecrementInstructionId;
			decrement->expression = expression;
			instruction = (Instruction *)decrement;
		}
		else
		{
			SetError(input, "Unexpected data after expression.");
		}
	}

	return instruction;
}

static bool 
func NeedsSemicolon(InstructionId id)
{
	bool needs_semicolon = false;
	switch(id)
	{
		case ForInstructionId:
		case IfInstructionId:
		case BlockInstructionId:
		case WhileInstructionId:
		{
			needs_semicolon = false;
			break;
		}
		default:
		{
			needs_semicolon= true;
			break;
		}
	}

	return needs_semicolon;
}

static BlockInstruction * 
func ReadBlock(ParseInput *input)
{
	BlockInstruction *block = 0;
	Assert(ReadTokenType(input, OpenBracesTokenId));

	StackState stack_state = GetStackState(input);

	Instruction *first_instruction = 0;
	Instruction *last_instruction = 0;
	while(true)
	{
		if(ReadTokenType(input, CloseBracesTokenId))
		{
			break;
		}

		Instruction *instruction = ReadInstruction(input);
		Assert(instruction);
		if(NeedsSemicolon(instruction->id))
		{
			if(!ReadTokenType(input, SemiColonTokenId))
			{
				SetError(input, "Expected ';' after instruction.");
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

	block = ArenaPushType(input->arena, BlockInstruction);
	block->inst.id = BlockInstructionId;
	block->inst.next = 0;
	block->first = first_instruction;

	SetStackState(input, stack_state);

	return block;
}

static Func *
func ReadFunc(ParseInput *input)
{
	VarStack *var_stack = &input->var_stack;
	Func *f = 0;
	if(PeekToken(input, FuncTokenId))
	{
		StackState stack_state = GetStackState(input);

		FuncHeader header = ReadFuncHeader(input);

		BlockInstruction *body = 0;
		if(ReadTokenType(input, SemiColonTokenId))
		{
			body = 0;
		}
		else
		{
			body = ReadBlock(input);
			if(!body)
			{
				SetError(input, "Function doesn't have a body.");
			}
		}

		f = ArenaPushType(input->arena, Func);
		f->header = header;
		f->body = body;

		SetStackState(input, stack_state);
	}

	return f;
}

static Enum *
func ReadEnum(ParseInput *input)
{
	Assert(ReadTokenType(input, EnumTokenId));

	if(!ReadTokenType(input, NameTokenId))
	{
		Assert("Expected enum name!");
	}
	Token name = input->last_token;

	if(!ReadTokenType(input, OpenBracesTokenId))
	{
		Assert("Expected '{' after enum name!");
	}

	Token *first_token = ArenaPushArray(input->arena, 0, Token);
	Token *last_token = first_token;
	int size = 0;

	bool first = true;
	while(1)
	{
		if(ReadTokenType(input, CloseBracesTokenId))
		{
			break;
		}

		if(!first)
		{
			if(!ReadTokenType(input, CommaTokenId))
			{
				SetError(input, "Expected ',' between enum members.");
			}
		}
		first = false;

		if(!ReadTokenType(input, NameTokenId))
		{
			SetError(input, "Invalid enum member.");
		}
		Token *pushed_token = ArenaPushType(input->arena, Token);
		*pushed_token = input->last_token;

		Assert(pushed_token == last_token);
		last_token++;

		size++;
	}

	Enum *e = ArenaPushType(input->arena, Enum);
	e->name = name;
	e->namespace_name = input->namespace_name;
	e->tokens = first_token;
	e->size = size;
	return e;
}

static bool
func IsValidOperatorToken(Token token)
{
	bool is_valid = IsValidOperatorTokenId(token.id);
	return is_valid;
}

static Operator *
func ReadOperator(ParseInput *input)
{
	StackState stack_state = GetStackState(input);

	Assert(ReadTokenType(input, OperatorTokenId));
	
	if(!ReadTokenType(input, NameTokenId))
	{
		SetError(input, "Expected name of left operand!");
	}
	Token left_name = input->last_token;

	if(!ReadTokenType(input, ColonTokenId))
	{
		SetError(input, "Expected ':' after left operand name!");
	}
	VarType *left_type = ReadVarType(input);

	Var left_var = {};
	left_var.name = left_name;
	left_var.type = left_type;
	left_var.has_final_type = true;
	PushVar(&input->var_stack, left_var);

	Token token = ReadToken(input);
	if(!IsValidOperatorToken(token))
	{
		SetError(input, "Invalid operator token");
	}

	if(!ReadTokenType(input, NameTokenId))
	{
		SetError(input, "Expected name of right operand!");
	}
	Token right_name = input->last_token;

	if(!ReadTokenType(input, ColonTokenId))
	{
		SetError(input, "Expected ':' after right operand name!");
	}
	VarType *right_type = ReadVarType(input);

	Var right_var = {};
	right_var.name = right_name;
	right_var.type = right_type;
	right_var.has_final_type = true;
	PushVar(&input->var_stack, right_var);

	if(!ReadTokenType(input, ColonTokenId))
	{
		SetError(input, "Expected ':' after right operand!");
	}
	VarType *return_type = ReadVarType(input);

	BlockInstruction *body = ReadBlock(input);

	SetStackState(input, stack_state);

	Operator *op = ArenaPushType(input->arena, Operator);
	op->left_name = left_name;
	op->right_name = right_name;
	op->left_type = left_type;
	op->right_type = right_type;
	op->token = token;
	op->return_type = return_type;
	op->body = body;

	return op;
}

static Constructor *
func ReadConstructor(ParseInput *input)
{
	StackState stack_state = GetStackState(input);

	Assert(ReadTokenType(input, ConstructorTokenId));

	FuncHeader header = ReadFuncHeaderWithoutFuncKeyword(input);
	Struct *str = GetStruct(&input->struct_stack, header.name);
	if(!str)
	{
		SetError(input, "Invalid struct name in constructor definition.");
	}
	VarType *type = (VarType *)PushStructType(input->arena, str);

	if(header.return_type)
	{
		if(!TypesEqual(type, header.return_type))
		{
			SetError(input, "Struct of constructor does not match return type.");
		}
	}
	else
	{
		header.return_type = type;
	}

	header.name = CreatePrefixToken(input, "_constructor_", str->name);

	BlockInstruction *block = ReadBlock(input);

	Constructor *ctor = ArenaPushType(input->arena, Constructor);
	ctor->type = type;
	ctor->header = header;
	ctor->body = block;

	SetStackState(input, stack_state);

	return ctor;
}

static Struct * 
func ReadStruct(ParseInput *input)
{
	Assert(ReadTokenType(input, StructTokenId));

	Struct *result = ArenaPushType(input->arena, Struct);
	*result = {};
	Assert(!input->in_struct_definition);
	input->in_struct_definition = result;

	Token name = ReadToken(input);
	if(name.id != NameTokenId)
	{
		SetError(input, "Invalid struct name.");
	}

	if(!ReadTokenType(input, OpenBracesTokenId))
	{
		SetError(input, "Expected '{' after struct name.");
	}

	StructVar *last_var = 0;
	StructMetaVar *last_meta_var = 0;
	while(true)
	{
		if(ReadTokenType(input, CloseBracesTokenId))
		{
			break;
		}

		if(ReadTokenType(input, PoundTokenId))
		{
			if(!ReadTokenType(input, NameTokenId))
			{
				SetError(input, "Expected struct meta variable name.");
			}
			Token name = input->last_token;

			if(!ReadTokenType(input, ColonEqualsTokenId))
			{
				SetError(input, "Expected ':=' after struct meta variable name.");
			}

			Expression *expression = ReadExpression(input);
			if(!expression)
			{
				SetError(input, "Expected expression for struct meta variable.");
			}

			if(!ReadTokenType(input, SemiColonTokenId))
			{
				SetError(input, "Expected ';' after struct meta variable definition.");
			}

			StructMetaVar *var = ArenaPushType(input->arena, StructMetaVar);
			var->next = 0;
			var->name = name;
			var->expression = expression;

			if(!last_meta_var)
			{
				Assert(!result->first_meta_var);
				result->first_meta_var = var;
				last_meta_var = var;
			}
			else
			{
				Assert(result->first_meta_var);
				last_meta_var->next = var;
				last_meta_var = var;
			}
		}
		else
		{
			NameList var_name_list = ReadNameList(input);
			if(var_name_list.size == 0)
			{
				SetError(input, "Expected struct member name or '}'.");
			}

			if(!ReadTokenType(input, ColonTokenId))
			{
				if(var_name_list.size == 1)
				{
					SetError(input, "Expected ':' after struct member name.");
				}
				else
				{
					SetError(input, "Expected ':' after struct member names.");
				}
				break;
			}

			VarType *var_type = ReadVarType(input);
			if(!var_type)
			{
				if(var_name_list.size == 1)
				{
					SetError(input, "Expected var type for struct member.");
				}
				else
				{
					SetError(input, "Expected var type for struct members.");
				}
			}

			if(!ReadTokenType(input, SemiColonTokenId))
			{
				SetError(input, "Expected ';' after struct member definition.");
			}

			for(int i = 0; i < var_name_list.size; i++)
			{
				StructVar *var = ArenaPushType(input->arena, StructVar);
				var->next = 0;
				var->name = var_name_list.names[i];
				var->type = var_type;

				if(!last_var)
				{
					Assert(!result->first_var);
					result->first_var = var;
					last_var = var;
				}
				else
				{
					Assert(result->first_var);
					last_var->next = var;
					last_var = var;
				}
			}
		}
	}

	result->name = name;
	result->namespace_name = input->namespace_name;

	StructVar *struct_var = result->first_var;
	while(struct_var)
	{
		VarType *type = struct_var->type;
		while(type->id == PointerTypeId)
		{
			type = ((PointerType *)type)->pointed_type;
		}

		if(type->id == ArrayTypeId)
		{
			ArrayType *array_type = (ArrayType *)type;
			Token meta_size_var_name = array_type->meta_size_var_name;
			if(meta_size_var_name.id != NoTokenId)
			{
				Assert(meta_size_var_name.id == NameTokenId);
				if(!StructHasVar(result, meta_size_var_name))
				{
					SetError(input, "Unknown struct member name for meta size variable.");
				}

				StructVar *meta_size_var = GetStructVar(result, meta_size_var_name);
				if(!IsIntegerType(meta_size_var->type))
				{
					SetError(input, "Non-integral meta size struct member.");
				}
			}
		}

		struct_var = struct_var->next;
	}

	input->in_struct_definition = 0;

	return result;
}

static bool
func MatchVarWithType(Var *var, VarType *type)
{
	bool can_match = false;
	if(var->has_final_type)
	{
		can_match = TypesEqual(var->type, type);
	}
	else
	{
		VarType *matching_type = GetMatchingType(var->type, type);
		if(TypesEqual(matching_type, type))
		{
			if(!TypesEqual(var->type, type))
			{
				Assert(true);
			}

			var->type = type;

			if(var->create_instruction)
			{
				var->create_instruction->type = type;
				var->has_final_type = true;
			}

			can_match = true;
		}
	}

	return can_match;
}

struct tdef Output
{
	MemArena *arena;
	Expression *in_struct;
	int tabs;
};

static void 
func WriteChar(Output *output, char c)
{
	char *mem = ArenaPushType(output->arena, char);
	*mem = c;
}

static void 
func WriteTabs(Output *output)
{
	Assert(output->tabs >= 0);
	for(int i = 0; i < output->tabs; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			WriteChar(output, ' ');
		}
	}
}

static void 
func WriteString(Output *output, char* string)
{
	for(int i = 0; string[i]; i++)
	{
		WriteChar(output, string[i]);
	}
}

static void 
func WriteToken(Output *output, Token token)
{
	for(int i = 0; i < token.length; i++)
	{
		WriteChar(output, token.text[i]);
	}
}

static void decl WriteExpression(Output *output, Expression *expression);

static void 
func WriteType(Output *output, VarType *type)
{
	Assert(type != 0);
	switch(type->id)
	{
		case BaseTypeId:
		{
			BaseType *base_type = (BaseType *)type;
			switch(base_type->base_id)
			{
				case Int8BaseTypeId:
				{
					WriteString(output, "char");
					break;
				}
				case Int32BaseTypeId:
				{
					WriteString(output, "int");
					break;
				}
				case UInt8BaseTypeId:
				{
					WriteString(output, "unsigned char");
					break;
				}
				case UInt32BaseTypeId:
				{
					WriteString(output, "unsigned int");
					break;
				}
				case Real32BaseTypeId:
				{
					WriteString(output, "float");
					break;
				}
				case Bool32BaseTypeId:
				{
					WriteString(output, "Bool32");
					break;
				}
				default:
				{
					DebugBreak();
				}
			}
			break;
		}
		case PointerTypeId:
		{
			PointerType *pointer_type = (PointerType *)type;
			WriteType(output, pointer_type->pointed_type);
			WriteString(output, " *");
			break;
		}
		case StructTypeId:
		{
			StructType *struct_type = (StructType *)type;
			WriteToken(output, struct_type->str->namespace_name);
			WriteToken(output, struct_type->str->name);
			break;
		}
		case ArrayTypeId:
		{
			ArrayType *array_type = (ArrayType *)type;
			WriteString(output, "(");
			WriteType(output, array_type->element_type);
			WriteString(output, ")[");
			WriteExpression(output, array_type->size);
			WriteString(output, "]");
			break;
		}
		case EnumTypeId:
		{
			EnumType *enum_type = (EnumType *)type;
			WriteToken(output, enum_type->e->namespace_name);
			WriteToken(output, enum_type->e->name);
			break;
		}
		default:
		{
			DebugBreak();
		}
	}
}

static void decl WriteExpression(Output *output, Expression *expression);

static void 
func WriteVar(Output *output, Var var)
{
	if(var.use_from)
	{
		WriteExpression(output, var.use_from);
		WriteString(output, ".");
	}

	WriteToken(output, var.name);
}

static char *
func GetOperatorFuncName(TokenId token_id)
{
	Assert(IsValidOperatorTokenId(token_id));

	char *name = 0;
	if(token_id == PlusTokenId)
	{
		name = "_plus";
	}
	else if(token_id == MinusTokenId)
	{
		name = "_minus";
	}
	else if(token_id == StarTokenId)
	{
		name = "_star";
	}
	else if(token_id == EqualsEqualsTokenId)
	{
		name = "_equal";
	}

	Assert(name);
	return name;
}

static void
func WriteIntegerConstantAsType(Output *output, Token token, VarType *type)
{
	Assert(IsNumericalType(type));
	Assert(token.id == IntegerConstantTokenId);

	if(IsIntegerType(type))
	{
		WriteToken(output, token);
	}
	else
	{
		Assert(IsRealType(type));
		WriteToken(output, token);
		WriteString(output, ".0f");
	}
}

static void 
func WriteExpression(Output *output, Expression *expression)
{
	switch(expression->id)
	{
		case IntegerConstantExpressionId:
		{
			IntegerConstantExpression *expr = (IntegerConstantExpression *)expression;
			WriteIntegerConstantAsType(output, expr->token, expression->type);
			break;
		}
		case CharacterConstantExpressionId:
		{
			CharacterConstantExpression *expr = (CharacterConstantExpression *)expression;
			WriteToken(output, expr->token);
			break;
		}
		case RealConstantExpressionId:
		{
			RealConstantExpression *expr = (RealConstantExpression *)expression;
			WriteToken(output, expr->token);
			WriteString(output, "f");
			break;
		}
		case VarExpressionId:
		{
			VarExpression *expr = (VarExpression *)expression;
			WriteVar(output, expr->var);
			break;
		}
		case MetaVarExpressionId:
		{
			MetaVarExpression *expr = (MetaVarExpression *)expression;
			WriteExpression(output, expr->var.expression);
			break;
		}
		case EqualExpressionId:
		{
			EqualExpression *expr = (EqualExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " == ");
			WriteExpression(output, expr->right);
			break;
		}
		case NotEqualExpressionId:
		{
			NotEqualExpression *expr = (NotEqualExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " != ");
			WriteExpression(output, expr->right);
			break;
		}
		case LessThanExpressionId:
		{
			LessThanExpression *expr = (LessThanExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " < ");
			WriteExpression(output, expr->right);
			break;
		}
		case GreaterThanExpressionId:
		{
			GreaterThanExpression *expr = (GreaterThanExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " > ");
			WriteExpression(output, expr->right);
			break;
		}
		case GreaterThanOrEqualToExpressionId:
		{
			GreaterThanOrEqualToExpression *expr = (GreaterThanOrEqualToExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " >= ");
			WriteExpression(output, expr->right);
			break;
		}
		case LessThanOrEqualToExpressionId:
		{
			LessThanOrEqualToExpression *expr = (LessThanOrEqualToExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " <= ");
			WriteExpression(output, expr->right);
			break;
		}
		case DereferenceExpressionId:
		{
			DereferenceExpression *expr = (DereferenceExpression *)expression;
			WriteString(output, "(*");
			WriteExpression(output, expr->base);
			WriteString(output, ")");
			break;
		}
		case AddExpressionId:
		{
			AddExpression *expr = (AddExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " + ");
			WriteExpression(output, expr->right);
			break;
		}
		case SubtractExpressionId:
		{
			SubtractExpression *expr = (SubtractExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " - ");
			WriteExpression(output, expr->right);
			break;
		}
		case ProductExpressionId:
		{
			ProductExpression *expr = (ProductExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " * ");
			WriteExpression(output, expr->right);
			break;
		}
		case DivideExpressionId:
		{
			DivideExpression *expr = (DivideExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " / ");
			WriteExpression(output, expr->right);
			break;
		}
		case ParenExpressionId:
		{
			ParenExpression *expr = (ParenExpression *)expression;
			WriteString(output, "(");
			WriteExpression(output, expr->in);
			WriteString(output, ")");
			break;
		}
		case StructVarExpressionId:
		{
			StructVarExpression *expr = (StructVarExpression *)expression;
			WriteExpression(output, expr->struct_expression);
			WriteString(output, ".");
			WriteToken(output, expr->var_name);
			break;
		}
		case StructDefVarExpressionId:
		{
			Assert(output->in_struct);
			StructDefVarExpression *expr = (StructDefVarExpression *)expression;
			{
				Assert(output->in_struct->type->id == StructTypeId);
				StructType *struct_type = (StructType *)output->in_struct->type;
				Assert(struct_type->str == expr->struct_def);
			}

			WriteExpression(output, output->in_struct);
			WriteString(output, ".");
			WriteToken(output, expr->var_name);
			break;
		}
		case StructMetaVarExpressionId:
		{
			StructMetaVarExpression *expr = (StructMetaVarExpression *)expression;

			Assert(output->in_struct == 0);
			output->in_struct = expr->struct_expression;

			WriteExpression(output, expr->meta_expression);

			Assert(output->in_struct == expr->struct_expression);
			output->in_struct = 0;
			break;
		}
		case FuncCallExpressionId:
		{
			FuncCallExpression *expr = (FuncCallExpression *)expression;
			Func *f = expr->f;
			FuncParam *param = f->header.first_param;
			FuncCallParam *call_param = expr->first_param;

			WriteToken(output, f->header.namespace_name);
			WriteToken(output, f->header.name);
			WriteString(output, "(");

			bool is_first_param = true;

			while(param)
			{
				if(!is_first_param)
				{
					WriteString(output, ", ");
				}
				is_first_param = false;

				Assert(call_param != 0 && call_param->expression != 0);
				Assert(TypesEqual(param->type, call_param->expression->type));

				WriteExpression(output, call_param->expression);

				param = param->next;
				call_param = call_param->next;
			}

			WriteString(output, ")");
			break;
		}
		case CastExpressionId:
		{
			CastExpression *expr = (CastExpression *)expression;
			WriteString(output, "(");
			WriteType(output, expr->type);
			WriteString(output, ")(");
			WriteExpression(output, expr->expression);
			WriteString(output, ")");
			break;
		}
		case LeftShiftExpressionId:
		{
			LeftShiftExpression *expr = (LeftShiftExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " << ");
			WriteExpression(output, expr->right);
			break;
		}
		case RightShiftExpressionId:
		{
			RightShiftExpression *expr = (RightShiftExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " >> ");
			WriteExpression(output, expr->right);
			break;
		}
		case TernaryOperatorExpressionId:
		{
			TernaryOperatorExpression *expr = (TernaryOperatorExpression *)expression;
			WriteExpression(output, expr->condition);
			WriteString(output, " ? ");
			WriteExpression(output, expr->left);
			WriteString(output, " : ");
			WriteExpression(output, expr->right);
			break;
		}
		case AddressExpressionId:
		{
			AddressExpression *expr = (AddressExpression *)expression;
			WriteString(output, "&");
			WriteExpression(output, expr->addressed);
			break;
		}
		case OrExpressionId:
		{
			OrExpression *expr = (OrExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " || ");
			WriteExpression(output, expr->right);
			break;
		}
		case AndExpressionId:
		{
			AndExpression *expr = (AndExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " && ");
			WriteExpression(output, expr->right);
			break;
		}
		case InvertExpressionId:
		{
			InvertExpression *expr = (InvertExpression *)expression;
			WriteString(output, "-");
			WriteExpression(output, expr->inverted);
			break;
		}
		case BoolConstantExpressionId:
		{
			BoolConstantExpression *expr = (BoolConstantExpression *)expression;
			if(expr->value)
			{
				WriteString(output, "true");
			}
			else
			{
				WriteString(output, "false");
			}
			break;
		}
		case NegateExpressionId:
		{
			NegateExpression *expr = (NegateExpression *)expression;
			WriteString(output, "!");
			WriteExpression(output, expr->negated);
			break;
		}
		case BitAndExpressionId:
		{
			BitAndExpression *expr = (BitAndExpression *)expression;
			WriteExpression(output, expr->left);
			WriteString(output, " & ");
			WriteExpression(output, expr->right);
			break;
		}
		case ArrayIndexExpressionId:
		{
			ArrayIndexExpression *expr = (ArrayIndexExpression *)expression;
			WriteString(output, "(");
			WriteExpression(output, expr->array);
			WriteString(output, ")[");
			WriteExpression(output, expr->index);
			WriteString(output, "]");
			break;
		}
		case OperatorCallExpressionId:
		{
			OperatorCallExpression *expr = (OperatorCallExpression *)expression;
			char *name = GetOperatorFuncName(expr->op->token.id);
			WriteString(output, name);
			WriteString(output, "(");
			WriteExpression(output, expr->left);
			WriteString(output, ", ");
			WriteExpression(output, expr->right);
			WriteString(output, ")");
			break;
		}
		case EnumMemberExpressionId:
		{
			EnumMemberExpression *expr = (EnumMemberExpression *)expression;
			EnumMember member = expr->member;
			Token name = member.e->tokens[member.index];
			WriteToken(output, name);
			break;
		}
		case ConstructorCallExpressionId:
		{
			ConstructorCallExpression *expr = (ConstructorCallExpression *)expression;
			FuncCallParam *call_param = expr->first_param;

			WriteToken(output, expr->ctor->header.namespace_name);
			WriteToken(output, expr->ctor->header.name);
			WriteString(output, "(");

			bool is_first_param = true;

			while(call_param)
			{
				if(!is_first_param)
				{
					WriteString(output, ", ");
				}
				is_first_param = false;

				WriteExpression(output, call_param->expression);
				call_param = call_param->next;
			}

			WriteString(output, ")");
			break;
		}
		case StringConstantExpressionId:
		{
			StringConstantExpression *expr = (StringConstantExpression *)expression;
			WriteToken(output, expr->token);
			break;
		}
		default:
		{
			DebugBreak();
		}
	}
}

static void 
func WriteTypeAndVar(Output *output, VarType *type, Token var_name)
{
	VarType *non_array_type = type;
	while(non_array_type->id == ArrayTypeId)
	{
		non_array_type = ((ArrayType *)non_array_type)->element_type;
	}
	
	WriteType(output, non_array_type);
	if(non_array_type->id != PointerTypeId)
	{
		WriteString(output, " ");
	}
	WriteToken(output, var_name);

	while(type->id == ArrayTypeId)
	{
		ArrayType *array_type = (ArrayType *)type;
		WriteString(output, "[");
		WriteExpression(output, array_type->size);
		WriteString(output, "]");
		type = array_type->element_type;
	}
}


static void decl WriteBlock(Output *output, BlockInstruction *block);
static void decl WriteFuncHeader(Output *output, FuncHeader *header);

static void
func WriteInstruction(Output *output, Instruction *instruction)
{
	switch(instruction->id)
	{
		case BlockInstructionId:
		{
			WriteBlock(output, (BlockInstruction *)instruction);
			break;
		}
		case ForInstructionId:
		{
			ForInstruction *for_instruction = (ForInstruction *)instruction;
			WriteString(output, "for(");
			WriteInstruction(output, for_instruction->init);
			WriteString(output, "; ");
			WriteExpression(output, for_instruction->condition);
			WriteString(output, "; ");
			WriteInstruction(output, for_instruction->advance);
			WriteString(output, ")\n");

			WriteTabs(output);
			WriteBlock(output, for_instruction->body);
			break;
		}
		case CreateVariableInstructionId:
		{
			CreateVariableInstruction *create_var = (CreateVariableInstruction *)instruction;
			WriteTypeAndVar(output, create_var->type, create_var->var_name);
			WriteString(output, " = ");
			if(create_var->init)
			{
				WriteExpression(output, create_var->init);
			}
			else
			{
				WriteString(output, "{}");
			}
			break;
		}
		case CreateMetaVariableInstructionId:
		{
			break;
		}
		case IncrementInstructionId:
		{
			IncrementInstruction *increment = (IncrementInstruction *)instruction;
			WriteExpression(output, increment->expression);
			WriteString(output, "++");
			break;
		}
		case DecrementInstructionId:
		{
			DecrementInstruction *decrement = (DecrementInstruction *)instruction;
			WriteExpression(output, decrement->expression);
			WriteString(output, "--");
			break;
		}
		case AssignInstructionId:
		{
			AssignInstruction *assign = (AssignInstruction *)instruction;
			WriteExpression(output, assign->left);
			WriteString(output, " = ");
			WriteExpression(output, assign->right);
			break;
		}
		case ReturnInstructionId:
		{
			ReturnInstruction *return_instruction = (ReturnInstruction *)instruction;
			WriteString(output, "return");
			if(return_instruction->value)
			{
				WriteString(output, " ");
				WriteExpression(output, return_instruction->value);
			}
			break;
		}
		case OrEqualsInstructionId:
		{
			OrEqualsInstruction *or_equals = (OrEqualsInstruction *)instruction;
			WriteExpression(output, or_equals->left);
			WriteString(output, " |= ");
			WriteExpression(output, or_equals->right);
			break;
		}
		case FuncCallInstructionId:
		{
			FuncCallInstruction *func_call = (FuncCallInstruction *)instruction;
			WriteExpression(output, (Expression *)func_call->call_expression);
			break;
		}
		case PlusEqualsInstructionId:
		{
			PlusEqualsInstruction *plus_equals = (PlusEqualsInstruction *)instruction;
			WriteExpression(output, plus_equals->left);
			WriteString(output, " += ");
			WriteExpression(output, plus_equals->right);
			break;
		}
		case MinusEqualsInstructionId:
		{
			MinusEqualsInstruction *minus_equals = (MinusEqualsInstruction *)instruction;
			WriteExpression(output, minus_equals->left);
			WriteString(output, " -= ");
			WriteExpression(output, minus_equals->right);
			break;
		}
		case StarEqualsInstructionId:
		{
			StarEqualsInstruction *star_equals = (StarEqualsInstruction *)instruction;
			WriteExpression(output, star_equals->left);
			WriteString(output, " *= ");
			WriteExpression(output, star_equals->right);
			break;
		}
		case IfInstructionId:
		{
			IfInstruction *if_instruction = (IfInstruction *)instruction;
			WriteString(output, "if(");
			WriteExpression(output, if_instruction->condition);
			WriteString(output, ")\n");

			WriteTabs(output);
			WriteBlock(output, if_instruction->body);

			if(if_instruction->first_else)
			{
				WriteTabs(output);
				WriteInstruction(output, if_instruction->first_else);
			}

			break;
		}
		case ElseInstructionId:
		{
			ElseInstruction *else_instruction = (ElseInstruction *)instruction;
			WriteString(output, "else");

			if(else_instruction->if_instruction)
			{
				WriteString(output, " ");
				Assert(!else_instruction->body);
				WriteInstruction(output, (Instruction *)else_instruction->if_instruction);
			}
			else if(else_instruction->body)
			{
				WriteString(output, "\n");
				WriteTabs(output);
				WriteBlock(output, else_instruction->body);
			}
			else
			{
				DebugBreak();
			}
			break;
		}
		case WhileInstructionId:
		{
			WhileInstruction *while_instruction = (WhileInstruction *)instruction;
			WriteString(output, "while(");
			WriteExpression(output, while_instruction->condition);
			WriteString(output, ")\n");

			WriteTabs(output);
			WriteBlock(output, while_instruction->body);
			break;
		}
		case BreakInstructionId:
		{
			WriteString(output, "break");
			break;
		}
		case ContinueInstructionId:
		{
			WriteString(output, "continue");
			break;
		}
		case UseInstructionId:
		{
			break;
		}
		case DeclInstructionId:
		{
			DeclInstruction *decl_instruction = (DeclInstruction *)instruction;
			WriteFuncHeader(output, &decl_instruction->func_header);
			break;
		}
		default:
		{
			DebugBreak();
		}
	}
}

static bool 
func IsPreprocessorInstruction(Instruction *instruction)
{
	bool result = false;
	switch(instruction->id)
	{
		case CreateMetaVariableInstructionId:
		case UseInstructionId:
		{
			result = true;
			break;
		}
		default:
		{
			result = false;
			break;
		}
	}

	return result;
}

static void 
func WriteBlock(Output *output, BlockInstruction *block)
{
	Assert(block != 0);

	WriteString(output, "{\n");
	output->tabs++;

	Instruction *instruction = block->first;
	while(instruction)
	{
		if(!IsPreprocessorInstruction(instruction))
		{
			WriteTabs(output);
			WriteInstruction(output, instruction);
			if(NeedsSemicolon(instruction->id))
			{
				WriteString(output, ";\n");
			}
		}
		instruction = instruction->next;
	}

	output->tabs--;
	WriteTabs(output);
	WriteString(output, "}\n");
}

static void
func WriteFuncHeader(Output* output, FuncHeader* header)
{
	if(header->return_type)
	{
		WriteType(output, header->return_type);
		if(header->return_type->id != PointerTypeId)
		{
			WriteString(output, " ");
		}
	}
	else
	{
		WriteString(output, "void ");
	}

	WriteToken(output, header->namespace_name);
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
func WriteFunc(Output *output, Func *f)
{
	WriteFuncHeader(output, &f->header);

	WriteTabs(output);
	WriteString(output, "\n");
	WriteBlock(output, f->body);
}

static void
func WriteOperator(Output *output, Operator *op)
{
	WriteType(output, op->return_type);

	if(op->return_type->id != PointerTypeId)
	{
		WriteString(output, " ");
	}
	char *name = GetOperatorFuncName(op->token.id);
	WriteString(output, name);

	WriteString(output, "(");
	WriteTypeAndVar(output, op->left_type, op->left_name);
	WriteString(output, ", ");
	WriteTypeAndVar(output, op->right_type, op->right_name);
	WriteString(output, ")\n");

	WriteTabs(output);
	WriteBlock(output, op->body);
}

static void
func WriteConstructor(Output *output, Constructor *ctor)
{
	WriteFuncHeader(output, &ctor->header);

	WriteTabs(output);
	WriteString(output, "\n");

	WriteBlock(output, ctor->body);
}

static void
func WriteEnum(Output *output, Enum *e)
{
	WriteString(output, "enum ");
	Assert(e->name.id == NameTokenId);
	WriteToken(output, e->namespace_name);
	WriteToken(output, e->name);
	WriteString(output, "\n");

	WriteString(output, "{\n");

	output->tabs++;

	for(int i = 0; i < e->size; i++)
	{
		WriteTabs(output);
		WriteToken(output, e->tokens[i]);
		if(i < e->size - 1)
		{
			WriteString(output, ",");
		}
		WriteString(output, "\n");
	}

	output->tabs--;

	WriteString(output, "};\n");
}

static void 
func WriteStruct(Output *output, Struct *str)
{
	WriteTabs(output);
	WriteString(output, "struct ");
	WriteToken(output, str->namespace_name);
	WriteToken(output, str->name);
	WriteString(output, "\n");

	WriteTabs(output);
	WriteString(output, "{\n");
	output->tabs++;

	StructVar *var = str->first_var;
	while(var)
	{
		WriteTabs(output);
		WriteTypeAndVar(output, var->type, var->name);
		WriteString(output, ";\n");

		var = var->next;
	}

	output->tabs--;
	WriteTabs(output);
	WriteString(output, "};\n");
}

static void decl ReadAndWriteDefinitions(ParseInput *input, Output *output);

static void
func ReadAndWriteNamespace(ParseInput *input, Output *output)
{
	Assert(ReadTokenType(input, NamespaceTokenId));
	Token name = ReadToken(input);
	if(name.id != NameTokenId)
	{
		SetError(input, "Invalid namespace name.");
	}

	if(!ReadTokenType(input, OpenBracesTokenId))
	{
		SetError(input, "Expected '{' after namespace name.");
	}

	input->namespace_name = name;

	ReadAndWriteDefinitions(input, output);

	if(!ReadTokenType(input, CloseBracesTokenId))
	{
		SetError(input, "Expected '}' for namespace.");
	}
}

static void
func ReadAndWriteDefinitions(ParseInput *input, Output *output)
{
	bool first = true;
	while(input->pos->at)
	{
		if(PeekToken(input, CloseBracesTokenId))
		{
			break;
		}

		if(PeekToken(input, EndOfFileTokenId))
		{
			break;
		}

		if(!first)
		{
			WriteString(output, "\n");
		}

		if(PeekToken(input, NamespaceTokenId))
		{
			ReadAndWriteNamespace(input, output);
		}
		else if(PeekToken(input, FuncTokenId))
		{
			Func *f = ReadFunc(input);
			PushFunc(&input->func_stack, f);
			WriteFunc(output, f);
		}
		else if(PeekToken(input, OperatorTokenId))
		{
			Operator *op = ReadOperator(input);
			PushOperator(&input->operator_stack, op);
			WriteOperator(output, op);
		}
		else if(PeekToken(input, ConstructorTokenId))
		{
			Constructor *ctor = ReadConstructor(input);
			PushConstructor(&input->constructor_stack, ctor);
			WriteConstructor(output, ctor);
		}
		else if(PeekToken(input, EnumTokenId))
		{
			Enum *e = ReadEnum(input);
			PushEnum(&input->enum_stack, e);
			WriteEnum(output, e);
		}
		else if(PeekToken(input, StructTokenId))
		{
			Struct *str = ReadStruct(input);
			if(HasStruct(&input->struct_stack, str->name))
			{
				SetErrorToken(input, "Struct already exists.", str->name);
			}
			PushStruct(&input->struct_stack, str);
			WriteStruct(output, str);
		}
		else if(PeekToken(input, DeclTokenId))
		{
			DeclInstruction *decl_instruction = ReadDeclInstruction(input);
			if(!ReadTokenType(input, SemiColonTokenId))
			{
				SetError(input, "Expected ';' after function forward declaration!");
			}

			Func *f = ArenaPushType(input->arena, Func);
			f->header = decl_instruction->func_header;
			f->body = 0;
			PushFunc(&input->func_stack, f);

			WriteInstruction(output, (Instruction *)decl_instruction);
			WriteString(output, ";");
		}
		else
		{
			SetError(input, "Invalid definition (expected 'func' or 'struct').");
			break;
		}

		first = false;
	}
}

static void 
func ReadAndWriteFile(ParseInput *input, Output *output)
{
	ReadAndWriteDefinitions(input, output);
	Token token = ReadToken(input);
	if(token.id != EndOfFileTokenId)
	{
		Assert(token.id == CloseBracesTokenId);
		SetError(input, "Unexpected '}', expected end of file.");
	}
}

CreateStaticArena(global_arena, 64 * 1024 * 1024);

int 
func main()
{
	char *m64_file_name = "Meta.m64";
	char *cpp_file_name = "Meta.hpp";
	printf("Compiling [%s] to c++ [%s]...\n", m64_file_name, cpp_file_name);

	FILE *m64_file = 0;
	fopen_s(&m64_file, m64_file_name, "r");
	Assert(m64_file != 0);

	int code_index = 0;
	while(1)
	{
		Assert(code_index < MaxCodeSize - 1);
		char c;
		int res = fscanf_s(m64_file, "%c", &c, 1);
		if(res != 1)
		{
			break;
		}
		else
		{
			global_code_buffer[code_index] = c;
			code_index++;
		}
	}

	fclose(m64_file);

	Assert(code_index < MaxCodeSize - 1);

	FILE *cpp_file = 0;
	fopen_s(&cpp_file, cpp_file_name, "w");
	Assert(cpp_file != 0);

	CodePosition pos = {};
	pos.at = global_code_buffer;
	pos.row = 1;
	pos.col = 1;
	ParseInput input = {};
	MemArena* arena = &global_arena;
	input.arena = arena;
	input.pos = &pos;

	input.var_stack.vars = ArenaPushArray(arena, VarStackMaxSize, Var);
	input.var_stack.size = 0;

	input.meta_var_stack.vars = ArenaPushArray(arena, MetaVarStackMaxSize, MetaVar);
	input.meta_var_stack.size = 0;

	input.struct_stack.structs = ArenaPushArray(arena, StructStackMaxSize, Struct*);
	input.struct_stack.size = 0;

	input.func_stack.funcs = ArenaPushArray(arena, FuncStackMaxSize, Func*);
	input.func_stack.size = 0;

	input.operator_stack.operators = ArenaPushArray(arena, OperatorStackMaxSize, Operator*);
	input.operator_stack.size = 0;

	input.enum_stack.enums = ArenaPushArray(arena, EnumStackMaxSize, Enum*);
	input.enum_stack.size = 0;

	input.constructor_stack.constructors = ArenaPushArray(arena, ConstructorStackMaxSize, Constructor*);
	input.constructor_stack.size = 0;

	CreateStaticArena(token_name_arena, 64 * 1024);
	input.token_name_arena = &token_name_arena;

	ReadCodeLines(&input);

	Output output = {};
	CreateStaticArena(out_arena, 64 * 1024);
	output.arena = &out_arena;

	ReadAndWriteFile(&input, &output);

	for(int i = 0; i < out_arena.used_size; i++)
	{
		int res = fprintf(cpp_file, "%c", out_arena.memory[i]);
		Assert(res == 1);
	}

	fclose(cpp_file);

	printf("Press Enter...\n");
	char c;
	scanf_s("%c", &c, 1);

	return 0;
}