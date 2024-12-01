#include "GUARD.h"
#include "UART.h"

#define MPU_REGION_NUMBER        0
#define MPU_REGION_NO_ACCESS     (0x00)
#define MPU_RASR_ENABLE          (1UL << 0)

static void ConfigMPU(uint32_t base_addr, uint32_t size, uint32_t attributes);

static void StackGuard_Enable(void) 
{
	// Enable the MPU peripheral
    MPU->CTRL = MPU_CTRL_ENABLE_Msk;
    // Use default Memory Map for Privileged Software execution
    MPU->CTRL |= MPU_CTRL_PRIVDEFENA_Msk;
    // Complete all previous memory operations
    __DSB();
    // Flush the Instruction Pipeline
    __ISB();
}

static void StackGuard_Disable(void) 
{
	// Disable the MPU peripheral
    MPU->CTRL = 0;
}

void StackGuard_Init(uint32_t guard_size) 
{
    extern uint32_t _estack;
    // Calculate Guard buffer size
    uint32_t guard_base = (uint32_t)&_estack - guard_size;
    // Configure MPU
    StackGuard_Disable();
    ConfigMPU(guard_base, guard_size, MPU_REGION_NO_ACCESS);
    StackGuard_Enable();
}

static void ConfigMPU(uint32_t base_addr, uint32_t size, uint32_t attributes)
{
	// Set Memory Region Number
    MPU->RNR = MPU_REGION_NUMBER;
    // Set Memory Region Base Address
    MPU->RBAR = base_addr & MPU_RBAR_ADDR_Msk;
    // Set Memory Access permissions
    MPU->RASR |= (attributes << MPU_RASR_AP_Pos);
    // Set Memory Region Size
    MPU->RASR |= ((size << MPU_RASR_SIZE_Pos) & MPU_RASR_SIZE_Msk);
    // Enable Memory Region
    MPU->RASR |= MPU_RASR_ENABLE;
}

void StackGuard_Inspect(void)
{

}

void MemManage_Handler(void) 
{
    // Retrieve Fault Location and Type
    uint32_t fault_addr = SCB->MMFAR;
    printf("[fault] Fault Address : 0x%08X", (unsigned int)fault_addr);
    // Write Fault data to Internal Flash for debugging
    // Reset the System 
    NVIC_SystemReset();
}
