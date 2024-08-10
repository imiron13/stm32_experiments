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
#include "ff.h"
#include "utils.h"
#include "tetris.h"
#include "menu.h"

#define EN_GPIO_CONTROL_SHELL_CMD               (0)
#define EN_GPIO_TEST                            (0)
#define EN_SD_CARD_READ_WRITE_SHELL_CMDS        (0)
#define EN_FATFS_TEST                           (0)
#define EN_OPEN_BMP                             (0)
#define EN_MENU                                 (0)
#define EN_TETRIS                               (0)
#define EN_FS_SHELL_COMMANDS                    (1)
#define EN_OPEN_VID                             (1)

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim2;
extern DMA_HandleTypeDef hdma_tim2_up;
extern RTC_HandleTypeDef hrtc;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern DMA_HandleTypeDef hdma_spi2_tx;
extern DMA_HandleTypeDef hdma_spi2_rx;

Shell_t shell;
St7735_Vt100_t st7735_vt100;
FILE *fuart1;
FILE *fuart2;
FILE *fst7735;
FATFS fs;

#if (EN_GPIO_CONTROL_SHELL_CMD == 1)
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
#endif

#if (EN_GPIO_TEST == 1)
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
#endif

#if (EN_TETRIS == 1)
TetrisGame_t<16, 16> tetris;

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
#endif

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

#if (EN_MENU == 1)
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
#endif

static const uint32_t SD_CARD_BLOCK_SIZE_IN_BYTES = 512;

#if (EN_SD_CARD_READ_WRITE_SHELL_CMDS == 1)
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
#endif

bool print_time_date(FILE *f)
{
    RTC_TimeTypeDef time;
    HAL_StatusTypeDef res = HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    if (res != HAL_OK) return false;
    fprintf(f, "Time: %02d:%02d:%02d, ", time.Hours, time.Minutes, time.Seconds);

    RTC_DateTypeDef date;
    res = HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
    if (res != HAL_OK) return false;
    fprintf(f, "date: %02d/%02d/20%02d\n", date.Date, date.Month, date.Year);
    return true;
}

bool shell_cmd_time_get(FILE *f, ShellCmd_t *cmd, const char *s) {
    return print_time_date(f);
}

bool shell_cmd_reset(FILE *f, ShellCmd_t *cmd, const char *s) {
    typedef void (*ResetFunc_t)();
    ResetFunc_t reset_func = *(ResetFunc_t*)0x4;
    reset_func();
    return false;
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
    fprintf(fuart1, BG_BLACK FG_BRIGHT_WHITE VT100_CLEAR_SCREEN VT100_CURSOR_HOME);
    fprintf(fuart1, "UART1 init...done, baudrate=%lu\n", huart1.Init.BaudRate);
    fprintf(fuart2, BG_BLACK FG_BRIGHT_WHITE VT100_CLEAR_SCREEN VT100_CURSOR_HOME);
    fprintf(fuart2, "UART2 init...done, baudrate=%lu\n", huart2.Init.BaudRate);
    printf("SPI1 init...done, %lu KBits/s\n", HAL_RCC_GetPCLK2Freq() / 1000 / (2U << (hspi1.Init.BaudRatePrescaler >> SPI_CR1_BR_Pos)));
    printf("SPI2 init...done, %lu KBits/s\n", HAL_RCC_GetPCLK1Freq() / 1000 / (2U << (hspi2.Init.BaudRatePrescaler >> SPI_CR1_BR_Pos)));
}

bool init_fatfs()
{
    printf("FATFS init...");
    FRESULT res;

    // mount the default drive
    res = f_mount(&fs, "", 0);
    if(res != FR_OK) {
        printf("f_mount() failed, res = %d\r\n", res);
        return false;
    }

    uint32_t freeClust;
    FATFS* fs_ptr = &fs;
    // Warning! This fills fs.n_fatent and fs.csize!
    res = f_getfree("", &freeClust, &fs_ptr);
    if(res != FR_OK) {
        printf("f_getfree() failed, res = %d\r\n", res);
        return false;
    }

    uint32_t totalBlocks = (fs.n_fatent - 2) * fs.csize;
    uint32_t freeBlocks = freeClust * fs.csize;
    printf("done\n");
    printf("Total blocks: %lu (%lu MiB), free blocks: %lu (%lu MiB), cluster=%lu B\r\n",
                totalBlocks, totalBlocks / 2048,
                freeBlocks, freeBlocks / 2048,
                fs.csize * SD_CARD_BLOCK_SIZE_IN_BYTES);
    return true;
}

bool init_storage()
{
    printf("SD card init...");
    int result = SDCARD_Init();
    if (result < 0)
    {
        printf("FAIL\n");
        return false;
    }
    uint32_t sd_num_blocks = 0;
    result = SDCARD_GetBlocksNumber(&sd_num_blocks);
    if (result < 0)
    {
        printf("FAIL\n");
        return false;
    }

    printf("done, size = %d MiB\n", (int)sd_num_blocks / 2 / 1024);

    return init_fatfs();
}

