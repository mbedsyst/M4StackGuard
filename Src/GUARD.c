#include "GUARD.h"
#include "UART.h"

#include "core_cm4.h"
#include <stdio.h>

#define MPU_REGION_NUMBER       0
#define MPU_REGION_NO_ACCESS    (0x00)
#define MPU_RASR_ENABLE         (1UL << 0)
#define CRASH_LOG_ADDRESS   	((uint32_t*)0x0807F800)
#define CRASH_LOG_MAGIC     	0xCAFEBABE


		typedef struct
		{
			uint32_t magic;
			uint32_t faulting_address;
			uint32_t fault_status;
			uint32_t stack_pointer;
			uint32_t program_counter;
		} CrashLog_t;

static void FLASH_Unlock(void)
{
    if ((FLASH->CR & FLASH_CR_LOCK) != 0)
    {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
}

static void FLASH_Lock(void)
{
	FLASH->CR |= FLASH_CR_LOCK;
}

static void FLASH_SectorErase(uint8_t sectorNumber)
{
    FLASH->CR &= ~FLASH_CR_PSIZE;
    // Activate Sector Erase
    FLASH->CR |= FLASH_CR_SER;
    // Select Sector Number
    FLASH->CR |= (sectorNumber << FLASH_CR_SNB_Pos);
    // Start Sector Erase Operation
    FLASH->CR |= FLASH_CR_STRT;
    // Wait for Erase Operation to be completed
    while (FLASH->SR & FLASH_SR_BSY);
    // Deactivate Sector Erase
    FLASH->CR &= ~FLASH_CR_SER;
}

static void FLASH_Write(uint32_t *data, uint32_t addr, uint32_t size)
{
    // Writing data to flash
    uint32_t *data_ptr = data;
    uint32_t flash_address = addr;
    // Loop to write each 32-bit word to flash
    for (uint32_t i = 0; i < size / 4; i++)
    {
        // Wait until the flash is not BUSY
    	while (FLASH->SR & FLASH_SR_BSY);
        // Disable Flash Programming
        FLASH->CR &= ~FLASH_CR_PG;
        // Enable Flash Programming
        FLASH->CR |= FLASH_CR_PG;
        // Write the current word to the flash memory
        *((uint32_t*)flash_address) = data_ptr[i];
        // Wait for the write operation to complete
        while (FLASH->SR & FLASH_SR_BSY);
        // Move to the next address for the next word
        flash_address += 4;
    }
}

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

        FLASH_Unlock();
        FLASH_SectorErase(7);
        FLASH_Lock();
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
    FLASH_Unlock();
    FLASH_SectorErase(7);
    FLASH_Write((uint32_t*)&crash_log, CRASH_LOG_ADDRESS, sizeof(CrashLog_t));
    FLASH_Lock();
    NVIC_SystemReset();
}
