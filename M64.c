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

typedef struct tdef CodePosition
{
	char *at;
} CodePosition;

typedef struct tdef ParseInput
{
	MemoryArena arena;
	CodePosition *pos;
	bool any_error;
} ParseInput;

typedef struct tdef Output
{
	MemoryArena arena;
} Output;

typedef enum tdef TokenId
{
	UnknownTokenId,
	PoundCCodeTokenId,
	OpenBracesTokenId,
	CloseBracesTokenId,
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
		printf("Expected '{' after '#c_code'!\n");
		input->any_error = true;
	}
	
	Token c_code_token = ReadTokenUntilClosingBraces(input);
	c_code_token.id = CCodeTokenId;
	def->code = c_code_token;

	if(!ReadTokenId(input, CloseBracesTokenId))
	{
		printf("No matching '}' after '#c_code'!\n");
		input->any_error = true;
	}
	
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
	else
	{
		printf("Expected definition instead of '%.*s'!\n", token.length, token.text);
		input->any_error = true;
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
