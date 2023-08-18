
	#include <stdio.h>
	
	int main()
	{
		printf("Finding minimum in array\n");
		while(1)
		{
			printf("n=");
			int n;
			scanf("%i", &n);
			
			if(n <= 0 || n > 100)
			{
				printf("Between 1 and 100..\n");
				continue;
			}
		
			int A[100];
			for(int i = 0; i < n; i++)
			{
				scanf("%i", A + i);
			}
			
			int min = ArrayMin(n, A);
			printf("min=%i\n", min);
		}
		
		return 0;
	}
