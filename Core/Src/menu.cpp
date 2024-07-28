#include "menu.h"
#include "vt100.h"
#include "shell.h"

const char *Menu_t::s_color_settings[2] = { BG_BRIGHT_BLUE FG_BRIGHT_WHITE, BG_BRIGHT_WHITE FG_BRIGHT_BLUE };

void Menu_t::set_menu_items(MenuItem_t *menu_items, int num_items)
{
    m_menu_items = menu_items;
    m_num_items_total = num_items;
}

void Menu_t::set_device(FILE *f)
{
    m_device = f;
}

MenuItem_t *Menu_t::get_item(int index)
{
    if (index < m_num_items_total)
    {
        return &m_menu_items[index];
    }
    else
    {
        return NULL;
    }
}

MenuItem_t *Menu_t::get_next_item(MenuItem_t *item)
{
    item++;
    if (item >= m_menu_items + m_num_items_total)
    {
        return NULL;
    }
    else
    {
        return item;
    }
}

MenuItem_t *Menu_t::get_next_item_in_sub_menu(MenuItem_t *item)
{
    MenuItem_t *next_item = (item == m_cur_sub_menu) ? m_menu_items : item;
    do
    {
        next_item = get_next_item(next_item);
    } while (next_item != NULL && next_item->parent != m_cur_sub_menu);
    return next_item;
}

MenuItem_t *Menu_t::get_item_in_sub_menu(int index_in_cur_sub_menu)
{
    int idx = 0;
    MenuItem_t *item = m_cur_sub_menu;
    while (idx < index_in_cur_sub_menu)
    {
        item = get_next_item_in_sub_menu(item);
        idx++;
        if (item == NULL)
        {
            return NULL;
        }
    }
    return item;
}

int Menu_t::get_num_items_in_sub_menu(MenuItem_t *item)
{
    int i = 0;
    MenuItem_t *next_item = m_cur_sub_menu;
    while (next_item != NULL)
    {
        next_item = get_next_item_in_sub_menu(next_item);
        i++;
    }
    return i;
}

void Menu_t::open_sub_menu(MenuItem_t *item)
{
    if (item == m_menu_items)
    {
        m_selected_item = 1;
    }
    else
    {
        m_selected_item = 0;
    }
    m_cur_sub_menu = item;
    m_num_items_in_cur_sub_menu = get_num_items_in_sub_menu(m_cur_sub_menu);
    m_need_full_redraw = true;
}

void Menu_t::reset()
{
    open_sub_menu(m_menu_items);
    m_need_full_redraw = true;
}

void Menu_t::handle_user_input(UserInput_t input)
{
    switch (input)
    {
    case UP:
        if (m_selected_item > 0)
        {
            if (m_selected_item > 1 || get_item_in_sub_menu(0)->parent != NULL)
            {
                prev_entry_idx_to_redraw = m_selected_item;
                m_need_selector_redraw = true;
                m_selected_item--;
            }
        }
        break;
    case DOWN:
        if (m_selected_item < m_num_items_in_cur_sub_menu - 1)
        {
            prev_entry_idx_to_redraw = m_selected_item;
            m_need_selector_redraw = true;
            m_selected_item++;
        }
        break;
    default:
        break;
    }

    if (input == LEFT || input == RIGHT)
    {
        if (m_selected_item == 0)
        {
            open_sub_menu(m_cur_sub_menu->parent);
        }
        else if (get_item_in_sub_menu(m_selected_item)->type == MENU_ITEM_TYPE_SUB_MENU)
        {
            open_sub_menu(get_item_in_sub_menu(m_selected_item));
        }
    }
}

bool Menu_t::run_ui()
{
    int c = fgetc(m_device);
    if (c != FILE_READ_NO_MORE_DATA && c != EOF)
    {
        switch (c)
        {
        case 'w':
            handle_user_input(UP);
            break;
        case 's':
            handle_user_input(DOWN);
            break;
        case 'a':
            handle_user_input(LEFT);
            break;
        case 'd':
            handle_user_input(RIGHT);
            break;
        case 'q':
            return true;
        }
    }
    draw();
    return false;
}

void Menu_t::draw()
{
    if (m_need_full_redraw)
    {
        fprintf(m_device, "%s" VT100_HIDE_CURSOR VT100_CURSOR_HOME VT100_CLEAR_SCREEN, s_color_settings[false]);
        int menu_idx = 0;
        MenuItem_t *item = m_cur_sub_menu;
        while (item != NULL)
        {
            draw_menu_item(item, 1, 1 + menu_idx, menu_idx == m_selected_item);
            item = get_next_item_in_sub_menu(item);
            menu_idx++;
        }
        m_need_full_redraw = false;
    }
    else if (m_need_selector_redraw)
    {
        draw_menu_item(get_item_in_sub_menu(m_selected_item), 1, 1 + m_selected_item, true);
        draw_menu_item(get_item_in_sub_menu(prev_entry_idx_to_redraw), 1, 1 + prev_entry_idx_to_redraw, false);
        m_need_selector_redraw = false;
    }
}

void Menu_t::draw_menu_item(MenuItem_t *item, int x, int y, bool is_selected)
{
    fprintf(m_device, VT100_SET_YX "%s%s", y, x, s_color_settings[is_selected], item->name);
}
