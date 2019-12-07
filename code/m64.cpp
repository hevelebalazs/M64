#include <stdio.h>
#include <stdlib.h>

#define Assert(condition) if(!(condition)) {printf("%s (line %i) failed.\n", #condition, __LINE__); exit(-1);}
#define Break() Assert(false)

#define func

struct MemArena
{
	char *memory;
	int max_size;
	int used_size;
};

MemArena
func CreateArena(char *memory, int size)
{
	Assert(memory != 0);
	Assert(size > 0);
	MemArena arena = {};
	arena.memory = memory;
	arena.max_size = size;
	arena.used_size = 0;
	return arena;
}

#define CreateStaticArena(name, size) char name##_buffer[size]; MemArena name = CreateArena(name##_buffer, size);

char *
func ArenaAlloc(MemArena *arena, int size)
{
	char *memory = arena->memory + arena->used_size;
	arena->used_size += size;
	Assert(arena->used_size <= arena->max_size);
	return memory;
}

#define ArenaAllocType(arena, type) (type *)ArenaAlloc(arena, sizeof(type))
#define ArenaAllocArray(arena, count, type) (type *)ArenaAlloc(arena, (count) * sizeof(type))

enum TokenId
{
	NoTokenId,
	EndOfFileTokenId,
	NameTokenId,
	StructTokenId,
	FuncTokenId,
	OpenParenTokenId,
	CloseParenTokenId,
	OpenCurlyBracketsTokenId,
	CloseCurlyBracketsTokenId,
	ColonTokenId,
	SemicolonTokenId,
	AtTokenId,
	CommaTokenId
};

struct Token
{
	TokenId id;
	char *text;
	int length;
};

struct CodePosition
{
	char *at;
};

bool
func IsNewLine(char c)
{
	return (c == '\n' || c == '\r');
}

bool
func IsWhiteSpace(char c)
{
	return (c == ' ' || c == '\t' || IsNewLine(c));
}

bool
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

bool
func IsDigit(char c)
{
	return (c >= '0' && c <= '9');
}

