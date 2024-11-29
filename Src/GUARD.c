#include "GUARD.h"

#define MPU_REGION_NUMBER        0
#define MPU_REGION_NO_ACCESS     (0x00)
#define MPU_RASR_ENABLE          (1UL << 0)

static void ConfigMPU(uint32_t base_addr, uint32_t size, uint32_t attributes);

void StackGuard_Init(uint32_t guard_size) 
{
    extern uint32_t _estack;
    uint32_t guard_base = (uint32_t)&_estack - guard_size;
    StackGuard_Disable();
    ConfigMPU(guard_base, guard_size, MPU_REGION_NO_ACCESS);
    StackGuard_Enable();
}

void StackGuard_Enable(void) 
{
    MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;
    __DSB();
    __ISB();
}

void StackGuard_Disable(void) 
{
    MPU->CTRL = 0;
}

void StackGuard_SetHandler(void (*handler)(void)) 
{
    if (handler)
    {
        SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
        __set_MSP((uint32_t)handler);
    }
}

static void ConfigMPU(uint32_t base_addr, uint32_t size, uint32_t attributes) 
{
    MPU->RNR = MPU_REGION_NUMBER;
    MPU->RBAR = base_addr & MPU_RBAR_ADDR_Msk;
    MPU->RASR = (attributes << MPU_RASR_AP_Pos) |
                ((size << MPU_RASR_SIZE_Pos) & MPU_RASR_SIZE_Msk) |
                MPU_RASR_ENABLE;
}

void MemManage_Handler(void) __attribute__((weak));
void MemManage_Handler(void) 
{
    uint32_t fault_addr = SCB->MMFAR;
    while(1);
}
