#pragma once

#include "stm32f1xx_hal.h"

FILE* uart_fopen(UART_HandleTypeDef *huart);

// returning EOF for file read causes no further reads possible from the file,
// but for UART as file we want to return no data, but be able to continue
// reading later once data is available
#define FILE_READ_NO_MORE_DATA          (-10)

extern "C" ssize_t uart_read(void *huart, char* buff, size_t len);

class ShellCmd_t;

typedef bool (*ShellCmdHandler_t)(FILE *f, ShellCmd_t *cmd, const char *s);

class ShellCmd_t
{
    static const int MAX_TOKEN_SIZE = 32;
    const char *m_name;
    const char *m_help;
    ShellCmdHandler_t m_handler;
    static char buf[MAX_TOKEN_SIZE];

    static const char *skip_whitespace(const char *s);
    static const char *skip_non_whitespace(const char *s);

public:
    ShellCmd_t();
    ShellCmd_t(const char *name, const char *help, ShellCmdHandler_t handler);
    static const char *get_str_arg(const char *s, int arg_idx);
    static int get_int_arg(const char *s, int arg_idx);

    bool check(const char *s);
    bool handle(FILE *f, const char *s);
};

class Shell_t
{
    static const int MAX_STR_SIZE = 128;
    static const int MAX_COMMANDS = 32;
    FILE *m_device;
    const char *m_str_prompt;
    char line_buf[MAX_STR_SIZE];
    int line_buf_pos;
    ShellCmd_t commands[MAX_COMMANDS];
    int num_commands;

    void make_lower();
    void handle_line();

public:
    Shell_t(const char *str_prompt=NULL);

    void set_device(FILE *device);
    FILE *get_device();
    bool add_command(ShellCmd_t cmd);
    ShellCmd_t *find_command();
    bool handle_command(ShellCmd_t *cmd);

    void print_echo(char c);
    void print_prompt();

    bool handle_char(char c);
    void handle_line(const char *s);
    void run();
};
