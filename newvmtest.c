#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"

int data_int;

int main(void)
{
	int stack_int;
	int* heap_int;
	int* ptr;

	heap_int = malloc(sizeof(int));
	printf(1, "main: 0x%p\n", main);
	printf(1, "data_int: 0x%p\n", &data_int);
	printf(1, "stack_int: 0x%p\n", &stack_int);
	printf(1, "heap_int: 0x%p\n", heap_int);

    printf(1, "%d\n", frees());

	// uncomment below to test 16KB stack 
	ptr = (int*)((int)&stack_int - 0x4000);
	*ptr = 0;

	printf(1, "frees before malloc: %d\n", frees());
	ptr = malloc(40960);
	*ptr = 10;
	printf(1, "frees after malloc: %d\n", frees());

	exit();
}




