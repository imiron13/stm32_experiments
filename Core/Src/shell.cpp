#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "shell.h"

char ShellCmd_t::buf[ShellCmd_t::MAX_TOKEN_SIZE];

extern "C" ssize_t uart_write(void *huart, const char *ptr, size_t len)
{
    HAL_StatusTypeDef status = HAL_UART_Transmit((UART_HandleTypeDef*)huart, (const uint8_t *)ptr, len, HAL_MAX_DELAY);
    return status == HAL_OK ? len : -1;
}


extern "C" ssize_t uart_read(void *huart, char* buff, size_t len)
{
    size_t bytes_read = 0;
    while (bytes_read < len && __HAL_UART_GET_FLAG((UART_HandleTypeDef*)huart, UART_FLAG_RXNE))
    {
        HAL_UART_Receive((UART_HandleTypeDef*)huart, (uint8_t*)buff, 1, HAL_MAX_DELAY);
        bytes_read++;
    }
    return bytes_read == 0 ? FILE_READ_NO_MORE_DATA : bytes_read;
}

FILE* uart_fopen(UART_HandleTypeDef *huart)
{
    cookie_io_functions_t cookie_funcs = {
        .read = uart_read,
        .write = uart_write,
        .seek = 0,
        .close = 0
    };
    FILE *f = fopencookie(huart, "a+", cookie_funcs);
    setbuf(f, NULL);
    setvbuf(f, NULL, _IONBF, 0);
    return f;
}

ShellCmd_t::ShellCmd_t()
    : m_name(NULL)
    , m_help(NULL)
    , m_handler(NULL)
{

}

ShellCmd_t::ShellCmd_t(const char *name, const char *help, ShellCmdHandler_t handler)
    : m_name(name)
    , m_help(help)
    , m_handler(handler)
{

}

bool ShellCmd_t::check(const char *s)
{
    const char *name = get_str_arg(s, 0);
    return (strcmp(m_name, name) == 0);
}

bool ShellCmd_t::handle(FILE *f, const char *s)
{
    return m_handler(f, this, s);
}

const char *ShellCmd_t::skip_whitespace(const char *s)
{
    while (*s != 0 && (*s == ' ' || *s == '\t'))
    {
        s++;
    }
    return s;
}

const char *ShellCmd_t::skip_non_whitespace(const char *s)
{
    while (*s != 0 && (*s != ' ' && *s != '\t'))
    {
        s++;
    }
    return s;
}

const char *ShellCmd_t::get_str_arg(const char *s, int arg_idx)
{
    for (int i = 0; i < arg_idx; i++)
    {
        s = skip_whitespace(s);
        s = skip_non_whitespace(s);
    }
    s = skip_whitespace(s);
    if (*s == 0)
    {
        return NULL;
    }
    else
    {
        const char *end = skip_non_whitespace(s);
        int i = 0;
        while (s != end && i < MAX_TOKEN_SIZE - 2)
        {
            buf[i++] = *s;
            s++;
        }
        buf[i] = 0;
        return buf;
    }
}

int ShellCmd_t::get_int_arg(const char *s, int arg_idx)
{
    s = get_str_arg(s, arg_idx);
    return atoi(s);
}

Shell_t::Shell_t(const char *str_prompt, const char *str_initial_prompt)
    : line_buf_pos(0)
    , num_commands(0)
{
    m_str_prompt = (str_prompt == NULL) ? "> " : str_prompt;
    m_str_initial_prompt = (str_initial_prompt == NULL) ? "\n" : str_initial_prompt;
}

void Shell_t::set_device(FILE *device)
{
    m_device = device;
}

bool Shell_t::add_command(ShellCmd_t cmd)
{
    if (num_commands < MAX_COMMANDS - 1)
    {
        commands[num_commands] = cmd;
        num_commands++;
        return true;
    }
    else
    {
        return false;
    }
}

ShellCmd_t *Shell_t::find_command()
{
    for (int i = 0; i < num_commands; i++)
    {
        if (commands[i].check(line_buf))
        {
            return &commands[i];
        }
    }
    return NULL;
}

bool Shell_t::handle_command(ShellCmd_t *cmd)
{
    return cmd->handle(m_device, line_buf);
}

void Shell_t::print_initial_prompt()
{
    fputs(m_str_initial_prompt, m_device);
    print_prompt();
}

void Shell_t::print_prompt()
{
    fputs(m_str_prompt, m_device);
}

void Shell_t::print_echo(char c)
{
    fputc(c, m_device);
}

bool Shell_t::handle_char(char c)
{
    if (c == '\r' || c == '\n')
    {
        line_buf[line_buf_pos] = 0;
        print_echo('\n');
        return true;
    }
    else if (c == '\b')
    {
        if (line_buf_pos > 0)
        {
            print_echo('\b');
            print_echo(' ');
            print_echo('\b');
            line_buf_pos--;
        }
        return false;
    }
    else if (line_buf_pos < MAX_STR_SIZE - 2 && ((c >= 32 && c < 127) || (c == 0x1B)))
    {
        line_buf[line_buf_pos] = c;
        line_buf_pos++;
        print_echo(c);
    }
    return false;
}

void Shell_t::handle_line()
{
    ShellCmd_t *cmd = find_command();
    if (cmd == NULL)
    {
        if (line_buf[0] != 0)
        {
            fputs("Unknown command\n", m_device);
        }
    }
    else
    {
        handle_command(cmd);
    }
    print_prompt();
    line_buf_pos = 0;
}

void Shell_t::run()
{
    int c = fgetc(m_device);
    if (c != FILE_READ_NO_MORE_DATA && c != EOF)
    {
        bool is_new_line = handle_char(c);

        if (is_new_line)
        {
            handle_line();
        }
    }
}
