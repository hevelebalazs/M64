static void
func WriteFormattedString(MemoryArena *arena, char *string)
{
	for(size_t i = 0; string[i]; i++)
	{
		char *mem = ArenaPushType(arena, char);
		*mem = string[i];
	}
}

static void
func WriteFormattedToken(MemoryArena *arena, Token token)
{
	for(size_t i = 0; i < token.length; i++)
	{
		char *mem = ArenaPushType(arena, char);
		*mem = token.text[i];
	}
}

static void
func WriteFormattedExpression(MemoryArena *arena, Expression *expression)
{
	switch(expression->id)
	{
		case IntegerConstantExpressionId:
		{
			IntegerConstantExpression *e = (IntegerConstantExpression *)expression;
			WriteFormattedToken(arena, e->token);
			break;
		}
		default:
		{
			// todo: write expression name as string instead!
			printf("Formatted output: expression type %i not yet supported\n", expression->id);
			break;
		}
	}
}

static void
func WriteFormattedReturnInstruction(MemoryArena *arena, ReturnInstruction *i)
{
	WriteFormattedString(arena, "return ");
	WriteFormattedExpression(arena, i->value);
	WriteFormattedString(arena, ";");
}