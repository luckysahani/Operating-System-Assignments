#include <stdio.h>
#include <stdlib.h>

int main (void)
{
	execv("print",NULL);
	printf("Returned from execv call.\n");
	return 0;
}
