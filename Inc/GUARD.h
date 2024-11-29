#ifndef __GUARD_H__
#define __GUARD_H__

#include "stm32f4xx.h"

void StackGuard_Init(uint32_t guard_size);
void StackGuard_Enable(void);
void StackGuard_Disable(void);
void StackGuard_SetHandler(void (*handler)(void));

#endif
