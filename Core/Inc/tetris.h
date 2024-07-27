#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "vt100.h"

template<int WIDTH, int HEIGHT>
class TetrisGame_t
{
	enum FigureType_t
	{
		LINE,
		BOX,
		//LADDER,
		PIRAMYD,
		CORNER,
		NUM_FIGURES
	};

	enum FigureOrientation_t
	{
		FIGURE_ORIENTATION_DEFAULT,
		FIGURE_ORIENTATION_ROTATE1,
		FIGURE_ORIENTATION_ROTATE2,
		FIGURE_ORIENTATION_ROTATE3,
		NUM_ORIENTATIONS
	};

	static const int BITS_PER_BYTE = 8;
	static const int FIGURE_MAX_WIDTH = 4;
	static const int SPEED_UP_FALL_DELAY_MS = 50;
	static const int INITIAL_FALL_DELAY_MS = 300;
	static const uint8_t s_figures_bitmap[NUM_FIGURES][NUM_ORIENTATIONS][FIGURE_MAX_WIDTH];

	uint8_t m_bitmap[WIDTH][(HEIGHT + BITS_PER_BYTE - 1) / BITS_PER_BYTE];
	uint8_t m_prev_bitmap[WIDTH][(HEIGHT + BITS_PER_BYTE - 1) / BITS_PER_BYTE];
	FigureType_t active_figure_type;
	FigureOrientation_t active_figure_orientation;
	int active_figure_x;
	int active_figure_y;
	int score;
	int cur_fall_delay_ms;
	int normal_fall_delay_ms;
	bool m_is_score_updated;

public:
	enum UserInput_t
	{
		SHIFT_LEFT,
		SHIFT_RIGHT,
		ROTATE,
		SPEED_UP
	};

	TetrisGame_t();

	bool is_occupied(int x, int y);
	void set(int x, int y);
	void clear(int x, int y);
	bool is_figure_location_valid(int x, int y, FigureType_t fig, FigureOrientation_t orientation);
	bool rotate_figure();
	bool shift_figure(int shift_x, int shift_y);
	void land_figure();
	bool get_figure_bit(FigureType_t fig, FigureOrientation_t orientation, int ofs_x, int ofs_y);
	void get_new_figure();
	bool check_full_line(int y);
	void check_full_lines();
	void remove_line(int y);
	void update_score(int num_full_lines);
	void apply_figure();
	void remove_figure();

	void start_game();
	void game_over();
	void handle_user_input(UserInput_t input);
	void fall_one_step_down();

	void render(FILE *f);
	void render_background(FILE *f);
	int get_cur_fall_delay_ms();
};

template<int WIDTH, int HEIGHT>
const uint8_t TetrisGame_t<WIDTH, HEIGHT>::s_figures_bitmap[TetrisGame_t<WIDTH, HEIGHT>::NUM_FIGURES][TetrisGame_t<WIDTH, HEIGHT>::NUM_ORIENTATIONS][TetrisGame_t<WIDTH, HEIGHT>::FIGURE_MAX_WIDTH] = {
	/* LINE */
	{
		{ 0b1000,
		  0b1000,
		  0b1000,
		  0b1000 },
		{ 0b0000,
		  0b0000,
		  0b0000,
		  0b1111 },
		{ 0b0001,
		  0b0001,
		  0b0001,
		  0b0001 },
		{ 0b1111,
		  0b0000,
		  0b0000,
		  0b0000 },
	},

	/* BOX */
	{
		{ 0b0000,
		  0b0000,
		  0b0011,
		  0b0011 },
		{ 0b0000,
		  0b0000,
		  0b0011,
		  0b0011 },
		{ 0b0000,
		  0b0000,
		  0b0011,
		  0b0011 },
		{ 0b0000,
		  0b0000,
		  0b0011,
		  0b0011 },
	},

	/* PIRAMYD */
	{
		{ 0b0000,
		  0b0000,
		  0b0100,
		  0b1110 },
		{ 0b0000,
		  0b0001,
		  0b0011,
		  0b0001 },
		{ 0b0000,
		  0b0111,
		  0b0010,
		  0b0000 },
		{ 0b0000,
		  0b1000,
		  0b1100,
		  0b1000 },
	},

	/* CORNER */
	{
		{ 0b0000,
		  0b0001,
		  0b0001,
		  0b0011 },
		{ 0b0000,
		  0b0000,
		  0b0111,
		  0b0001 },
		{ 0b0110,
		  0b0100,
		  0b0100,
		  0b0000 },
		{ 0b0000,
		  0b0000,
		  0b0100,
		  0b0111 },
	}
};

