#ifndef __FLASH_H__
#define __FLASH_H__

void FLASH_Lock(void);
void FLASH_Unlock(void);
void FLASH_Write(uint32_t addr, uint8_t *data, uint32_t size);
void FLASH_Read(uint32_t addr, uint8_t *data, uint32_t size);

#endif
