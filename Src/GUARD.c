#include "GUARD.h"
#include "UART.h"

#include "core_cm4.h"
#include <stdio.h>

#define MPU_REGION_NUMBER       0
#define MPU_REGION_NO_ACCESS    (0x00)
#define MPU_RASR_ENABLE         (1UL << 0)
#define CRASH_LOG_ADDRESS   	((uint32_t*)0x0807F800)
#define CRASH_LOG_MAGIC     	0xCAFEBABE              // Identifies valid logs


typedef struct
{
    uint32_t magic;             // Validity check
    uint32_t faulting_address;  // Address causing the fault
    uint32_t fault_status;      // Status register (CFSR)
    uint32_t stack_pointer;     // Main Stack Pointer at fault
    uint32_t program_counter;   // Program Counter at fault
} CrashLog_t;


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

static void RecoverCrashLog(void)
{
    CrashLog_t *log = (CrashLog_t*)CRASH_LOG_ADDRESS;

    if (log->magic == CRASH_LOG_MAGIC)
    {
        // Print the crash log to serial terminal
        printf("Fault Address: 0x%08X\n", (unsigned int)log->faulting_address);
        printf("Fault Status: 0x%08X\n", (unsigned int)log->fault_status);
        printf("Stack Pointer: 0x%08X\n", (unsigned int)log->stack_pointer);
        printf("Program Counter: 0x%08X\n", (unsigned int)log->program_counter);

        // Unlock Flash for Erasing
        if ((FLASH->CR & FLASH_CR_LOCK) != 0)
        {
            FLASH->KEYR = 0x45670123;
            FLASH->KEYR = 0xCDEF89AB;
        }

        FLASH->CR &= ~FLASH_CR_PSIZE;
        // Activate Sector Erase
        FLASH->CR |= FLASH_CR_SER;
        // Select Sector Number
        FLASH->CR |= (7U << FLASH_CR_SNB_Pos);
        // Start Sector Erase Operation
        FLASH->CR |= FLASH_CR_STRT;
        // Wait for Erase Operation to be completed
        while (FLASH->SR & FLASH_SR_BSY);
        // Deactivate Sector Erase
        FLASH->CR &= ~FLASH_CR_SER;
        // Lock the Flash
        FLASH->CR |= FLASH_CR_LOCK;
    }
}

void StackGuard_Init(uint32_t guard_size) 
{
	// Recover the Previous Crash Log if existing
	RecoverCrashLog();
	// Calculate Guard buffer size
    extern uint32_t _estack;
    uint32_t guard_base = (uint32_t)&_estack - guard_size;
    // Enable MPU for Configuration
    StackGuard_Disable();
    // Configure Guard Buffer with Attributes
    ConfigMPU(guard_base, guard_size, MPU_REGION_NO_ACCESS);
    // Disable MPU after configuration
    StackGuard_Enable();
}

void MemManage_Handler(void) 
{
    uint32_t msp, pc;
    // Capture MSP and PC
    __asm__("MRS %0, MSP" : "=r" (msp));
    __asm__("LDR %0, [SP, #24]" : "=r" (pc));
    // Record the Crash Log parameters
    CrashLog_t crash_log =
    {
        .magic = CRASH_LOG_MAGIC,
        .faulting_address = SCB->MMFAR,
        .fault_status = SCB->CFSR,
        .stack_pointer = msp,
        .program_counter = pc
    };

    // Unlock Flash for programming
    if ((FLASH->CR & FLASH_CR_LOCK) != 0)
    {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
    // Set Flash Operation Page Size
    FLASH->CR &= ~FLASH_CR_PSIZE;
    // Activate Sector Erase
    FLASH->CR |= FLASH_CR_SER;
    // Select Sector Number
    FLASH->CR |= (7U << FLASH_CR_SNB_Pos);
    // Start Sector Erase Operation
    FLASH->CR |= FLASH_CR_STRT;
    // Wait for Erase operation to be completed
    while (FLASH->SR & FLASH_SR_BSY);
    // Deactivate Sector Erase
    FLASH->CR &= ~FLASH_CR_SER;


    uint32_t *data_ptr = (uint32_t*)&crash_log;
    // Writing Crash Analysis to Internal Flash
    for (uint32_t i = 0; i < sizeof(CrashLog_t) / 4; i++)
    {
    	// Disable Flash Programming
        FLASH->CR &= ~FLASH_CR_PG;
        // Enable Flash Programming
        FLASH->CR |= FLASH_CR_PG;
        // Write word of data to Flash
        *((uint32_t*)(CRASH_LOG_ADDRESS + i)) = data_ptr[i];
        // Wait for Write operation to be completed
        while (FLASH->SR & FLASH_SR_BSY);
    }
    // Lock the Flash
    FLASH->CR |= FLASH_CR_LOCK;
    // Reset the system
    NVIC_SystemReset();
}
