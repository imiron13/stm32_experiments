#pragma once

#include <stdio.h>

enum MenuItemType_t
{
    MENU_ITEM_TYPE_INT,
    MENU_ITEM_TYPE_BOOL,
    MENU_ITEM_TYPE_SELECT,
    MENU_ITEM_TYPE_STRING,
    MENU_ITEM_TYPE_SUB_MENU,
};

struct MenuItem_t;

struct MenuItem_t
{
    MenuItemType_t type;
    const char *name;
    int value;
    MenuItem_t *parent;
};

class Menu_t
{
    static const char *s_color_settings[2];
    MenuItem_t *m_menu_items;
    int m_num_items_total;
    FILE *m_device;
    int m_selected_item;
    int m_num_items_in_cur_sub_menu;
    MenuItem_t *m_cur_sub_menu;
    bool m_need_full_redraw;
    bool m_need_selector_redraw;
    int prev_entry_idx_to_redraw;
public:
    enum UserInput_t
    {
        UP,
        DOWN,
        LEFT,
        RIGHT,

        TEXT_INPUT
    };

    void reset();
    void set_menu_items(MenuItem_t *menu_items, int num_items);
    void set_device(FILE *f);

    void open_sub_menu(MenuItem_t *item);

    MenuItem_t *get_item(int index);
    MenuItem_t *get_item_in_sub_menu(int index_in_cur_sub_menu);
    MenuItem_t *get_next_item(MenuItem_t *item);
    MenuItem_t *get_next_item_in_sub_menu(MenuItem_t *item);
    int get_num_items_in_sub_menu(MenuItem_t *item);

    void draw_menu_item(MenuItem_t *item, int x, int y, bool is_selected);
    void handle_user_input(UserInput_t input);
    void handle_text_input(char c);
    void draw();
    bool run_ui();
};
