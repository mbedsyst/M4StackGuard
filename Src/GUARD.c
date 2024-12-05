#include "stm32f4xx.h"
#include "core_cm4.h"
#include <stdio.h>
#include "GUARD.h"
#include "UART.h"

#define MPU_REGION_NUMBER       0
#define MPU_REGION_NO_ACCESS    (0x00)
#define MPU_RASR_ENABLE         (1UL << 0)

static void ConfigMPU(uint32_t base_addr, uint32_t size, uint32_t attributes)
{
	// Enable the MPU peripheral
    MPU->CTRL = MPU_CTRL_ENABLE_Msk;
    // Use default Memory Map for Privileged Software execution
    MPU->CTRL |= MPU_CTRL_PRIVDEFENA_Msk;
    // Complete all previous memory operations
    __DSB();
    // Flush the Instruction Pipeline
    __ISB();
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

void StackGuard_Init(uint32_t guard_size) 
{
	UART2_Init();
	// Calculate Guard buffer size
    extern uint32_t _estack;
    uint32_t guard_base = (uint32_t)&_estack - guard_size;
    // Configure Guard Buffer with Attributes
    ConfigMPU(guard_base, guard_size, MPU_REGION_NO_ACCESS);
    printf("[info] Configured Memory Region with MPU\n\r");
}

void MemManage_Handler(void) 
{
	printf("[fault] Executing Fault Handler\n\r");
    uint32_t original_msp, msp, pc;
    // Save the current MSP
    __asm__("MRS %0, MSP" : "=r" (original_msp));
    // Switch to the fault stack
    __asm__("LDR R0, =_estack");
    __asm__("MSR MSP, R0");
    // Capture MSP and PC from fault context
    __asm__("MRS %0, MSP" : "=r" (msp));
    __asm__("LDR %0, [SP, #24]" : "=r" (pc));
    // Validate and fetch the faulting address
    uint32_t faulting_address = (SCB->CFSR & SCB_CFSR_MMARVALID_Msk) ? SCB->MMFAR : 0xFFFFFFFF;
    // Print crash details
    printf("========== Crash Report ==========\n");
    printf("Fault Address  : 0x%08X\n", (unsigned int)faulting_address);
    printf("Fault Status   : 0x%08X\n", (unsigned int)SCB->CFSR);
    printf("Stack Pointer  : 0x%08X\n", (unsigned int)msp);
    printf("Program Counter: 0x%08X\n", (unsigned int)pc);
    printf("==================================\n");
    // Restore the original MSP
    __asm__("MSR MSP, %0" :: "r" (original_msp));
    printf("[fault] Executing System Reset\n\r");
    // Perform a system reset
    NVIC_SystemReset();
}