void init_display()
{
    printf("ST7735 LCD init...");
    st7735_vt100.init(Font_7x10);
    fst7735 = st7735_vt100.fopen(uart_read, &huart1);
    fprintf(fst7735, BG_BLACK FG_BRIGHT_WHITE VT100_CLEAR_SCREEN VT100_CURSOR_HOME);
    printf("done\n");
}

#if (EN_FATFS_TEST == 1)
bool shell_cmd_fatfs_test(FILE *f, ShellCmd_t *cmd, const char *s)
{

    DIR dir;
    int res = f_opendir(&dir, "/");
    if(res != FR_OK) {
       fprintf(f, "f_opendir() failed, res = %d\n", res);
        return false;
    }

    FILINFO fileInfo;
    uint32_t totalFiles = 0;
    uint32_t totalDirs = 0;
    fprintf(f, "--------\r\nRoot directory:\n");
    for(;;) {
       res = f_readdir(&dir, &fileInfo);
       if((res != FR_OK) || (fileInfo.fname[0] == '\0')) {
           break;
       }

       if(fileInfo.fattrib & AM_DIR) {
           fprintf(f, "  DIR  %s\n", fileInfo.fname);
           totalDirs++;
       } else {
           fprintf(f, "  FILE %s\n", fileInfo.fname);
           totalFiles++;
       }
    }
    return true;
}
#endif

#if (EN_FS_SHELL_COMMANDS == 1)
char current_folder[FF_MAX_LFN] = "/";

bool shell_cmd_ls(FILE *f, ShellCmd_t *cmd, const char *s)
{
    DIR dir;
    int res = f_opendir(&dir, current_folder);
    if(res != FR_OK) {
       fprintf(f, "f_opendir() failed, res = %d\n", res);
        return false;
    }

    FILINFO fileInfo;
    uint32_t totalFiles = 0;
    uint32_t totalDirs = 0;
    for(;;) {
       res = f_readdir(&dir, &fileInfo);
       if((res != FR_OK) || (fileInfo.fname[0] == '\0')) {
           break;
       }

       if(fileInfo.fattrib & AM_DIR) {
           fprintf(f, "  DIR  %s\n", fileInfo.fname);
           totalDirs++;
       } else {
           fprintf(f, "  FILE %s\n", fileInfo.fname);
           totalFiles++;
       }
    }

    return true;
}

bool shell_cmd_pwd(FILE *f, ShellCmd_t *cmd, const char *s)
{
    fprintf(f, "%s\n", current_folder);
    return true;
}

bool shell_cmd_cd(FILE *f, ShellCmd_t *cmd, const char *s)
{
    bool is_success = true;
    DIR dir;
    const char *folder_arg = cmd->get_str_arg(s, 1);
    char *new_folder = (char*)malloc(FF_MAX_LFN);
    if (strcmp(folder_arg, "..") == 0)
    {
        int i = 0;
        int slash_pos = 0;
        while (current_folder[i] != 0)
        {
            if (current_folder[i] == '/')
            {
                slash_pos = i;
            }
            i++;
        }
        if (slash_pos == 0) slash_pos = 1;
        current_folder[slash_pos] = 0;
    }
    else
    {
        if (folder_arg[0] != '/')
        {
            strcpy(new_folder, current_folder);
            if (current_folder[strlen(current_folder)- 1] != '/')
            {
                strcat(new_folder, "/");
            }
        }

        strcat(new_folder, folder_arg);
        int res = f_opendir(&dir, new_folder);

        if(res != FR_OK) {
            fprintf(f, "Invalid path\n");
            is_success = false;
        }
        else
        {
            strcpy(current_folder, new_folder);

        }
    }
    free(new_folder);
    return is_success;
}
#endif

