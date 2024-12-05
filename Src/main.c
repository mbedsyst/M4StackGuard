#include <stdint.h>
#include "GUARD.h"

void RecursiveFunction(int depth)
{
    volatile int dummy[100];
    dummy[0] = depth;
    RecursiveFunction(depth + 1);
}

int main()
{
	StackGuard_Init(32);

	RecursiveFunction(0);

	while(1)
	{

	}
}
