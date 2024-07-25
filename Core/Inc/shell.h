#pragma once

#include "stm32f1xx_hal.h"

FILE* uart_fopen(UART_HandleTypeDef *huart);

// returning EOF for file read causes no further reads possible from the file,
// but for UART as file we want to return no data, but be able to continue
// reading later once data is available
#define FILE_READ_NO_MORE_DATA			(-10)

class OutputStreamDevice_t
{
public:
	virtual void put(char c) = 0;
	void put_string(const char *s)
	{
		while (*s != 0)
		{
			put(*s);
			s++;
		}
	}
};

class InputStreamDevice_t
{
public:
	virtual bool is_available() = 0;
	virtual char get() = 0;
};

class UartOutputDevice_t : public OutputStreamDevice_t
{
	static const int TIMEOUT = 100;
	UART_HandleTypeDef *m_huart;
public:
	static UART_HandleTypeDef *s_stdout;

	UartOutputDevice_t(UART_HandleTypeDef *huart)
		: m_huart(huart)
	{

	}

	virtual void put(char c)
	{
		HAL_UART_Transmit(m_huart, (uint8_t*)&c, 1, TIMEOUT);
	}
};

class UartInputDevice_t : public InputStreamDevice_t
{
	static const int TIMEOUT = 100;
	UART_HandleTypeDef *m_huart;
public:
	UartInputDevice_t(UART_HandleTypeDef *huart)
		: m_huart(huart)
	{

	}

	virtual bool is_available()
	{
		return __HAL_UART_GET_FLAG(m_huart, UART_FLAG_RXNE);
	}

	virtual char get()
	{
		unsigned char c;
		HAL_UART_Receive(m_huart, &c, 1, TIMEOUT);
		return c;
	}
};

class ShellCmd_t;

typedef bool (*ShellCmdHandler_t)(FILE *f, ShellCmd_t *cmd, const char *s);

class ShellCmd_t
{
	static const int MAX_TOKEN_SIZE = 32;
	const char *m_name;
	const char *m_help;
	ShellCmdHandler_t m_handler;
	static char buf[MAX_TOKEN_SIZE];
public:

	ShellCmd_t();
	ShellCmd_t(const char *name, const char *help, ShellCmdHandler_t handler);
	static const char *skip_whitespace(const char *s);
	static const char *skip_non_whitespace(const char *s);
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
	const char *m_str_initial_prompt;
	char line_buf[MAX_STR_SIZE];
	int line_buf_pos;
	ShellCmd_t commands[MAX_COMMANDS];
	int num_commands;

public:
	Shell_t(const char *str_prompt=NULL, const char *str_initial_prompt=NULL);

	void set_device(FILE *device);
	bool add_command(ShellCmd_t cmd);
	ShellCmd_t *find_command();
	bool handle_command(ShellCmd_t *cmd);

	void print_echo(char c);
	void print_initial_prompt();
	void print_prompt();

	bool handle_char(char c);
	void handle_line();
	void run();
};
