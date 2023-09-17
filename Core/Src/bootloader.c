/*
 * bootloader.c
 *
 *  Created on: 18 Jan. 2022
 *      Author: Jacob Bai
 */

#include "bootloader.h"
#include <stdbool.h>

struct Image_Header {
	uint32_t startSector;
	uint32_t numOfSectors;
	uint32_t startAddr;
};

static struct Image_Header newImg = {
		NEW_IMG_START_SECTOR,
		NEW_IMG_NUM_SECTORS,
		NEW_IMG_ADDR,
};

static struct Image_Header currImg = {
		CURR_IMG_START_SECTOR,
		CURR_IMG_NUM_SECTORS,
		CURR_IMG_ADDR,
};

static struct Image_Header backImg = {
		BACK_IMG_START_SECTOR,
		BACK_IMG_NUM_SECTORS,
		BACK_IMG_ADDR,
};

typedef void (*pFunction)(void);
uint32_t JumpAddress;
pFunction JumpToApplication;

void system_DeInit()
{
	//-- reset peripherals to guarantee flawless start of user application
	HAL_GPIO_DeInit(USER_Btn_GPIO_Port, USER_Btn_Pin);
	HAL_GPIO_DeInit(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin);
	HAL_GPIO_DeInit(USB_OverCurrent_GPIO_Port, USB_OverCurrent_Pin);
	HAL_GPIO_DeInit(GPIOB, LD1_Pin);
	HAL_GPIO_DeInit(GPIOB, LD2_Pin);
	HAL_GPIO_DeInit(GPIOB, LD3_Pin);
	__HAL_RCC_GPIOC_CLK_DISABLE();
	__HAL_RCC_GPIOD_CLK_DISABLE();
	__HAL_RCC_GPIOB_CLK_DISABLE();
	__HAL_RCC_GPIOA_CLK_DISABLE();
	HAL_RCC_DeInit();
	HAL_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;
}

void blinkLed (GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t time) {
	for (; time > 0; time--) {
		HAL_GPIO_TogglePin(GPIOx, GPIO_Pin);
		HAL_Delay(500);
	}
}
bool sectorErase (uint32_t sector, uint32_t num)
{
	FLASH_EraseInitTypeDef flash;
	HAL_StatusTypeDef status = HAL_OK;

	status = HAL_FLASH_Unlock();
	if (status != HAL_OK)
		return false;

	// init flash type
	flash.TypeErase = FLASH_TYPEERASE_SECTORS;
	flash.Sector = sector;
	flash.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	flash.NbSectors = num;

	// erase sectors
	uint32_t error = 0xFFFFFFFFU;
	status = HAL_FLASHEx_Erase(&flash, &error);
	if (status != HAL_OK) {
		HAL_FLASH_Lock();
		return false;
	}

	status = HAL_FLASH_Lock();
	if (status != HAL_OK)
		return false;

	return true;
}

bool imageCopy(struct Image_Header* dstImg, struct Image_Header* srcImg)
{
	HAL_StatusTypeDef status = HAL_OK;
	status = HAL_FLASH_Unlock();
	if (status != HAL_OK)
		return false;
	uint32_t data = 0;
	// start flashing from the end to the start, if lost power when backup, bl will do backup again next time
	//					image start address	+ image size
	uint32_t srcFlashAddr = srcImg->startAddr + IMG_PARTITION_SZ - 4;
	//					dst image start address	+ image size
	uint32_t dstFlashAddr = dstImg->startAddr + IMG_PARTITION_SZ - 4;

	for (; srcFlashAddr >= srcImg->startAddr; srcFlashAddr -= 4) {
		data = *(__IO uint32_t*)srcFlashAddr;
		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dstFlashAddr, (uint64_t)data);
		if (status != HAL_OK || (*(__IO uint32_t*)dstFlashAddr != data)) {
			HAL_FLASH_Lock();
			return false;
		}
		dstFlashAddr -= 4;
	}

	status = HAL_FLASH_Lock();
	if (status != HAL_OK)
		return false;

	return true;
}

bool imageClean (struct Image_Header* thisImg) {
	if (!sectorErase(thisImg->startSector, thisImg->numOfSectors))
		return false;
	return true;
}

bool imageEmpty (struct Image_Header thisImg) {
	uint32_t dstFlashAddr = thisImg.startAddr;
	for (; dstFlashAddr <= thisImg.startAddr + IMG_PARTITION_SZ - 4; dstFlashAddr += 4) {
		if ((*(__IO uint32_t*)dstFlashAddr != 0xFFFFFFFF)) {
			return false;
		}
	}
	return true;
}

bool backup()
{
	if(!imageClean(&backImg))
		return false;

	if (!imageCopy(&backImg, &currImg))
		return false;

	return true;
}

bool update()
{
	if(!imageClean(&currImg))
		return false;

	if (!imageCopy(&currImg, &newImg))
		return false;

	if(!imageClean(&newImg))
		return false;

	return true;
}

bool restore()
{
	if(!imageClean(&currImg))
		return false;

	if (!imageCopy(&currImg, &backImg))
		return false;

	return true;
}

void start(struct Image_Header* thisImg) {
	JumpAddress = *(__IO uint32_t*) (thisImg->startAddr + 4);
	JumpToApplication = (pFunction) JumpAddress;

	__set_MSP(JumpAddress);
	JumpToApplication();
}

void bootloader()
{
	while (1) {

		if (!IMAGE_AVALIABLE(backImg)) {
			if(!backup())
				blinkLed(LD1_GPIO_Port, LD1_Pin, 2); //NVIC_SystemReset();
		}
		if (IMAGE_AVALIABLE(newImg)) {
			if(!update())
				blinkLed(LD2_GPIO_Port, LD2_Pin, 2); //NVIC_SystemReset();
		}
		if (!IMAGE_AVALIABLE(currImg)) {
			if(!restore())
				blinkLed(LD3_GPIO_Port, LD3_Pin, 2); //NVIC_SystemReset();
		}
		if (!imageEmpty(newImg))
			if(!imageClean(&newImg))
				blinkLed(LD3_GPIO_Port, LD3_Pin, 2); //NVIC_SystemReset();

		blinkLed(LD3_GPIO_Port, LD3_Pin, 2);
		system_DeInit();
		start(&currImg);
	}


}

