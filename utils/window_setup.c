#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>

#define DO_SOMETHING ESC "[?1049h"  // Switch to alternate screen buffer
#define DO_SOMETHING_OFF ESC "[?1049l"  // Switch back to normal screen buffer
#define MOVE_CURSOR_TO(x, y) ESC "[%d;%dH", x, y
#define ESC "\033"
#define CLEAR_SCREEN ESC "[2J"

#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define ENABLE_MS "[?1003h" // enable mouse reporting mode. This or ESC[?1000h
#define DISABLE_MS "[?1003l" // disable ms reporting mode

typedef struct {
	int HEIGHT;
	int WIDTH;
	int cursor_pos_x;
	int cursor_pos_y;
	int ms_pos_x;
	int ms_pos_y;
	struct termios original_state;
} terminal_conf;

terminal_conf Term_Conf;

struct Point2D {
	double x;
	double y;
};

typedef struct {
	double x;
	double y;
	double z;
} Point3D;

void enable_virtual_window() {
	printf(DO_SOMETHING);
	fflush(stdout);
}

void disable_virtual_window() {
	printf(DO_SOMETHING_OFF);
	fflush(stdout);
}

double translateX(double x) {
	double val = x + (Term_Conf.WIDTH / 2.0);
	return val;
}

double translateY(double y) {
	double val = (Term_Conf.HEIGHT / 2.0) - y;
	if (val < 0) {
		return val * -1;
	}
	return val;
}

void cleanup_threads() {
    // pthread_mutex_destroy(&mutex);
    // pthread_exit(NULL);
}

void disable_raw_mode() {
	cleanup_threads();

	printf(DISABLE_MS);
	printf(SHOW_CURSOR);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &Term_Conf.original_state);

    // Switch to normal scrn buff
	disable_virtual_window();
}

int get_terminal_size(int *rows, int *cols) {
    struct winsize ws;

	// Catching err
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    }

    *cols = ws.ws_col;
    *rows = ws.ws_row;

	return 0;
}

// Hard way?
int get_terminal_size2(int *rows, int *cols) {
	struct winsize ws;
	if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
		// editorReadKey();
		return -1;
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

// Canonical mode -> raw mode
void enable_raw_mode() {
	enable_virtual_window();
	printf(HIDE_CURSOR);
	printf(ENABLE_MS);

	tcgetattr(STDIN_FILENO, &Term_Conf.original_state);
	// atexit(disable_raw_mode);

	struct termios raw = Term_Conf.original_state;

	// c_lflag - local flags (misc flags)
	// c_iflag - input flag
	// c_oflag - output flag
	// c_cflag - control flag
	// Turning off
	// ECHO - Typed text wont appear on screen like usual
	// ICANON - Able to read input byte by byte instead of line by line
	raw.c_lflag &= ~(ECHO | ICANON);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	perror(s);
	exit(1);
}

void clear_screen() {
	printf(CLEAR_SCREEN);
	fflush(stdout);
}

// fflush(stdout) is a function in C that forces 
// the output buffer of the stdout stream (standard output) 
// to be written to the terminal (or any other associated output device) immediately.
// move_cursor updates the global x and y cursor positions
void move_cursor(int x, int y) {
	// Term_Conf.cursor_pos_x = x;
	// Term_Conf.cursor_pos_y = y;

    printf(MOVE_CURSOR_TO(y, x));
	fflush(stdout);
}

// move cursor without updating pos
void move_cursor_NO_REASSGN(int x, int y) {
    printf(MOVE_CURSOR_TO(y, x));
	fflush(stdout);
}

// Move without reassigning and translate
void move_cursor_translate(double x, double y) {
	double new_x = translateX(x);
	double new_y = translateY(y);

    printf(MOVE_CURSOR_TO((int)new_y, (int)new_x));
	fflush(stdout);
}