template<int WIDTH, int HEIGHT>
TetrisGame_t<WIDTH, HEIGHT>::TetrisGame_t()
{

}

template<int WIDTH, int HEIGHT>
bool TetrisGame_t<WIDTH, HEIGHT>::is_occupied(int x, int y)
{
	return (m_bitmap[x][y / BITS_PER_BYTE] >> (y % BITS_PER_BYTE)) & 0x01;
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::set(int x, int y)
{
	m_bitmap[x][y / BITS_PER_BYTE] |= 1U << (y % BITS_PER_BYTE);
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::clear(int x, int y)
{
	m_bitmap[x][y / BITS_PER_BYTE] &= ~(1U << (y % BITS_PER_BYTE));
}

template<int WIDTH, int HEIGHT>
bool TetrisGame_t<WIDTH, HEIGHT>::is_figure_location_valid(int x, int y, FigureType_t fig, FigureOrientation_t orientation)
{
	for (int ofs_x = 0; ofs_x < FIGURE_MAX_WIDTH; ofs_x++)
	{
		for (int ofs_y = 0; ofs_y < FIGURE_MAX_WIDTH; ofs_y++)
		{
			if (get_figure_bit(fig, orientation, ofs_x, ofs_y) &&
				(is_occupied(x + ofs_x, y + ofs_y) || x + ofs_x < 0 || y + ofs_y < 0 || x + ofs_x >= WIDTH || y + ofs_y >= HEIGHT))
			{
				return false;
			}
		}
	}
	return true;
}

template<int WIDTH, int HEIGHT>
bool TetrisGame_t<WIDTH, HEIGHT>::rotate_figure()
{
	FigureOrientation_t new_orientation = (FigureOrientation_t)(((int)active_figure_orientation + 1) % NUM_ORIENTATIONS);
	remove_figure();
	bool is_ok = false;
	if (is_figure_location_valid(active_figure_x, active_figure_y, active_figure_type, new_orientation))
	{
		active_figure_orientation = new_orientation;
		is_ok = true;
	}
	apply_figure();
	return is_ok;
}

template<int WIDTH, int HEIGHT>
bool TetrisGame_t<WIDTH, HEIGHT>::shift_figure(int shift_x, int shift_y)
{
	int new_x = active_figure_x + shift_x;
	int new_y = active_figure_y + shift_y;
	remove_figure();
	bool is_ok = false;
	if (is_figure_location_valid(new_x, new_y, active_figure_type, active_figure_orientation))
	{
		remove_figure();
		active_figure_x = new_x;
		active_figure_y = new_y;
		apply_figure();
		is_ok = true;
	}
	apply_figure();
	return is_ok;
}

template<int WIDTH, int HEIGHT>
bool TetrisGame_t<WIDTH, HEIGHT>::get_figure_bit(FigureType_t fig, FigureOrientation_t orientation, int ofs_x, int ofs_y)
{
	return (s_figures_bitmap[(int)fig][(int)orientation][ofs_x] >> ofs_y) & 0x01;
}

template<int WIDTH, int HEIGHT>
bool TetrisGame_t<WIDTH, HEIGHT>::check_full_line(int y)
{
	for (int x = 0; x < WIDTH; x++)
	{
		if (is_occupied(x, y) == false)
		{
			return false;
		}
	}
	return true;
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::remove_line(int y)
{
	for (; y < HEIGHT - 1; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			if (is_occupied(x, y + 1))
			{
				set(x, y);
			}
			else
			{
				clear(x, y);
			}
		}
	}
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::update_score(int num_full_lines)
{
	if (num_full_lines == 0)
	{
	}
	else
	{
        if (num_full_lines == 1)
        {
            score += 10;
        }
        else if (num_full_lines == 2)
        {
            score += 30;
        }
        else if (num_full_lines == 3)
        {
            score += 50;
        }
        else
        {
            score += 80;
        }
        m_is_score_updated = false;
	}
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::check_full_lines()
{
	int num_full_lines = 0;
	for (int y = active_figure_y + FIGURE_MAX_WIDTH; y >= active_figure_y; y--)
	{
		if (check_full_line(y))
		{
			remove_line(y);
			num_full_lines++;
		}
	}
	update_score(num_full_lines);
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::apply_figure()
{
	for (int x = 0; x < FIGURE_MAX_WIDTH; x++)
	{
		for (int y = 0; y < FIGURE_MAX_WIDTH; y++)
		{
			if (get_figure_bit(active_figure_type, active_figure_orientation, x, y))
			{
				set(active_figure_x + x, active_figure_y + y);
			}
		}
	}
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::remove_figure()
{
	for (int x = 0; x < FIGURE_MAX_WIDTH; x++)
	{
		for (int y = 0; y < FIGURE_MAX_WIDTH; y++)
		{
			if (get_figure_bit(active_figure_type, active_figure_orientation, x, y))
			{
				clear(active_figure_x + x, active_figure_y + y);
			}
		}
	}
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::land_figure()
{
	check_full_lines();
	cur_fall_delay_ms = normal_fall_delay_ms;
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::get_new_figure()
{
	active_figure_x = WIDTH / 2 - 2;
	active_figure_y = HEIGHT - 4;
	active_figure_type = (FigureType_t)(rand() % NUM_FIGURES);
	active_figure_orientation = (FigureOrientation_t)(rand() % NUM_ORIENTATIONS);

	if (!is_figure_location_valid(active_figure_x, active_figure_y, active_figure_type, active_figure_orientation))
	{
		game_over();
	}
	else
	{
		apply_figure();
	}
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::start_game()
{
	for (int x = 0; x < WIDTH; x++)
	{
		for (int y = 0; y < HEIGHT; y++)
		{
			clear(x, y);
		}
	}
	get_new_figure();
	score = 0;
	m_is_score_updated = false;
	normal_fall_delay_ms = INITIAL_FALL_DELAY_MS;
	cur_fall_delay_ms = normal_fall_delay_ms;
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::game_over()
{
	start_game();
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::handle_user_input(UserInput_t input)
{
	if (input == SHIFT_LEFT)
	{
		shift_figure(-1, 0);
	}
	else if (input == SHIFT_RIGHT)
	{
		shift_figure(+1, 0);
	}
	else if (input == ROTATE)
	{
		rotate_figure();
	}
	else if (input == SPEED_UP)
	{
		cur_fall_delay_ms = SPEED_UP_FALL_DELAY_MS;
	}
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::fall_one_step_down()
{
	remove_figure();
	if (is_figure_location_valid(active_figure_x, active_figure_y - 1, active_figure_type, active_figure_orientation))
	{
		shift_figure(0, -1);
		apply_figure();
	}
	else
	{
		apply_figure();
		land_figure();
		get_new_figure();
	}
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::render_background(FILE *f)
{
    for (int y = 1; y <= HEIGHT; y++)
    {
        fprintf(f, "\e[%d;%dH|", 1, y);
        fprintf(f, "\e[%d;%dH|", 2 + WIDTH, y);
    }
}

template<int WIDTH, int HEIGHT>
void TetrisGame_t<WIDTH, HEIGHT>::render(FILE *f)
{
	for (size_t i = 0; i < sizeof(m_bitmap); i++)
	{
		int xor_res = ((uint8_t*)m_bitmap)[i] ^ ((uint8_t*)m_prev_bitmap)[i];
		if (xor_res != 0)
		{
			((uint8_t*)m_prev_bitmap)[i] = ((uint8_t*)m_bitmap)[i];
			int y = (i % sizeof(m_bitmap[0])) * BITS_PER_BYTE;
			int x = i / sizeof(m_bitmap[0]);
			for (int y_ofs = 0; y_ofs < BITS_PER_BYTE; y_ofs++)
			{
				if ((xor_res >> y_ofs) & 0x01)
				{
					bool val = (((uint8_t*)m_bitmap)[i] >> y_ofs) & 0x01;
					fprintf(f, "\e[%d;%dH%c", 2 + x, HEIGHT - y - y_ofs, val ? 'o' : ' ');
				}
			}
		}
	}

	if (!m_is_score_updated)
	{
	    fprintf(f, "\e[%d;%dH%4d", 2, 1, score);
	    m_is_score_updated = true;
	}
	fflush(f);
}

template<int WIDTH, int HEIGHT>
int TetrisGame_t<WIDTH, HEIGHT>::get_cur_fall_delay_ms()
{
	return cur_fall_delay_ms;
}
