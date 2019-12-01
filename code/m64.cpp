#include <stdio.h>
#include <stdlib.h>

#define Assert(condition) if(!(condition)) {printf("%s (line %i) failed.\n", #condition, __LINE__); exit(-1);}

#define func

int
func main(int argument_count, char** arguments)
{
	Assert(argument_count == 3);
	
	FILE *m64_file = 0;
	fopen_s(&m64_file, arguments[1], "r");
	Assert(m64_file != 0);
	
	
	
	fclose(m64_file);
	
	FILE *cpp_file = 0;
	fopen_s(&cpp_file, arguments[2], "w");
	Assert(cpp_file != 0);
	
	fprintf(cpp_file, "// Generated code.\n");
	
	fclose(cpp_file);

	return 0;
}