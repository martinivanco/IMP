/************************************************
 * File: main.c (original)                      *
 * Author: Martin Ivanco (xivanc03)             *
 * Project: IMP - Hra HAD                       *
 ************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fitkitlib.h>
#include <keyboard/keyboard.h>

/* Macros and constants */
#define MASK_P6 0xff
#define MASK_P4 0x0f
#define MASK_P1 0xf0

#define SNAKE_UP 100
#define SNAKE_RIGHT 101
#define SNAKE_DOWN 102
#define SNAKE_LEFT 103

#define ONE_BIT ((unsigned char) 1)


/* Snake's globals */
unsigned char display[8][8];
unsigned char snake_head[2];
unsigned char snake_direction;
unsigned char snake[62];
unsigned char snake_size;
unsigned char food_pos[2];


/* Obligatory part */
void print_user_help(void) {}

unsigned char decode_user_cmd(char *cmd_ucase, char *cmd) {
  return (CMD_UNKNOWN);
}

void fpga_initialized() {}


/* Keyboard handling functions */ 
int keyboard_idle() {
	return key_decode(read_word_keyboard_4x4());
}

void change_direction() {
	int new_dir;
	terminal_idle();
	switch(keyboard_idle()) {
		case '2':
			if (snake_direction != SNAKE_DOWN)
				snake_direction = SNAKE_UP;
			break;
		case '4':
			if (snake_direction != SNAKE_RIGHT)
				snake_direction = SNAKE_LEFT;
			break;
		case '6':
			if (snake_direction != SNAKE_LEFT)
				snake_direction = SNAKE_RIGHT;
			break;
		case '8':
			if (snake_direction != SNAKE_UP)
				snake_direction = SNAKE_DOWN;
			break;
		default:
			break;
	}
}

int check_key(char check) {
	terminal_idle();
	if (keyboard_idle() == check)
		return 0;
	else
		return 1;
}


/* Iterate through rows of display once */
void show_display() {
	int row;
	int col;
	int wait;
	for (row = 0; row < 8; row++) {
		/* Clear the output */
		P6OUT &= ~MASK_P6;
		P4OUT |= MASK_P4;
		P1OUT |= MASK_P1;

		/* Set on appropriate columns */
		for (col = 0; col < 8; col++) {
			if (display[col][row] == 1)
				P6OUT |= ONE_BIT << col;
		}

		/* Ground appropriate row */
		if (row < 4)
			P4OUT &= ~(ONE_BIT << row);
		else
			P1OUT &= ~(ONE_BIT << row);

		/* Delay */
		for (wait = 0; wait < 0x0020; wait++);
	}
}

/* Auxiliary debug function */
void print_display() {
	int i = 0;
	for (; i < 8; i++) {
		int j = 0;
		for (; j < 8; j++)
			term_send_num(display[j][i]);
		term_send_crlf();
	}
	term_send_crlf();
}

/* Refill display array */
void redraw() {
	/* Clear display */
	int i = 0; 
	for (; i < 8; i++) {
		int j = 0;
		for (; j < 8; j++)
			display[i][j] = 0;
	}

	/* Put on food and head */
	display[food_pos[0]][food_pos[1]] = 1;
	display[snake_head[0]][snake_head[1]] = 1;

	/* Put on the rest of snake body */
	unsigned char position[2];
	position[0] = snake_head[0];
	position[1] = snake_head[1];
	i = 0;
	for (; i < snake_size - 1; i++) {
		if (snake[i] == SNAKE_UP)
			position[1] = (unsigned char) (((unsigned char) (position[1] - 1)) % 8);
		else if (snake[i] == SNAKE_RIGHT)
			position[0] = (unsigned char) ((position[0] + 1) % 8);
		else if (snake[i] == SNAKE_DOWN)
			position[1] = (unsigned char) ((position[1] + 1) % 8);
		else
			position[0] = (unsigned char) (((unsigned char) (position[0] - 1)) % 8);

		display[position[0]][position[1]] = 1;
	}
}

void generate_food() {
	unsigned char new_food;
	while (1) {
		/* Generate random position */
		new_food = (unsigned char) ((unsigned char) rand() % 64);
		food_pos[0] = (unsigned char) (new_food / 8);
        food_pos[1] = (unsigned char) (new_food % 8);

        /* Check if generated position isn't part of snake */
        if (display[food_pos[0]][food_pos[1]] == 0)
        	break;
	}
}

