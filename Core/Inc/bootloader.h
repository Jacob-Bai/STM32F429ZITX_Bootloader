/*
 * bootloader.h
 *
 *  Created on: 18 Jan. 2022
 *      Author: Jacob Bai
 */

#ifndef INC_BOOTLOADER_H_
#define INC_BOOTLOADER_H_

#include "main.h"

#define IMG_START				0x20030000
#define IMG_PARTITION_SZ		0x00080000
#define CURR_IMG_ADDR			0x08080000
#define NEW_IMG_ADDR			0x08100000
#define BACK_IMG_ADDR			0x08180000

#define CURR_IMG_START_SECTOR	FLASH_SECTOR_8
#define CURR_IMG_NUM_SECTORS	4

#define NEW_IMG_START_SECTOR	FLASH_SECTOR_12
#define NEW_IMG_NUM_SECTORS		8

#define BACK_IMG_START_SECTOR	FLASH_SECTOR_20
#define BACK_IMG_NUM_SECTORS	4

#define IMAGE_AVALIABLE(IMG)	((*(__IO uint32_t*)(IMG.startAddr)) == IMG_START)

typedef void (application_t)(void);

typedef struct
{
    uint32_t		stack_addr;     // Stack Pointer
    application_t*	func_p;        // Program Counter
} JumpStruct;

void bootloader();



#endif /* INC_BOOTLOADER_H_ */
