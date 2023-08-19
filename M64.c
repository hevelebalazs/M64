#include <stdio.h>
#include <stdlib.h>

#define func
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
} CodePosition;

typedef enum tdef TokenId
{
	UnknownTokenId,
	PoundCCodeTokenId,
	OpenBracesTokenId,
	CloseBracesTokenId,
	OpenParenTokenId,
	CloseParenTokenId,
	CommaTokenId,
	ColonTokenId,
	ColonEqualsTokenId,
	SemiColonTokenId,
	AtTokenId,
	EndOfFileTokenId,
	CCodeTokenId,
	FuncTokenId,
	NameTokenId
} TokenId;

typedef struct tdef Token
{
	TokenId id;
	char *text;
	size_t length;
} Token;

typedef enum tdef VarTypeId
{
	NoTypeId,
	BaseTypeId,
	PointerTypeId
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

#define VarStackMaxSize 64
typedef struct tdef VarStack
{
	Var *vars;
	int size;
} VarStack;

typedef struct tdef ParseInput
{
	MemoryArena arena;
	CodePosition *pos;
	VarStack var_stack;
	bool any_error;
	Token last_token;
} ParseInput;

typedef struct tdef Output
{
	MemoryArena arena;
} Output;

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
		if(res != 1) break;

		buffer[size] = c;
		size++;
	}

	buffer = realloc(buffer, size + 1);
	buffer[size] = 0;
	return buffer;
}

static void
func SetError(ParseInput *input, char *description)
{
	printf("Error: %s\n", description);
	input->any_error = true;
}

static void
func SetErrorToken(ParseInput *input, char *description, Token token)
{
	printf("Error: %s '%.*s'\n", description, token.length, token.text);
	input->any_error = true;
}

static bool
func TokensEqual(Token token1, Token token2)
{
	if(token1.id != token2.id) return false;
	if(token1.length != token2.length) return false;
	
	for(size_t i = 0; i < token1.length; i++)
	{
		if(token1.text[i] != token2.text[i]) return false;
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
		{
			length_matches = false;
		}
	}

	return (text_matches && length_matches);
}

