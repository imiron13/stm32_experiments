#include <stdio.h>

#include "user_main.h"
#include "main.h"
#include "shell.h"

extern UART_HandleTypeDef huart1;

UartOutputDevice_t uart_out(&huart1);
UartInputDevice_t uart_in(&huart1);

Shell_t shell(&uart_in, &uart_out, "> " , "\nWelcome to the STM32 Experiments Demo FW\n");

int user_main()
{
	shell.print_initial_prompt();

	while (1)
    {
		shell.run();
    }
    
    return 0;
}
