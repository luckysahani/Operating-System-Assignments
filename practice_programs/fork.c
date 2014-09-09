#include <stdio.h>
#include <stdlib.h>
typedef struct {
int x;
} global_t;
global_t*g;
int main(void)
{
	g = (global_t*)malloc(sizeof(global_t));
	g->x = 5;
	if (fork() == 0) {
		if (fork() == 0) {
			g->x--;
			printf("1 %d\n", g->x);
		}
		else {
			g->x++;
		}
		wait(NULL);
		g->x++;
		printf("2 %d\n", g->x);
	}
	else {
		g->x = 2*g->x;
		wait(NULL);
		g->x++;
		printf("3 %d\n", g->x);
	}
	printf("4 %d\n", g->x);
	return 0;
}