static bool
func IsDigit(char c)
{
	return (c >= '0' && c <= '9');
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

static void
func SkipWhiteSpace(CodePosition *pos)
{
	while(pos->at[0])
	{
		if(IsWhiteSpace(pos->at[0]))
		{
			pos->at++;
		}
		else
		{
			break;
		}
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

	if(pos->at[0] == 0)
	{
		token.id = EndOfFileTokenId;
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
	else if(pos->at[0] == '@')
	{
		token.id = AtTokenId;
		token.length = 1;
		pos->at++;
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
		{
			token.id = PoundCCodeTokenId;
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
	
	input->last_token = token;

	return token;
}

static bool
func ReadTokenId(ParseInput *input, TokenId id)
{
	bool result = false;
	SkipWhiteSpace(input->pos);
	
	CodePosition start_pos = *input->pos;
	
	Token token = ReadToken(input);
	if(token.id != id)
	{
		*input->pos = start_pos;
	}
	
	result = (token.id == id);
	return result;
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
		{
			open_braces_count++;
		}
		if(pos->at[0] == '}')
		{
			open_braces_count--;
			if(open_braces_count == 0)
			{
				pos->at--;
				break;
			}
		}
		
		if(!pos->at[0]) break;
		
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

typedef enum tdef ExpressionId
{
	ArrayIndexExpressionId,
	IntegerConstantExpressionId,
	VarExpressionId
} ExpressionId;

typedef struct tdef Expression
{
	ExpressionId id;
	VarType *type;
} Expression;

static Expression *
func ReadExpression(ParseInput *input)
{
	// TODO: finish this
	return 0;
}

typedef enum tdef InstructionId
{
	BlockInstructionId,
	CreateVariableInstructionId
} InstructionId;

typedef struct tdef Instruction
{
	InstructionId id;
	struct Instruction *next;
} Instruction;

typedef struct tdef BlockInstruction
{
	Instruction i;

	Instruction *first;
} BlockInstruction;

typedef struct tdef CreateVariableInstruction
{
	Instruction i;
	
	Token name;
	VarType *type;
	Expression *init;
} CreateVariableInstruction;

typedef enum tdef DefinitionId
{
	FuncDefinitionId,
	CCodeDefinitionId
} DefinitionId;

typedef struct tdef Definition
{
	DefinitionId id;
} Definition;

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
	}
	
	Token c_code_token = ReadTokenUntilClosingBraces(input);
	c_code_token.id = CCodeTokenId;
	def->code = c_code_token;

	if(!ReadTokenId(input, CloseBracesTokenId))
	{
		SetError(input, "No matching '}' after '#c_code'!");
	}
	
	return def;
}

typedef struct tdef PointerType
{
	VarType type;
	
	VarType *pointed_type;
} PointerType;

static PointerType *
func PushPointerType(MemoryArena *arena, VarType *pointed_type)
{
	PointerType *type = ArenaPushType(arena, PointerType);
	type->type.id = PointerTypeId;
	type->pointed_type = pointed_type;
	return type;
}

typedef enum tdef BaseVarTypeId
{
	Int32BaseTypeId
} BaseVarTypeId;

typedef struct tdef BaseType
{
	VarType type;
	
	BaseVarTypeId base_id;
} BaseType;

// TODO: have a static array of base types instead of always pushing it
static BaseType *
func PushBaseType(MemoryArena *arena, BaseVarTypeId base_id)
{
	BaseType *base_type = ArenaPushType(arena, BaseType);
	base_type->type.id = BaseTypeId;
	base_type->base_id = base_id;
	return base_type;
}

static VarType *
func ReadVarType(ParseInput *input)
{
	VarType *type = 0;
	
	if(ReadTokenId(input, AtTokenId))
	{
		VarType *pointed_type = ReadVarType(input);
		if(!pointed_type)
		{
			SetError(input, "Expected variable type after '@'.");
		}
		
		type = (VarType *)PushPointerType(&input->arena, pointed_type);
	}
	else if(ReadTokenId(input, NameTokenId))
	{
		if(TokenEquals(input->last_token, "int"))
		{
			type = (VarType *)PushBaseType(&input->arena, Int32BaseTypeId);
		}
	}
}

static Var *
func GetVar(VarStack *stack, Token name)
{
	for(size_t i = 0; i < stack->size; i++)
	{
		Var *var = &stack->vars[i];
		if(TokensEqual(var->name, name)) return var;
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
		
		if(!ReadTokenId(input, CommaTokenId)) break;
	}
	
	return list;
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

static bool
func NeedsSemicolon(InstructionId id)
{
	switch(id)
	{
		case BlockInstructionId: 
			return true;
	}
	
	return true;
}

static Instruction *
func ReadInstruction(ParseInput *input)
{
	Instruction *instruction = 0;
	if(PeekTwoTokenIds(input, NameTokenId, ColonEqualsTokenId))
	{
		Token var_name = ReadToken(input);
		if(VarExists(&input->var_stack, var_name))
		{
			SetErrorToken(input, "Variable already exists ", var_name);
		}
		ReadTokenId(input, ColonEqualsTokenId);
		
		Expression *init = ReadExpression(input);
		if(!init)
		{
			SetError(input, "Expected expression after ':='");
		}
		
		if(!init->type)
		{
			SetError(input, "Expression for variable initialization doesn't have type.");
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
	return instruction;
}

static BlockInstruction *
func ReadBlock(ParseInput *input)
{
	BlockInstruction *block = 0;
	ReadTokenId(input, OpenBracesTokenId);
	
	StackState stack_state = GetStackState(input);
	
	Instruction *first_instruction = 0;
	Instruction *last_instruction = 0;
	while(1)
	{
		if(ReadTokenId(input, CloseBracesTokenId)) break;
		
		Instruction *instruction = ReadInstruction(input);
		
		if(!instruction) break;
		
		if(NeedsSemicolon(instruction->id))
		{
			if(!ReadTokenId(input, SemiColonTokenId))
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
	
	block = ArenaPushType(&input->arena, BlockInstruction);
	block->i.id = BlockInstructionId;
	block->i.next = 0;
	block->first = first_instruction;
	
	SetStackState(input, stack_state);
	
	return block;
}

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
	
	FuncHeader header;
	BlockInstruction *body;
} FuncDefinition;

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
		if(ReadTokenId(input, CloseParenTokenId)) break;
		
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
		}
		if(!ReadTokenId(input, ColonTokenId))
		{
			SetError(input, "Expected ':' after function parameter name.");
		}
		
		VarType *param_type = ReadVarType(input);
		for(size_t i = 0; i < name_list.size; i++)
		{
			Token param_name = name_list.names[i];
			if(VarExists(&input->var_stack, param_name))
			{
				SetErrorToken(input, "Variable already exists, cannot be used as a function parameter ", param_name);
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
	
	VarType *return_type = 0;
	if(ReadTokenId(input, ColonTokenId)) return_type = ReadVarType(input);
	
	FuncHeader header = {};
	header.name = name;
	header.first_param = first_param;
	header.return_type = return_type;
	return header;
}

static FuncDefinition *
func ReadFuncDefinition(ParseInput *input)
{
	ReadTokenId(input, FuncTokenId);	
	
	FuncDefinition *def = ArenaPushType(&input->arena, FuncDefinition);
	def->def.id = FuncDefinitionId;
	
	FuncHeader header = ReadFuncHeader(input);
	BlockInstruction *body = ReadBlock(input);
	if(!body)
	{
		SetError(input, "Function doesn't have a body!");
	}
	
	def->header = header;
	def->body = body;
	
	return def;
}

static Definition *
func ReadDefinition(ParseInput *input)
{
	Definition *def = 0;
	Token token = PeekToken(input);
	if(token.id == PoundCCodeTokenId)
	{
		def = (Definition *)ReadCCodeDefinition(input);
	}
	else if(token.id == FuncTokenId)
	{
		def = (Definition *)ReadFuncDefinition(input);
	}
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
		{
			break;
		}
		
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
func WriteChar(Output *output, char c)
{
	char *mem = ArenaPushType(&output->arena, char);
	*mem = c;
}

static void 
func WriteString(Output *output, char* string)
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
		}
		
		elem = elem->next;
		first = false;
	}
}

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

	printf("Transpiling <%s> to <%s>...\n", arg_v[1], arg_v[2]);

	char *buffer = ReadFileToMemory(in);
	
	CodePosition pos = {};
	pos.at = buffer;
	ParseInput input = {};
	input.pos = &pos;
	
	input.arena = CreateArena((size_t)64 * 1024 * 1024);
	
	input.var_stack.vars = ArenaPushArray(&input.arena, VarStackMaxSize, Var);
	input.var_stack.size = 0;
	
	DefinitionList *def_list = ReadDefinitionList(&input);
	
	if(input.any_error) return -1;
	
	Output output = {};
	output.arena = CreateArena((size_t)64 * 1024);
	WriteDefinitionList(&output, def_list);
	
	for(size_t i = 0; i < output.arena.used_size; i++)
	{
		fprintf(out, "%c", output.arena.memory[i]);
	}

	printf("Done transpiling!\n");

	
	return 0;
}
