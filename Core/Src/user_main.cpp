#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "user_main.h"
#include "main.h"
#include "shell.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

Shell_t shell("> " , "\nWelcome to the STM32 Experiments Demo FW\n");

bool shell_cmd_sum_handler(FILE *f, ShellCmd_t *cmd, const char *s) {
	int arg1 = cmd->get_int_arg(s, 1);
	int arg2 = cmd->get_int_arg(s, 2);
	fprintf(f, "Sum of %d and %d is %d\n", arg1, arg2, arg1 + arg2);
	return true;
}

bool shell_cmd_gpio(FILE *f, ShellCmd_t *cmd, const char *s) {
	const int NUM_GPIO_PORTS_SUPPORTED = 5;
	const int NUM_GPIO_PINS_SUPPORTED = 16;
	enum {
		OP_LOW = 0,
		OP_HIGH = 1
	} op;

	const char* op_str = cmd->get_str_arg(s, 1);
	if (op_str && strcmp(op_str, "hi") == 0)
	{
		op = OP_HIGH;
	}
	else if (op_str && strcmp(op_str, "lo") == 0)
	{
		op = OP_LOW;
	}
	else
	{
		fprintf(f, "gpio <cmd> <pin_id>\n");
		fprintf(f, "<cmd> is 'hi' or 'lo', <pin_id> example: b12\n");
		return false;
	}

	GPIO_TypeDef *const gpio[NUM_GPIO_PORTS_SUPPORTED] = { GPIOA, GPIOB, GPIOC, GPIOD, GPIOE };
	const char* pin_str = cmd->get_str_arg(s, 2);
	int port_id = pin_str[0] - 'a';
	char *end_ptr;
	int pin_id = strtoul(&pin_str[1], &end_ptr, 10);
	if (port_id < 0 || port_id >= NUM_GPIO_PORTS_SUPPORTED ||
		(*end_ptr != 0 && !isspace(*end_ptr)) ||
		pin_id < 0 || pin_id >= NUM_GPIO_PINS_SUPPORTED)
	{
		fprintf(f, "Incorrect pin ID\n");
		return false;
	}

	if (op == OP_LOW)
	{
		HAL_GPIO_WritePin(gpio[port_id], 1U << pin_id, GPIO_PIN_RESET);
	}
	else if (op == OP_HIGH)
	{
		HAL_GPIO_WritePin(gpio[port_id], 1U << pin_id, GPIO_PIN_SET);
	}
	fprintf(f, "Pin %s is set to %c\n", pin_str, op == 0 ? '0' : '1');
	return true;
}

int user_main()
{
	FILE *fuart1 = uart_fopen(&huart1);
	FILE *fuart2 = uart_fopen(&huart2);
	stdout = uart_fopen(&huart2);

	shell.set_device(stdout);
	shell.add_command(ShellCmd_t("sum", "calculates sum of two integers", shell_cmd_sum_handler));
	shell.add_command(ShellCmd_t("gpio", "GPIO control", shell_cmd_gpio));

	fprintf(fuart1, "Hello from UART1\n");
	fprintf(fuart2, "Hello from UART2\n");
	printf("Hello from stdout\n");

	shell.set_device(fuart1);
	shell.print_initial_prompt();
	shell.set_device(fuart2);
	shell.print_initial_prompt();

	while (1)
    {
		shell.set_device(fuart1);
		shell.run();
		shell.set_device(fuart2);
		shell.run();
    }
    
    return 0;
}
