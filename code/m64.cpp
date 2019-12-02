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

struct CodePosition
{
	char *at;
};

struct ParseInput
{
	MemArena *arena;
	CodePosition *pos;
};

enum TokenId
{
	NoTokenId,
	EndOfFileTokenId,
	NameTokenId,
	StructTokenId,
	OpenCurlyBracketsTokenId,
	CloseCurlyBracketsTokenId,
	ColonTokenId,
	SemicolonTokenId,
	AtTokenId
};

struct Token
{
	TokenId id;
	char *text;
	int length;
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

enum VarTypeId
{
	NoTypeId,
	BaseTypeId,
	PointerTypeId
};

struct VarType
{
	VarTypeId id;
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
	}
	
	return type;
}

struct StructVar
{
	Token name;
	VarType *type;
	StructVar *next;
};

struct StructDefinition
{
	Token name;
	StructVar *first_var;
};

StructDefinition
func ReadStructDefinition(ParseInput *input)
{
	StructDefinition definition = {};
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
	
	definition.name = name;
	definition.first_var = first_var;
	
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
func WriteStructDefinition(Output *output, StructDefinition struct_definition)
{
	WriteString(output, "struct ");
	WriteToken(output, struct_definition.name);
	WriteString(output, "\n");
	
	WriteString(output, "{\n");
	
	for(StructVar *var = struct_definition.first_var; var != 0; var = var->next)
	{
		WriteString(output, "\t");
		WriteTypeAndVar(output, var->type, var->name);
		WriteString(output, ";\n");
	}
	
	WriteString(output, "};\n");
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
	
	Assert(PeekToken(&input, StructTokenId));
	StructDefinition struct_definition = ReadStructDefinition(&input);
	
	FILE *cpp_file = 0;
	fopen_s(&cpp_file, arguments[2], "w");
	Assert(cpp_file != 0);
	
	Output output = {};
	CreateStaticArena(out_arena, 64 * 1024);
	output.arena = &out_arena;
	
	WriteStructDefinition(&output, struct_definition);
	
	fprintf(cpp_file, "// Generated code.\n");
	fprintf(cpp_file, "%.*s", out_arena.used_size, out_arena.memory);
	
	fclose(cpp_file);

	return 0;
}