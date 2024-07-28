#pragma once

#include <stdint.h>
#include <stdio.h>
#include <limits.h>

#define FG_BLACK            "\e[30m"
#define FG_RED              "\e[31m"
#define FG_GREEN            "\e[32m"
#define FG_YELLOW           "\e[33m"
#define FG_BLUE             "\e[34m"
#define FG_MAGENTA          "\e[35m"
#define FG_CYAN             "\e[36m"
#define FG_WHITE            "\e[37m"
#define BG_BLACK            "\e[40m"
#define BG_RED              "\e[41m"
#define BG_GREEN            "\e[42m"
#define BG_YELLOW           "\e[43m"
#define BG_BLUE             "\e[44m"
#define BG_MAGENTA          "\e[45m"
#define BG_CYAN             "\e[46m"
#define BG_WHITE            "\e[47m"
#define FG_GRAY             "\e[90m"
#define FG_BRIGHT_RED       "\e[91m"
#define FG_BRIGHT_GREEN     "\e[92m"
#define FG_BRIGHT_YELLOW    "\e[93m"
#define FG_BRIGHT_BLUE      "\e[94m"
#define FG_BRIGHT_MAGENTA   "\e[95m"
#define FG_BRIGHT_CYAN      "\e[96m"
#define FG_BRIGHT_WHITE     "\e[97m"
#define BG_GRAY             "\e[100m"
#define BG_BRIGHT_RED       "\e[101m"
#define BG_BRIGHT_GREEN     "\e[102m"
#define BG_BRIGHT_YELLOW    "\e[103m"
#define BG_BRIGHT_BLUE      "\e[104m"
#define BG_BRIGHT_MAGENTA   "\e[105m"
#define BG_BRIGHT_CYAN      "\e[106m"
#define BG_BRIGHT_WHITE     "\e[107m"

#define VT100_CLEAR_SCREEN  "\e[2J"
#define VT100_CURSOR_HOME   "\e[H"
#define VT100_HIDE_CURSOR   "\e[?25l"
#define VT100_SHOW_CURSOR   "\e[?25h"
#define VT100_SET_YX        "\e[%d;%dH"

union RgbColor_t
{
    struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t reserved;
    };
    uint32_t u32;
};

class Vt100TerminalServer_t
{
public:
    static const int MAX_ARGS = 4;
    static const char CHAR_ESC = '\e';
    static const char CHAR_CSI = '[';
    static const char CHAR_CSI_PRIVATE = '?';
    static const int START_X_POS = 1;
    static const int START_Y_POS = 1;
    static const int ARG_NOT_SPECIFIED = INT_MIN;

    typedef uint8_t ColorCode_t;

    enum ColorIndex_t
    {
        VT100_COLOR_INDEX_BLACK = 0,
        VT100_COLOR_INDEX_RED,
        VT100_COLOR_INDEX_GREEN,
        VT100_COLOR_INDEX_YELLOW,
        VT100_COLOR_INDEX_BLUE,
        VT100_COLOR_INDEX_MAGENTA,
        VT100_COLOR_INDEX_CYAN,
        VT100_COLOR_INDEX_WHITE,
        VT100_COLOR_INDEX_GRAY,
        VT100_COLOR_INDEX_BRIGHT_RED,
        VT100_COLOR_INDEX_BRIGHT_GREEN,
        VT100_COLOR_INDEX_BRIGHT_YELLOW,
        VT100_COLOR_INDEX_BRIGHT_BLUE,
        VT100_COLOR_INDEX_BRIGHT_MAGENTA,
        VT100_COLOR_INDEX_BRIGHT_CYAN,
        VT100_COLOR_INDEX_BRIGHT_WHITE,
        VT100_COLOR_INDEX_BRIGHT = 8
    };

    static const ColorIndex_t DEFAULT_TEXT_COLOR_INDEX = VT100_COLOR_INDEX_BRIGHT_WHITE;
    static const ColorIndex_t DEFAULT_BACKGROUND_COLOR_INDEX = VT100_COLOR_INDEX_BLACK;

    enum ParserState_t
    {
        VT100_PARSER_STATE_DEFAULT,
        VT100_PARSER_STATE_ESCAPE_RECEIVED,
        VT100_PARSER_STATE_CSI_RECEIVED,
        VT100_PARSER_STATE_CSI_PRIVATE_RECEIVED,
    };

    enum ArgsParserState_t
    {
        ARGS_PARSER_STATE_WAIT_DIGIT1,
        ARGS_PARSER_STATE_WAIT_DIGIT2,
        ARGS_PARSER_STATE_WAIT_DIGIT3,
        ARGS_PARSER_STATE_WAIT_DIGIT4,
        ARGS_PARSER_STATE_WAIT_DIGIT5,
        ARGS_PARSER_STATE_WAIT_LAST_DIGIT = ARGS_PARSER_STATE_WAIT_DIGIT5
    };

protected:
    typedef uint32_t RawColor_t;

    static const RgbColor_t colors_table_rgb[16];

    ParserState_t m_parser_state;
    ArgsParserState_t m_args_parser_state;
    int m_num_args;
    int m_args[MAX_ARGS];

    int m_width;
    int m_height;
    int m_x;
    int m_y;
    RawColor_t m_raw_text_color;
    RawColor_t m_raw_bg_color;
    bool m_cursor_enabled;

    cookie_read_function_t *m_read_func;
    void *m_read_cookie;
public:
    Vt100TerminalServer_t();

    FILE *fopen(cookie_read_function_t *read_func=NULL, void *read_cookie=NULL);

    virtual void print_char(char c) = 0;
    virtual RawColor_t rgb_to_raw_color(RgbColor_t rgb) = 0;
    virtual void hide_cursor();
    virtual void show_cursor();
    virtual void clear_screen();
    virtual void clear_line(int line_num);
    virtual void scroll(int num_lines);
    virtual void set_pos(int x, int y);

    void init(int width, int height);
    void set_text_color(ColorIndex_t vt100_color_index);
    void set_background_color(ColorIndex_t vt100_color_index);
    RgbColor_t vt100_color_index_to_rgb(ColorIndex_t vt100_color_index);
    RawColor_t vt100_color_index_to_raw(ColorIndex_t vt100_color_index);
    void cursor_home();

    void reset_parser();
    void reset();

    static bool is_csi_termination_char(char c);
    void handle_char(char c);
    void handle_ascii(char c);
    void handle_escape_sequence_end(char c);

    cookie_read_function_t *get_read_func() { return m_read_func; }
    void *get_read_cookie() { return m_read_cookie; }
};

class Vt100Terminal_t
{
public:
    void set_color_seq(int code);
    void set_text_color();
    void set_background_color();
    void cursor_home();
    void set_pos(int x, int y);
    void hide_cursor();
    void show_cursor();
    void clear_screen();
};
