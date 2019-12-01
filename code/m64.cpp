#include <stdio.h>
#include <stdlib.h>

#define Assert(condition) if(!(condition)) {printf("%s (line %i) failed.\n", #condition, __LINE__); exit(-1);}

#define func

struct CodePosition
{
	char *at;
};

struct ParseInput
{
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
func ReadToken(ParseInput *input, TokenId id)
{
	bool result = false;
	SkipWhiteSpaceAndComments(input->pos);
	
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
	input.pos = &pos;
	
	bool has_struct_definition = false;
	Token struct_name = {};
	
	if(ReadToken(&input, StructTokenId))
	{
		has_struct_definition = true;
		struct_name = ReadNextToken(&input);
		Assert(struct_name.id == NameTokenId);	
	}
	
	FILE *cpp_file = 0;
	fopen_s(&cpp_file, arguments[2], "w");
	Assert(cpp_file != 0);
	
	fprintf(cpp_file, "// Generated code.\n");
	
	if(has_struct_definition)
	{
		fprintf(cpp_file, "struct %.*s {};\n", struct_name.length, struct_name.text);
	}
	
	fclose(cpp_file);

	return 0;
}