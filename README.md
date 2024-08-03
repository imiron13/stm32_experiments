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
UART shell (terminal) is supported so that a user can register command handlers and trigger them using UART input in interactive mode. 
Arguments parsing is also supported. See shell.h/.cpp:
<pre>
    bool shell_cmd_clear_screen(FILE *f, ShellCmd_t *cmd, const char *s);

    shell.add_command(ShellCmd_t("cls", "Clear screen", shell_cmd_clear_screen));
    shell.set_device(fuart1);
  
    while (1)
    {
        shell.run();
    }
</pre>

## LCD display on ST7735 controller
ST7735 library from the following repository was used: https://github.com/afiskon/stm32-st7735. See st7735.h/.c, fonts.h/.c.
It features text outpus using multiple fonts and basic graphic output.

Added features:
- Basic support of HW vertical scrolling (based on other ST7735 libraries)
- Redirecting of stdio text output to that display

<pre>
    fst7735 = st7735_vt100.fopen(uart_read, &huart1);  // use uart1 for input and st7735 for output
    shell.set_device(fst7735); 
</pre>

## Basic support of VT100 terminal escape sequences
VT100 terminal standard defines escape sequences to control various terminal attributes via serial interface.
Only a very limited subset of features has been supproted: text and background color (16 basic colors only) change, cursor move/hide/show, clear screen and vertical scrolling. See vt100.h/.cpp.
Note that using that interface you can run pseudo-graphic applications on different devices such as LCD screen on UART terminal using exactly the same code. 
<pre>
    fprintf(fst7735, VT100_HIDE_CURSOR BG_BRIGHT_BLUE FG_BRIGHT_WHITE VT100_CLEAR_SCREEN);
</pre>

## Pseudographical menu
This is a simple pseudographical interactive menu implemented on stdio input/output using VT100 sequences. See menu.h/.cpp.
<pre>
    MenuItem_t menu_items[] = {
        { MENU_ITEM_TYPE_SUB_MENU, "=== Config Menu ===", 0, NULL },
        { MENU_ITEM_TYPE_BOOL, "Enable feature1", 0, &menu_items[0] },
        { MENU_ITEM_TYPE_INT, "Volume", 50, &menu_items[0] },
        { MENU_ITEM_TYPE_SUB_MENU, "Misc", 0, &menu_items[0] },
        { MENU_ITEM_TYPE_SELECT, "ADC mode", 2, &menu_items[3] },
    };
    menu.set_device(fst7735);
    menu.set_menu_items(menu_items, sizeof(menu_items) / sizeof(menu_items[0]));
    menu.reset();
    bool quit = false;
    while (!quit)
    {
        quit = menu.run_ui();
        HAL_Delay(200);
    }    
</pre>

## Pseudographical Tetris
A demonstration of the features described above. Implemented on stdio input/output using VT100 sequences. Can use ST7735 display or UART terminal as an output for example. See tetris.h.
<pre>
    tetris.set_device(f);
    tetris.start_game();

    bool quit = false;
    while (!quit)
    {
        quit = tetris.run_ui();
        HAL_Delay(tetris.get_ui_update_period_ms());
    }
</pre>
