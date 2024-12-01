#include <stdint.h>
#include "LED.h"
#include "UART.h"
#include "W25Qxx.h"

int main()
{
	uint8_t data[4096];
	W25Q_Init();
	LED_Init();
	UART2_Init();

	LED_Toggle();
	W25Q_ReadData(0, 0, data, 4096);
	LED_Toggle();

	while(1)
	{

	}
}
