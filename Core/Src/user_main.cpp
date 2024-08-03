#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "user_main.h"
#include "main.h"
#include "shell.h"
#include "st7735.h"
#include "vt100.h"
#include "st7735_vt100.h"
#include "sdcard.h"
#include "utils.h"
#include "tetris.h"
#include "menu.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim2;
extern DMA_HandleTypeDef hdma_tim2_up;
extern RTC_HandleTypeDef hrtc;

Shell_t shell;
St7735_Vt100_t st7735_vt100;
FILE *fuart1;
FILE *fuart2;
FILE *fst7735;
TetrisGame_t<16, 16> tetris;

bool shell_cmd_sum_handler(FILE *f, ShellCmd_t *cmd, const char *s)
{
    int arg1 = cmd->get_int_arg(s, 1);
    int arg2 = cmd->get_int_arg(s, 2);
    fprintf(f, "Sum of %d and %d is %d\n", arg1, arg2, arg1 + arg2);
    return true;
}

bool shell_cmd_gpio(FILE *f, ShellCmd_t *cmd, const char *s)
{
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

bool shell_cmd_gpio_speed_test(FILE *f, ShellCmd_t *cmd, const char *s) __attribute((optimize("unroll-loops")));
bool shell_cmd_gpio_speed_test(FILE *f, ShellCmd_t *cmd, const char *s)
{
#pragma GCC unroll 32
    while (1)
    {

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
    }
    return true;
}

bool shell_cmd_gpio_dma_test(FILE *f, ShellCmd_t *cmd, const char *s)
{
    // taken from https://github.com/mnemocron/STM32_PatternDriver
    static uint32_t pixelclock[16] = {0};
    for (int i = 0; i < 16; i++)
    {
        if (i % 2 == 0)
        {
            pixelclock[i] = 0x00020000;
        }
        else
        {
            pixelclock[i] = 0x00000002;
        }
    }
    // the pixelclock goes straight to the BSRR register
    // [31 ... 16] are reset bits corresponding to Ports [15 ... 0]
    // [15 ...  0] are   set bits corresponding to Ports [15 ... 0]
    // if a reset bit is set, the GPIO port will be set LOW
    // if a   set bit is set, the GPIO port will be set HIGH
    pixelclock[ 5] |= 0x00000001;  // set SI signal on 1st clock edge
    pixelclock[ 8] |= 0x00010000;  // reset SI signal on 3rd clock edge

    // DMA, circular memory-to-peripheral mode, full word (32 bit) transfer
    HAL_DMA_Start(&hdma_tim2_up,  (uint32_t)pixelclock, (uint32_t)&(GPIOB->BSRR), 16);
    HAL_TIM_Base_Start(&htim2);
    HAL_TIM_OC_Start(&htim2, TIM_CHANNEL_1);
    TIM2->DIER |= (1 << 8);   // set UDE bit (update dma request enable)
    return true;
}

bool shell_cmd_tetris(FILE *f, ShellCmd_t *cmd, const char *s)
{
    tetris.set_device(f);
    tetris.start_game();

    bool quit = false;
    while (!quit)
    {
        quit = tetris.run_ui();
        HAL_Delay(tetris.get_ui_update_period_ms());
    }
    fprintf(f, BG_BLACK FG_BRIGHT_WHITE VT100_CLEAR_SCREEN VT100_CURSOR_HOME VT100_SHOW_CURSOR);
    fprintf(f, "Thanks for playing!\n");
    return true;
}

bool shell_cmd_clear_screen(FILE *f, ShellCmd_t *cmd, const char *s)
{
    fprintf(f, BG_BLACK FG_BRIGHT_WHITE VT100_CLEAR_SCREEN VT100_CURSOR_HOME);
    return true;
}

bool shell_cmd_heap_test(FILE *f, ShellCmd_t *cmd, const char *s)
{
    int arg1 = cmd->get_int_arg(s, 1);
    void *p = malloc(arg1);
    if (p)
    {
        fprintf(f, "Success\n");
        free(p);
        return true;
    }
    else
    {
        fprintf(f, "Failed to allocate heap memory\n");
        return false;
    }
}

MenuItem_t menu_items[] = {
        { MENU_ITEM_TYPE_SUB_MENU, "=== Config Menu ===", 0, NULL },
        { MENU_ITEM_TYPE_BOOL, "Enable feature1", 0, &menu_items[0] },
        { MENU_ITEM_TYPE_INT, "Volume", 50, &menu_items[0] },
        { MENU_ITEM_TYPE_SUB_MENU, "Misc", 0, &menu_items[0] },
        { MENU_ITEM_TYPE_SELECT, "ADC mode", 2, &menu_items[3] },
};

Menu_t menu;

bool shell_cmd_menu_test(FILE *f, ShellCmd_t *cmd, const char *s)
{
    menu.set_device(f);
    menu.set_menu_items(menu_items, sizeof(menu_items) / sizeof(menu_items[0]));
    fprintf(f, "Menu test\n");
    menu.reset();

    bool quit = false;
    while (!quit)
    {
        quit = menu.run_ui();
        HAL_Delay(200);
    }
    fprintf(f, BG_BLACK FG_BRIGHT_WHITE VT100_CLEAR_SCREEN VT100_CURSOR_HOME VT100_SHOW_CURSOR);
    return true;
}

static const uint32_t SD_CARD_BLOCK_SIZE_IN_BYTES = 512;
uint8_t sd_card_buf[SD_CARD_BLOCK_SIZE_IN_BYTES];

bool shell_cmd_sd_card_read(FILE *f, ShellCmd_t *cmd, const char *s)
{
    int blk = cmd->get_int_arg(s, 1);
    int num_bytes = cmd->get_int_arg(s, 2);
    int res = SDCARD_ReadSingleBlock(blk, sd_card_buf);
    if (res < 0)
    {
        fprintf(f, "Error reading SD card block %d\n", blk);
        return false;
    }
    else
    {
        for (int i = 0; i < num_bytes; i++)
        {
            fprintf(f, "%02X ", sd_card_buf[i]);
            if ((i % 8) == 7)
            {
                fprintf(f, "\n");
            }
        }
        fprintf(f, "\n");
        return true;
    }
}

bool shell_cmd_sd_card_write(FILE *f, ShellCmd_t *cmd, const char *s)
{
    int blk = cmd->get_int_arg(s, 1);
    int res = SDCARD_WriteSingleBlock(blk, sd_card_buf);
    if (res < 0)
    {
        fprintf(f, "Error writing SD card block %d\n", blk);
        return false;
    }
    else
    {
        fprintf(f, "Successfully written to SD card block %d\n", blk);
        return true;
    }
}

bool shell_cmd_time_get(FILE *f, ShellCmd_t *cmd, const char *s) {
    RTC_TimeTypeDef time;
    HAL_StatusTypeDef res = HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    if (res != HAL_OK) return false;
    fprintf(f, "Time: %02d:%02d:%02d\n", time.Hours, time.Minutes, time.Seconds);

    RTC_DateTypeDef date;
    res = HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
    if (res != HAL_OK) return false;
    fprintf(f, "Date: %02d/%02d/20%02d\n", date.Date, date.Month, date.Year);
    return true;
}

bool shell_cmd_reset(FILE *f, ShellCmd_t *cmd, const char *s) {
    typedef void (*ResetFunc_t)();
    ResetFunc_t reset_func = *(ResetFunc_t*)0x4;
    reset_func();
    return false;
}

void init_shell_commands()
{
    shell.add_command(ShellCmd_t("sum", "calculates sum of two integers", shell_cmd_sum_handler));
    shell.add_command(ShellCmd_t("gpio", "GPIO control", shell_cmd_gpio));
    shell.add_command(ShellCmd_t("gpio_speed_test", "GPIO speed test", shell_cmd_gpio_speed_test));
    shell.add_command(ShellCmd_t("gpio_dma_test", "GPIO DMA test", shell_cmd_gpio_dma_test));
    shell.add_command(ShellCmd_t("tetris", "Tetris!", shell_cmd_tetris));
    shell.add_command(ShellCmd_t("cls", "Clear screen", shell_cmd_clear_screen));
    shell.add_command(ShellCmd_t("heap", "Heap test", shell_cmd_heap_test));
    shell.add_command(ShellCmd_t("menu", "Menu test", shell_cmd_menu_test));
    shell.add_command(ShellCmd_t("sdrd", "SD card read", shell_cmd_sd_card_read));
    shell.add_command(ShellCmd_t("sdwr", "SD card write", shell_cmd_sd_card_write));
    shell.add_command(ShellCmd_t("time", "Get current date/time", shell_cmd_time_get));
    shell.add_command(ShellCmd_t("reset", "Soft reset", shell_cmd_reset));
}

void deselect_all_devices()
{
    SDCARD_Unselect();
    ST7735_Unselect();
}

void init_serial()
{
    fuart1 = uart_fopen(&huart1);
    fuart2 = uart_fopen(&huart2);
    stdout = fuart2;
    fprintf(fuart1, BG_BLACK FG_BRIGHT_WHITE VT100_CLEAR_SCREEN VT100_CURSOR_HOME "UART1 init...done\n");
    fprintf(fuart2, BG_BLACK FG_BRIGHT_WHITE VT100_CLEAR_SCREEN VT100_CURSOR_HOME "UART2 init...done\n");
}


int user_main()
{
    deselect_all_devices();

    init_serial();

    printf("SD card init...");
    SDCARD_Init();
    uint32_t sd_num_blocks = 0;
    SDCARD_GetBlocksNumber(&sd_num_blocks);
    printf("done, size = %d KB\n", (int)sd_num_blocks / 2);

    printf("ST7735 LCD init...");
    st7735_vt100.init(Font_7x10);
    fst7735 = st7735_vt100.fopen(uart_read, &huart1);
    fprintf(fst7735, BG_BLACK FG_BRIGHT_WHITE VT100_CLEAR_SCREEN VT100_CURSOR_HOME);
    printf("done\n");

    const char *str_welcome = "\nWelcome to the STM32 Experiments Demo FW\n";
    fprintf(fst7735, str_welcome);
    fprintf(fuart1, str_welcome);
    fprintf(fuart2, str_welcome);

    init_shell_commands();

    shell.set_device(fst7735);
    shell.print_prompt();
    shell.set_device(fuart2);
    shell.print_prompt();

    while (1)
    {
        shell.set_device(fst7735);
        shell.run();
        shell.set_device(fuart2);
        shell.run();
    }
    
    return 0;
}
