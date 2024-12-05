#include <stdint.h>
#include "UART.h"
#include "GUARD.h"

void RecursiveFunction(int depth)
{
    volatile int dummy[100];
    dummy[0] = depth;
    RecursiveFunction(depth + 1);
}

int main()
{
	UART2_Init();
	printf("Hello World\n\r");
	StackGuard_Init(128);
	RecursiveFunction(0);

	while(1)
	{

	}
}