#if (EN_OPEN_BMP == 1)
int displayImage(const char* fname, FILE *f) {
    fprintf(f, "Opening %s...\r\n", fname);
    FIL file;
    FRESULT res = f_open(&file, fname, FA_READ);
    if(res != FR_OK) {
        fprintf(f, "f_open() failed, res = %d\n", res);
        return -1;
    }

    fprintf(f, "File opened, reading...\n");

    unsigned int bytesRead;
    uint8_t header[34];
    res = f_read(&file, header, sizeof(header), &bytesRead);
    if(res != FR_OK) {
        fprintf(f, "f_read() failed, res = %d\n", res);
        f_close(&file);
        return -2;
    }

    if((header[0] != 0x42) || (header[1] != 0x4D)) {
        fprintf(f, "Wrong BMP signature: 0x%02X 0x%02X\n",
                    header[0], header[1]);
        f_close(&file);
        return -3;
    }

    uint32_t imageOffset = header[10] | (header[11] << 8) |
                           (header[12] << 16) | (header[13] << 24);
    uint32_t imageWidth  = header[18] | (header[19] << 8) |
                           (header[20] << 16) | (header[21] << 24);
    uint32_t imageHeight = header[22] | (header[23] << 8) |
                           (header[24] << 16) | (header[25] << 24);
    uint16_t imagePlanes = header[26] | (header[27] << 8);

    uint16_t imageBitsPerPixel = header[28] | (header[29] << 8);
    uint32_t imageCompression  = header[30] | (header[31] << 8) |
                                 (header[32] << 16) |
                                 (header[33] << 24);
    fprintf(f,
        "--- Image info ---\r\n"
        "Pixels offset: %lu\r\n"
        "WxH: %lux%lu\r\n"
        "Planes: %d\r\n"
        "Bits per pixel: %d\r\n"
        "Compression: %lu\r\n"
        "------------------\r\n",
        imageOffset, imageWidth, imageHeight, imagePlanes,
        imageBitsPerPixel, imageCompression);

    if((imageWidth != ST7735_WIDTH) ||
       (imageHeight != ST7735_HEIGHT))
    {
        fprintf(f, "Wrong BMP size, %dx%d expected\r\n",
                    ST7735_WIDTH, ST7735_HEIGHT);
        f_close(&file);
        return -4;
    }

    if((imagePlanes != 1) || (imageBitsPerPixel != 24) ||
       (imageCompression != 0))
    {
        fprintf(f, "Unsupported image format\r\n");
        f_close(&file);
        return -5;
    }

    res = f_lseek(&file, imageOffset);
    if(res != FR_OK) {
        fprintf(f, "f_lseek() failed, res = %d\r\n", res);
        f_close(&file);
        return -6;
    }

    // row size is aligned to 4 bytes
    uint8_t imageRow[(ST7735_WIDTH * 3 + 3) & ~3];
    for(uint32_t y = 0; y < imageHeight; y++) {
        uint32_t rowIdx = 0;
        res = f_read(&file, imageRow, sizeof(imageRow), &bytesRead);
        if(res != FR_OK) {
            fprintf(f, "f_read() failed, res = %d\r\n", res);
            f_close(&file);
            return -7;
        }

        for(uint32_t x = 0; x < imageWidth; x++) {
            uint8_t b = imageRow[rowIdx++];
            uint8_t g = imageRow[rowIdx++];
            uint8_t r = imageRow[rowIdx++];
            uint16_t color565 = ST7735_COLOR565(r, g, b);
            ST7735_DrawPixel(x, imageHeight - y - 1, color565);
        }
    }

    res = f_close(&file);
    if(res != FR_OK) {
        fprintf(f, "f_close() failed, res = %d\r\n", res);
        return -8;
    }

    return 0;
}

bool shell_cmd_open_bmp(FILE *f, ShellCmd_t *cmd, const char *s)
{
    const char *bmp_file_name = cmd->get_str_arg(s, 1);

    displayImage(bmp_file_name, f);
    return true;
}
#endif

