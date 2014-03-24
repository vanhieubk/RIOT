#include <stdio.h>
#include "stm32f10x.h"

int main (void){
	uint32_t aCount;
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_Init (GPIOC, &GPIO_InitStruct);
	
	while (1){
		GPIO_ResetBits (GPIOC, GPIO_Pin_9);
		for (aCount = 0; aCount < 1000000; aCount++);
		GPIO_SetBits (GPIOC, GPIO_Pin_9);
		for (aCount = 0; aCount < 1000000; aCount++);
	}
	
	
	return 0;
}

