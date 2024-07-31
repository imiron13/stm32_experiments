# stm32_experiments

This repository has educational purposes. Here you can find experiments with STM32 microcontrollers and external peripheral modules using libraries created by other people as well as attempts to implement libraries that may be helpful in other projects.

## UART redirection to stdio files
Mutliple UARTs may be redirected to multiple files:
<pre>
    fuart1 = uart_fopen(&huart1);
    fuart2 = uart_fopen(&huart2);
    stdio = fuart1;
   
    fprintf(fuart1, "Hello from UART1\n");
    fprintf(fuart2, "Hello from UART2\n");
    printf("Hello from stdout\n");
</pre>

UART input is also supported. It works in non-blocking (polling) mode and returns FILE_READ_NO_MORE_DATA if no data available:
<pre>
    int c = fgetc(m_device);
    if (c != FILE_READ_NO_MORE_DATA && c != EOF)
    { ... }
</pre>

## UART shell
UART shell (terminal) is supported so that a user can register command handlers and trigger them using UART input in interactive mode:
<pre>
    bool shell_cmd_clear_screen(FILE *f, ShellCmd_t *cmd, const char *s);

    shell.add_command(ShellCmd_t("cls", "Clear screen", shell_cmd_clear_screen));
    shell.set_device(fuart1);
  
    while (1)
    {
        shell.run();
    }
</pre>