void initialize_snake() {
	/* Place snake head */
	snake_head[0] = 2;
	snake_head[1] = 0;
	snake_direction = SNAKE_RIGHT;

	/* Place snake body */
	snake_size = 3;
	snake[0] = SNAKE_LEFT;
	snake[1] = SNAKE_LEFT;

	redraw();
	generate_food();
	redraw();
}

void snake_move(unsigned char *new_head, unsigned char new_next) {
	/* Set new head position */
	snake_head[0] = new_head[0];
	snake_head[1] = new_head[1];

	/* Move snake body */
	int i = snake_size - 2;
	for (; i > 0; i--)
		snake[i] = snake[i-1];
	snake[0] = new_next;
}

void snake_eat(unsigned char new_next) {
	/* Grow snake an generate new food */
	snake_size += 1;
	snake_move(food_pos, new_next);
	generate_food();
}

unsigned char game_step() {
	/* Find position ahead of snake */
	unsigned char ahead[2];
	unsigned char head_next_dir;
	switch (snake_direction) {
		case SNAKE_UP:
			ahead[0] = snake_head[0];
			ahead[1] = (unsigned char) (((unsigned char) (snake_head[1] - 1)) % 8);
			head_next_dir = SNAKE_DOWN;
			break;
		case SNAKE_RIGHT:
			ahead[0] = (unsigned char) ((snake_head[0] + 1) % 8);
			ahead[1] = snake_head[1];
			head_next_dir = SNAKE_LEFT;
			break;
		case SNAKE_DOWN:
			ahead[0] = snake_head[0];
			ahead[1] = (unsigned char) ((snake_head[1] + 1) % 8);
			head_next_dir = SNAKE_UP;
			break;
		case SNAKE_LEFT:
			ahead[0] = (unsigned char) (((unsigned char) (snake_head[0] - 1)) % 8);
			ahead[1] = snake_head[1];
			head_next_dir = SNAKE_RIGHT;
			break;
		default:
			break;
	}

	/* Evaluate it and set the display matrix accordingly */
	if (display[ahead[0]][ahead[1]] == 0)
		snake_move(ahead, head_next_dir);
	else {
		if ((ahead[0] == food_pos[0]) && (ahead[1] == food_pos[1]))
			snake_eat(head_next_dir);
		else
			return 1;
	}
	redraw();
	return 0;
}

unsigned char start_game() {
	int i;
	/* Game loop */
	while (1) {
		if (game_step() != 0)
			return 0; // player lost

		if (snake_size == 30)
			return 1; // player won

		/* Display snake for a while and check for change of direction */
		for (i = 0; i < 0x0300; i++) {
			show_display();
			change_direction();
		}
	}
}

void set_win_screen() {
	/* Clear display */
	int i = 0; 
	for (; i < 8; i++) {
		int j = 0;
		for (; j < 8; j++)
			display[i][j] = 0;
	}

	/* Draw winning screen */
	i = 0;
	for (; i < 2; i++) {
		display[7][1+i] = 1;
		display[6][2+i] = 1;
		display[5][3+i] = 1;
		display[4][4+i] = 1;
		display[3][5+i] = 1;
		display[2][6+i] = 1;
		display[1][5+i] = 1;
		display[0][4+i] = 1;
	}
}

void set_lose_screen() {
	/* Clear display */
	int i = 0; 
	for (; i < 8; i++) {
		int j = 0;
		for (; j < 8; j++)
			display[i][j] = 0;
	}

	/* Draw lose screen */
	i = 0;
	for (; i < 8; i++) {
		display[i][i] = 1;
		display[i][7-i] = 1;
	}
}

int main() {
	initialize_hardware();
	keyboard_init();
	WDG_stop();

	/* Set appropriate bits as output */
	P6DIR |= MASK_P6;
	P4DIR |= MASK_P4;
	P1DIR |= MASK_P1;

	/* Main loop */
	while (1) {
		initialize_snake();

		/* Wait for user to press start (5) */
		while (check_key('5'))
			show_display();

		unsigned char res = start_game();
		if (res)
			set_win_screen();
		else
			set_lose_screen();

		/* Wait for user to press restart (*) */
		while (check_key('*'))
			show_display();
	}
    return 0;
}

