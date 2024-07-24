#pragma once

#include "stm32f1xx_hal.h"

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

	void retarget_stdout()
	{
		s_stdout = m_huart;
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

typedef bool (*ShellCmdHandler_t)(ShellCmd_t *cmd, const char *s);

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
	bool handle(const char *s);
};

class Shell_t
{
	static const int MAX_STR_SIZE = 128;
	static const int MAX_COMMANDS = 32;
	InputStreamDevice_t *m_input;
	OutputStreamDevice_t *m_output;
	const char *m_str_prompt;
	const char *m_str_initial_prompt;
	char line_buf[MAX_STR_SIZE];
	int line_buf_pos;
	ShellCmd_t commands[MAX_COMMANDS];
	int num_commands;

public:
	Shell_t(InputStreamDevice_t *input, OutputStreamDevice_t *output, const char *str_prompt=NULL, const char *str_initial_prompt=NULL);

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
