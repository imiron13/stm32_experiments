#include <stdio.h>

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

int user_main()
{
	FILE *fuart1 = uart_fopen(&huart1);
	FILE *fuart2 = uart_fopen(&huart2);
	stdout = uart_fopen(&huart2);

	shell.set_device(stdout);
	shell.add_command(ShellCmd_t("sum", "calculates sum of two integers", shell_cmd_sum_handler));

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
