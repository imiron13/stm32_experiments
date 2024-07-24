#include <stdio.h>

#include "user_main.h"
#include "main.h"

extern UART_HandleTypeDef huart1;

extern "C" int _write(int file, char *ptr, int len)
{
  for (int i = 0; i < len ; i++)
  {
	  HAL_UART_Transmit(&huart1, (const uint8_t *)ptr, len, 100);
  }
  return len;
}

int user_main()
{
    while (1)
    {
    	printf("STM32 Experiments FW\n");
    }
    
    return 0;
}
