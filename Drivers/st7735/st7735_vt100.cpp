#include "st7735_vt100.h"
#include "utils.h"

void St7735_Vt100_t::init(FontDef font)
{
    ST7735_Init();
    m_font = font;
    m_cur_scroll_pos = 0;
    Vt100TerminalServer_t::init(Utils_t::div_ceil_uint(ST7735_WIDTH, m_font.width), Utils_t::div_ceil_uint(ST7735_HEIGHT, m_font.height));
}

void St7735_Vt100_t::print_char(char c)
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

St7735_Vt100_t::RawColor_t St7735_Vt100_t::rgb_to_raw_color(RgbColor_t rgb)
{
    // 16 bits color mode: BIT [15:11] - B, BITS[10:5] - G, BITS[4:0] - R
    return ST7735_COLOR565(rgb.r, rgb.g, rgb.b);
}

void St7735_Vt100_t::scroll(int num_lines)
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
