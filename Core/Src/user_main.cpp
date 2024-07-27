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
#include "utils.h"
#include "tetris.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim2;
extern DMA_HandleTypeDef hdma_tim2_up;

Shell_t shell("> " , "\nWelcome to the STM32 Experiments Demo FW\n");

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

class St7735Vt100: public Vt100TerminalServer_t
{
    FontDef m_font;
    int m_cur_scroll_pos;
public:
    void init(FontDef font);

    virtual void print_char(char c);
    virtual RawColor_t rgb_to_raw_color(RgbColor_t rgb);
    virtual void scroll(int num_lines);
};

void St7735Vt100::init(FontDef font)
{
    ST7735_Init();
    m_font = font;
    m_cur_scroll_pos = 0;
    Vt100TerminalServer_t::init(Utils_t::div_ceil_uint(ST7735_WIDTH, m_font.width), Utils_t::div_ceil_uint(ST7735_HEIGHT, m_font.height));
}

void St7735Vt100::print_char(char c)
{
    if (m_x <= m_width && m_y <= m_height)
    {
        int ypos = m_y - 1 - m_cur_scroll_pos;
        if (ypos < 0)
        {
            ypos += m_height;
        }
        ST7735_WriteChar((m_x - 1) * m_font.width, ypos * m_font.height, c, m_font, m_raw_text_color, m_raw_bg_color);
    }
}

St7735Vt100::RawColor_t St7735Vt100::rgb_to_raw_color(RgbColor_t rgb)
{
    // 16 bits color mode: BIT [15:11] - B, BITS[10:5] - G, BITS[4:0] - R
    return (((uint32_t)rgb.r >> 3) << 11) | (((uint32_t)rgb.g >> 2) << 5) | (rgb.b >> 3);
}

void St7735Vt100::scroll(int num_lines)
{
    if (num_lines >= 0)
    {
        for (int i = 0; i < num_lines; i++)
        {
            if (m_cur_scroll_pos == 0)
            {
                m_cur_scroll_pos = m_height - 1;
            }
            else
            {
                m_cur_scroll_pos--;
            }
            m_y--;
        }
    }
    else
    {
        for (int i = 0; i < -num_lines; i++)
        {
            if (m_cur_scroll_pos == m_height - 1)
            {
                m_cur_scroll_pos = 0;
            }
            else
            {
                m_cur_scroll_pos++;
            }
            m_y++;
        }
    }
    ST7735_SetScrollPos(m_font.height * m_cur_scroll_pos);
    if (num_lines > 0)
    {
        clear_line(m_height);
    }
    else if (num_lines < 0)
    {
        clear_line(1);
    }
}

St7735Vt100 st7735_vt100;
TetrisGame_t<16, 16> tetris;

int user_main()
{
    st7735_vt100.init(Font_7x10);

    FILE *fuart1 = uart_fopen(&huart1);
    FILE *fuart2 = uart_fopen(&huart2);
    stdout = uart_fopen(&huart2);
    FILE *fst7735 = st7735_vt100.fopen(uart_read, &huart1);

    shell.set_device(stdout);
    shell.add_command(ShellCmd_t("sum", "calculates sum of two integers", shell_cmd_sum_handler));
    shell.add_command(ShellCmd_t("gpio", "GPIO control", shell_cmd_gpio));
    shell.add_command(ShellCmd_t("gpio_speed_test", "GPIO speed test", shell_cmd_gpio_speed_test));
    shell.add_command(ShellCmd_t("gpio_dma_test", "GPIO DMA test", shell_cmd_gpio_dma_test));


    fprintf(fst7735, "Hello from UART1\n");
    fprintf(fuart2, "Hello from UART2\n");
    printf("Hello from stdout\n");
    //fprintf(fst7735, FG_BRIGHT_WHITE BG_BRIGHT_BLUE VT100_CLEAR_SCREEN "Hello from ST7735\n");
    //fprintf(fst7735, FG_BRIGHT_WHITE BG_BRIGHT_MAGENTA "Hello from ST7735\n");
    //fprintf(fst7735, FG_BLACK BG_BRIGHT_YELLOW "Hello from ST7735\n");
    fprintf(fst7735, BG_BRIGHT_BLUE FG_BRIGHT_WHITE VT100_CLEAR_SCREEN);
    //shell.set_device(fst7735);
    //shell.print_initial_prompt();
    shell.set_device(fuart2);
    shell.print_initial_prompt();

    tetris.start_game();
    tetris.render_background(fst7735);

    int i = 0;
    while (1)
    {
        //shell.set_device(fst7735);
        //shell.run();
        //shell.set_device(fuart2);
        //shell.run();
        int c = fgetc(fst7735);
        if (c != FILE_READ_NO_MORE_DATA && c != EOF)
        {
            if (c == 'a')
            {
                tetris.handle_user_input(tetris.SHIFT_LEFT);
            }
            else if (c == 'd')
            {
                tetris.handle_user_input(tetris.SHIFT_RIGHT);
            }
            else if (c == 'w')
            {
                tetris.handle_user_input(tetris.ROTATE);
            }
            else if (c == 's')
            {
                tetris.handle_user_input(tetris.SPEED_UP);
            }
        }

        tetris.render(fst7735);
        HAL_Delay(tetris.get_cur_fall_delay_ms() / 4);
        i = (i + 1) % 4;
        if (i == 0) tetris.fall_one_step_down();
    }
    
    return 0;
}