#if (EN_OPEN_VID == 1)
bool shell_cmd_open_vid(FILE *f, ShellCmd_t *cmd, const char *s)
{
    const char *vid_file_name = cmd->get_str_arg(s, 1);
    int frame_delay_ms = cmd->get_int_arg(s, 2);

    fprintf(f, "Opening %s...\r\n", vid_file_name);
    FIL file;
    FRESULT res = f_open(&file, vid_file_name, FA_READ);
    if(res != FR_OK) {
        fprintf(f, "f_open() failed, res = %d\n", res);
        return -1;
    }

    fprintf(f, "File opened, reading...\n");


    const int FRAME_SIZE = 2 * ST7735_WIDTH * ST7735_HEIGHT;
    const int CHUNK_SIZE = 2 * ST7735_WIDTH * 32;
    const int NUM_CHUNKS_PER_FRAME = FRAME_SIZE / CHUNK_SIZE;
    uint8_t *buf = (uint8_t*)malloc(CHUNK_SIZE);
    RTC_TimeTypeDef time0, time1;
    HAL_RTC_GetTime(&hrtc, &time0, RTC_FORMAT_BIN);
    uint32_t num_frames = 0;
    ST7735_Select();
    bool end = false;
    while (!end)
    {
        ST7735_SetAddressWindow(0, 0, ST7735_WIDTH-1, ST7735_HEIGHT-1);
        HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
        for (int chunk = 0; chunk < NUM_CHUNKS_PER_FRAME; chunk++)
        {
            unsigned bytesRead = 0;
            res = f_read(&file, buf, CHUNK_SIZE, &bytesRead);
            if(res != FR_OK) {
                fprintf(f, "Error %d\n", res);
                end = 1;
                break;
            }
            else if (bytesRead == 0)
            {
                fprintf(f, "End video\n");
                end = 1;
                break;
            }

            HAL_StatusTypeDef status = HAL_DMA_Start(&hdma_spi1_tx, (uint32_t)buf, (uint32_t)&ST7735_SPI_PORT.Instance->DR, CHUNK_SIZE);
            ATOMIC_SET_BIT(ST7735_SPI_PORT.Instance->CR2, 0x02);
            status  = HAL_DMA_PollForTransfer(&hdma_spi1_tx, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
            //ST7735_WriteData(buf, CHUNK_SIZE);
        }
        //HAL_Delay(frame_delay_ms);
        int c = fgetc(f);
        if (c =='q')
        {
            end = 1;
        }
        num_frames++;
        HAL_RTC_GetTime(&hrtc, &time1, RTC_FORMAT_BIN);
        uint32_t dif_seconds;
        if (time1.Seconds < time0.Seconds) dif_seconds = 60 + time1.Seconds - time0.Seconds;
        else dif_seconds = time1.Seconds - time0.Seconds;
        if (dif_seconds >= 10)
        {
            time0 = time1;
            fprintf(f, "fps = %lu\n", (num_frames + 5) / 10);
            num_frames = 0;
        }
    }
    ST7735_Unselect();
    free(buf);
    res = f_close(&file);
    if(res != FR_OK) {
        fprintf(f, "f_close() failed, res = %d\n", res);
        return -8;
    }
    return true;
}
#endif

void init_shell()
{
    shell.add_command(ShellCmd_t("time", "Get current date/time", shell_cmd_time_get));
    shell.add_command(ShellCmd_t("reset", "Soft reset", shell_cmd_reset));
    #if (EN_GPIO_CONTROL_SHELL_CMD == 1)
    shell.add_command(ShellCmd_t("gpio", "GPIO control", shell_cmd_gpio));
#endif
#if (EN_GPIO_TEST == 1)
    shell.add_command(ShellCmd_t("gpio_speed_test", "GPIO speed test", shell_cmd_gpio_speed_test));
    shell.add_command(ShellCmd_t("gpio_dma_test", "GPIO DMA test", shell_cmd_gpio_dma_test));
#endif
#if (EN_TETRIS == 1)
    shell.add_command(ShellCmd_t("tetris", "Tetris!", shell_cmd_tetris));
#endif
    shell.add_command(ShellCmd_t("cls", "Clear screen", shell_cmd_clear_screen));
    shell.add_command(ShellCmd_t("heap", "Heap test", shell_cmd_heap_test));
#if (EN_MENU == 1)
    shell.add_command(ShellCmd_t("menu", "Menu test", shell_cmd_menu_test));
#endif
#if (EN_SD_CARD_READ_WRITE_SHELL_CMDS == 1)
    shell.add_command(ShellCmd_t("sdrd", "SD card read", shell_cmd_sd_card_read));
    shell.add_command(ShellCmd_t("sdwr", "SD card write", shell_cmd_sd_card_write));
#endif
#if (EN_FATFS_TEST == 1)
    shell.add_command(ShellCmd_t("fatfs_test", "FATFS test", shell_cmd_fatfs_test));
#endif
#if (EN_FS_SHELL_COMMANDS == 1)
    shell.add_command(ShellCmd_t("ls", "Print conents of the current directory", shell_cmd_ls));
    shell.add_command(ShellCmd_t("cd", "Change directory", shell_cmd_cd));
    shell.add_command(ShellCmd_t("pwd", "Print current directory", shell_cmd_pwd));
#endif
#if (EN_OPEN_BMP == 1)
    shell.add_command(ShellCmd_t("openbmp", "Open BMP file", shell_cmd_open_bmp));
#endif
#if (EN_OPEN_VID == 1)
    shell.add_command(ShellCmd_t("openvid", "Open VID file", shell_cmd_open_vid));
#endif

    shell.set_device(fst7735);
    shell.print_prompt();
    shell.set_device(fuart2);
    shell.print_prompt();
}
int user_main()
{
    deselect_all_devices();

    init_serial();

    init_storage();

    init_display();

#ifdef DEBUG
    printf("DEBUG=1, build time: " __TIME__ "\n");
#else
    printf("DEBUG=0, build time: " __TIME__ "\n");
#endif
    print_time_date(stdout);

    const char *str_welcome = "\nWelcome to the STM32 Experiments Demo FW\n";
    fprintf(fst7735, str_welcome);
    fprintf(fuart1, str_welcome);
    fprintf(fuart2, str_welcome);

    init_shell();

    while (1)
    {
        shell.set_device(fst7735);
        shell.run();
        shell.set_device(fuart2);
        shell.run();
    }
    
    return 0;
}
