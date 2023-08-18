#include <stdio.h>
#include <stdlib.h>

#define func
#define tdef

#define bool int
#define true 1
#define false 0

typedef struct tdef CodePosition
{
	char *at;
} CodePosition;

typedef struct tdef ParseInput
{
	CodePosition *pos;
} ParseInput;

typedef enum tdef TokenId
{
	UnknownTokenId,
	PoundCCodeTokenId,
	OpenBracesTokenId,
	CloseBracesTokenId,
	CCodeTokenId,
	EndOfFileTokenId
} TokenId;

typedef struct tdef Token
{
	TokenId id;
	char *text;
	int length;
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
func ReadTokenType(ParseInput *input, TokenId id)
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
	
	bool any_error = false;
	
	while(1)
	{
		Token token = ReadToken(&input);
		if(token.id == EndOfFileTokenId) break;
		
		if(token.id == PoundCCodeTokenId)
		{
			if(!ReadTokenType(&input, OpenBracesTokenId))
			{
				printf("Expected '{' after '#c_code'!\n");
				any_error = true;
			}
			
			Token c_code_token = ReadTokenUntilClosingBraces(&input);

			fprintf(out, "%.*s", c_code_token.length, c_code_token.text);
			
			if(!ReadTokenType(&input, CloseBracesTokenId))
			{
				printf("No matching '}' after '#c_code'!\n");
				any_error = true;
			}
		}
		else if(token.id == UnknownTokenId)
		{
			printf("Unknown token: [%.*s]\n", token.length, token.text);
			any_error = true;
		}
	}
	
	if(any_error) return -1;
		
	printf("Done transpiling!\n");

	
	return 0;
}