void
func SkipWhiteSpaceAndComments(CodePosition *pos)
{
	while(pos->at[0])
	{
		if(IsWhiteSpace(pos->at[0]))
		{
			pos->at++;
		}
		else if(pos->at[0] == '/' && pos->at[1] == '/')
		{
			pos->at += 2;
			while(pos->at[0] && !IsNewLine(pos->at[0]))
			{
				pos->at++;
			}
		}
		else if(pos->at[0] == '/' && pos->at[1] == '*')
		{
			int comment_level = 1;
			
			pos->at++;
			while(1)
			{
				if(pos->at[0] == '*' && pos->at[1] == '/')
				{
					pos->at += 2;
					
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
					comment_level++;
				}
				else if(pos->at[0] == 0)
				{
					break;
				}
				else
				{
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

bool
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
		else if(text[i] != token.text[i])
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
	
	return (length_matches && text_matches);
}

bool
func TokensEqual(Token token1, Token token2)
{
	bool length_equals = (token1.length == token2.length);
	bool text_equals = false;
	if(length_equals)
	{
		text_equals = true;
		for(int i = 0; i < token1.length; i++)
		{
			if(token1.text[i] != token2.text[i])
			{
				text_equals = false;
				break;
			}
		}
	}
	
	bool equals = (length_equals && text_equals);
	return equals;
}

enum VarTypeId
{
	NoTypeId,
	BaseTypeId,
	PointerTypeId,
	StructTypeId
};

struct VarType
{
	VarTypeId id;
};

struct StructVar
{
	Token name;
	StructVar *next;
	VarType *type;
};

struct StructDefinition
{
	StructDefinition *prev;
	StructDefinition *next;
	
	Token name;
	StructVar *first_var;
};

struct FuncParam
{	
	FuncParam *next;
	Token name;
	VarType *type;
};

struct FuncDefinition
{
	Token name;
	FuncParam *first_param;
};

struct ParseInput
{
	MemArena *arena;
	CodePosition *pos;
	
	StructDefinition struct_sentinel;
};

enum BaseVarTypeId
{
	NoBaseTypeId,
	Int32BaseTypeId,
	UInt32BaseTypeId
};

struct BaseType
{
	VarType type;
	
	BaseVarTypeId base_id;
};

struct PointerType
{
	VarType type;
	
	VarType *pointed_type;
};

struct StructType
{
	VarType type;
	
	StructDefinition *definition;
};

Token
func ReadNextToken(ParseInput *input)
{
	CodePosition *pos = input->pos;
	SkipWhiteSpaceAndComments(pos);
	
	Token token = {};
	token.id = NoTokenId;
	token.text = pos->at;
	token.length = 0;
	
	if(pos->at[0] == 0)
	{
		token.id = EndOfFileTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '{')
	{
		token.id = OpenCurlyBracketsTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '}')
	{
		token.id = CloseCurlyBracketsTokenId;
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
	else if(pos->at[0] == ':')
	{
		token.id = ColonTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == ';')
	{
		token.id = SemicolonTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == '@')
	{
		token.id = AtTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(pos->at[0] == ',')
	{
		token.id = CommaTokenId;
		token.length = 1;
		pos->at++;
	}
	else if(IsAlpha(pos->at[0]))
	{
		while(IsAlpha(pos->at[0]) || IsDigit(pos->at[0]))
		{
			token.length++;
			pos->at++;
		}
		
		if(TokenEquals(token, "struct"))
		{
			token.id = StructTokenId;
		}
		else if(TokenEquals(token, "func"))
		{
			token.id = FuncTokenId;
		}
		else
		{
			token.id = NameTokenId;
		}
	}
	
	Assert(token.id != NoTokenId);
	
	return token;
}

bool
func PeekToken(ParseInput *input, TokenId id)
{
	bool result = false;
	
	CodePosition start_pos = *input->pos;
	Token token = ReadNextToken(input);
	
	result = (token.id == id);
	*input->pos = start_pos;
	return result;
}


bool
func ReadToken(ParseInput *input, TokenId id)
{
	bool result = false;
	
	CodePosition start_pos = *input->pos;
	Token token = ReadNextToken(input);
	
	if(token.id != id)
	{
		*input->pos = start_pos;
	}
	
	result = (token.id == id);
	return result;
}

#define MaxCodeSize 1024 * 1024
static char global_code_buffer[MaxCodeSize];

BaseType *
func PushBaseType(MemArena *arena, BaseVarTypeId base_id)
{
	BaseType *base_type = ArenaAllocType(arena, BaseType);
	base_type->type.id = BaseTypeId;
	base_type->base_id = base_id;
	return base_type;
}

PointerType *
func PushPointerType(MemArena *arena, VarType *pointed_type)
{
	Assert(pointed_type != 0);
	PointerType *type = ArenaAllocType(arena, PointerType);
	type->type.id = PointerTypeId;
	type->pointed_type = pointed_type;
	return type;
}

StructType *
func PushStructType(MemArena *arena, StructDefinition *definition)
{
	Assert(definition != 0);
	StructType *type = ArenaAllocType(arena, StructType);
	type->type.id = StructTypeId;
	type->definition = definition;
	return type;
}

StructDefinition *
func FindStructDefinitionByName(ParseInput *input, Token name)
{
	Assert(name.id == NameTokenId);
	StructDefinition *result = 0;
	
	StructDefinition *definition = input->struct_sentinel.next;
	while(definition != &input->struct_sentinel)
	{
		if(TokensEqual(definition->name, name))
		{
			result = definition;
			break;
		}
			
		definition = definition->next;
	}
	
	return result;
}

VarType *
func ReadVarType(ParseInput *input)
{
	VarType *type = 0;
	
	if(ReadToken(input, AtTokenId))
	{
		VarType *pointed_type = ReadVarType(input);
		Assert(pointed_type != 0);
		
		type = (VarType *)PushPointerType(input->arena, pointed_type);
	}
	else
	{
		Token name = ReadNextToken(input);
		Assert(name.id == NameTokenId);
		if(TokenEquals(name, "Int32"))
		{
			type = (VarType *)PushBaseType(input->arena, Int32BaseTypeId);
		}
		else if(TokenEquals(name, "UInt32"))
		{
			type = (VarType *)PushBaseType(input->arena, UInt32BaseTypeId);
		}
		else
		{
			StructDefinition *struct_definition = FindStructDefinitionByName(input, name);
			if(struct_definition)
			{
				type = (VarType *)PushStructType(input->arena, struct_definition);
			}
			else
			{
				Break();
			}
		}
	}
	return type;
}

StructDefinition *
func ReadStructDefinition(ParseInput *input)
{
	StructDefinition *definition = ArenaAllocType(input->arena, StructDefinition);
	
	StructDefinition *next = &input->struct_sentinel;
	StructDefinition *prev = next->prev;
	
	definition->prev = prev;
	definition->next = next;
	prev->next = definition;
	next->prev = definition;
	
	Assert(ReadToken(input, StructTokenId));
	
	Token name = ReadNextToken(input);
	Assert(name.id == NameTokenId);
	
	Assert(ReadToken(input, OpenCurlyBracketsTokenId));
	
	StructVar *first_var = 0;
	StructVar *last_var = 0;
	while(1)
	{
		if(ReadToken(input, CloseCurlyBracketsTokenId))
		{
			break;
		}
		
		Token var_name = ReadNextToken(input);
		Assert(var_name.id == NameTokenId);
		
		Assert(ReadToken(input, ColonTokenId));
		
		VarType *var_type = ReadVarType(input);
		Assert(var_type != 0);
		
		Assert(ReadToken(input, SemicolonTokenId));
		
		StructVar *var = ArenaAllocType(input->arena, StructVar);
		var->next = 0;
		var->name = var_name;
		var->type = var_type;
		
		if(!last_var)
		{
			Assert(!first_var);
			first_var = var;
			last_var = var;
		}
		else
		{
			Assert(first_var);
			last_var->next = var;
			last_var = var;
		}
	}
	
	definition->name = name;
	definition->first_var = first_var;
	
	return definition;
}

FuncDefinition
func ReadFuncDefinition(ParseInput *input)
{
	FuncDefinition definition = {};
	Assert(ReadToken(input, FuncTokenId));
	
	Token name = ReadNextToken(input);
	Assert(name.id == NameTokenId);
	
	Assert(ReadToken(input, OpenParenTokenId));
	
	FuncParam *first_param = 0;
	FuncParam *last_param = 0;
	
	bool is_first_param = true;
	
	while(1)
	{
		if(ReadToken(input, CloseParenTokenId))
		{
			break;
		}
		
		if(!is_first_param)
		{
			Assert(ReadToken(input, CommaTokenId));
		}
		is_first_param = false;
		
		Token var_name = ReadNextToken(input);
		Assert(var_name.id == NameTokenId);
		
		Assert(ReadToken(input, ColonTokenId));
		
		VarType *var_type = ReadVarType(input);
		Assert(var_type != 0);
		
		FuncParam *param = ArenaAllocType(input->arena, FuncParam);
		param->next = 0;
		param->name = var_name;
		param->type = var_type;
		
		if(!last_param)
		{
			Assert(!first_param);
			first_param = param;
			last_param = param;
		}
		else
		{
			Assert(first_param);
			last_param->next = param;
			last_param = param;
		}
	}
	
	definition.name = name;
	definition.first_param  = first_param;
	return definition;
}

struct Output
{
	MemArena *arena;
};

void 
func WriteChar(Output *output, char c)
{
	char *mem = ArenaAllocType(output->arena, char);
	*mem = c;
}

void
func WriteString(Output *output, char *string)
{
	for(int i = 0; string[i]; i++)
	{
		WriteChar(output, string[i]);
	}
}

void
func WriteToken(Output *output, Token token)
{
	for(int i = 0; i < token.length; i++)
	{
		WriteChar(output, token.text[i]);
	}
}

void 
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
				case Int32BaseTypeId:
				{
					WriteString(output, "int");
					break;
				}
				case UInt32BaseTypeId:
				{
					WriteString(output, "unsigned int");
					break;
				}
				default:
				{
					Break();
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
			StructDefinition *definition = struct_type->definition;
			Assert(definition != 0);
			WriteToken(output, definition->name);
			break;
		}
		default:
		{
			Break();
		}
	}
}

void 
func WriteTypeAndVar(Output *output, VarType *type, Token var_name)
{	
	WriteType(output, type);
	if(type->id != PointerTypeId)
	{
		WriteString(output, " ");
	}
	WriteToken(output, var_name);
}

void
func WriteStructDefinition(Output *output, StructDefinition *definition)
{
	WriteString(output, "struct ");
	WriteToken(output, definition->name);
	WriteString(output, "\n");
	
	WriteString(output, "{\n");
	
	for(StructVar *var = definition->first_var; var != 0; var = var->next)
	{
		WriteString(output, "\t");
		WriteTypeAndVar(output, var->type, var->name);
		WriteString(output, ";\n");
	}
	
	WriteString(output, "};\n");
}

void
func WriteFuncDefinition(Output *output, FuncDefinition definition)
{
	WriteString(output, "void ");
	WriteToken(output, definition.name);
	WriteString(output, "(");
	
	for(FuncParam *param = definition.first_param; param != 0; param = param->next)
	{
		WriteTypeAndVar(output, param->type, param->name);
		
		if(param->next)
		{
			WriteString(output, ", ");
		}
	}
	
	WriteString(output, ") {}\n");
}

CreateStaticArena(global_arena, 64 * 1024 * 1024);

int
func main(int argument_count, char** arguments)
{
	Assert(argument_count == 3);
	
	FILE *m64_file = 0;
	fopen_s(&m64_file, arguments[1], "r");
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
	
	CodePosition pos = {};
	pos.at = global_code_buffer;
	ParseInput input = {};
	input.arena = &global_arena;
	input.pos = &pos;
	
	input.struct_sentinel.prev = &input.struct_sentinel;
	input.struct_sentinel.next = &input.struct_sentinel;
	
	Output output = {};
	CreateStaticArena(out_arena, 64 * 1024);
	output.arena = &out_arena;
	
	while(1)
	{
		if(PeekToken(&input, EndOfFileTokenId))
		{
			break;
		}
		
		if(PeekToken(&input, StructTokenId))
		{
			StructDefinition *definition = ReadStructDefinition(&input);
			WriteStructDefinition(&output, definition);
		}
		else if(PeekToken(&input, FuncTokenId))
		{
			FuncDefinition definition = ReadFuncDefinition(&input);
			WriteFuncDefinition(&output, definition);
		}
		else
		{
			Break();
		}
	}
	
	FILE *cpp_file = 0;
	fopen_s(&cpp_file, arguments[2], "w");
	Assert(cpp_file != 0);
	
	fprintf(cpp_file, "// Generated code.\n");
	fprintf(cpp_file, "%.*s", out_arena.used_size, out_arena.memory);
	
	fclose(cpp_file);

	return 0;
}