#include <pthread.h>
#include <asm-generic/ioctls.h>
#include <bits/pthreadtypes.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <math.h>

#define ESC "\033"
#define DO_SOMETHING ESC "[?1049h"  // Switch to alternate screen buffer
#define DO_SOMETHING_OFF ESC "[?1049l"  // Switch back to normal screen buffer
#define MOVE_CURSOR_TO(x, y) ESC "[%d;%dH", x, y
#define CLEAR_SCREEN ESC "[2J"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define ENABLE_MS "[?1003h" // enable mouse reporting mode. This or ESC[?1000h
#define DISABLE_MS "[?1003l" // disable ms reporting mode

#define NUM_THREADS 3

int STOP_READING = 1;
pthread_t threads[NUM_THREADS];
pthread_mutex_t mutex;

typedef struct {
	int rows;
	int cols;
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

int D_DIST = 1;

double translateX(double x) {
	double val = x + (Term_Conf.cols / 2.0);
	return val;
}

double translateY(double y) {
	double val = (Term_Conf.rows / 2.0) - y;
	if (val < 0) {
		return val * -1;
	}
	return val;
}

void cleanup_threads() {
    // pthread_mutex_destroy(&mutex);
    // pthread_exit(NULL);
}

void init_thread() {
	pthread_t threads[NUM_THREADS];
	pthread_mutex_init(&mutex, NULL);
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

void waitFor (unsigned int secs) {
    unsigned int retTime = time(0) + secs;   // Get finishing time.
    while (time(0) < retTime);               // Loop until it arrives.
}

// fflush(stdout) is a function in C that forces 
// the output buffer of the stdout stream (standard output) 
// to be written to the terminal (or any other associated output device) immediately.
// move_cursor updates the global x and y cursor positions
void move_cursor(int x, int y) {
	Term_Conf.cursor_pos_x = x;
	Term_Conf.cursor_pos_y = y;

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

void enable_virtual_window() {
	printf(DO_SOMETHING);
	fflush(stdout);
}

void disable_virtual_window() {
	printf(DO_SOMETHING_OFF);
	fflush(stdout);
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

void print_live_size(int *p_x, int *p_y) {
	move_cursor(3, 3);
	printf("%d %d", *p_x, *p_y);

	move_cursor(*p_x, *p_y);
}

void print_at_pos(char *text, int pos_x, int pos_y) {
	int temp_x = Term_Conf.cursor_pos_x;
	int temp_y = Term_Conf.cursor_pos_y;

	move_cursor(pos_x, pos_y);
	printf("%s", text);

	// reset to previous pos
	move_cursor(temp_x, temp_y);
}

// get random double within a range
double random_double(double min, double max) {
    return min + (rand() / (double)RAND_MAX) * (max - min);
}

void init_window() {
	init_thread();
	atexit(disable_raw_mode);

	int rows, cols;
	if (get_terminal_size(&rows, &cols) == -1) {
		die("get_terminal_size");
	}

	Term_Conf.cursor_pos_x = 1;
	Term_Conf.cursor_pos_y = 1;

	Term_Conf.cols = cols;
	Term_Conf.rows = rows;

	move_cursor_NO_REASSGN(1, 3);
	printf("Max cols: %d Max rows: %d ", Term_Conf.cols, Term_Conf.rows);
	move_cursor_NO_REASSGN(Term_Conf.cursor_pos_x, Term_Conf.cursor_pos_y);
}

// Goes left to right, top to bottom
void draw_rectangle(int x, int y, int l, int b) {
	for (int i_x = 0; i_x < l; i_x++) {
		for (int i_y = 0; i_y < b; i_y++) {
			pthread_mutex_lock(&mutex);
			move_cursor_NO_REASSGN(x + i_x, y + i_y);
			pthread_mutex_unlock(&mutex);
			printf("%d", i_y);
		}
	}
}

void draw_circle(int x, int y, int r) {
	int r_sq = r*r;
	for (int i_x = x-r; i_x < x+r; i_x++) {
		for (int i_y = y-r; i_y < y+r; i_y++) {
			// pt1 = (i_x, i_y)
			// pt2 = (x, y)
			int dist = (i_x-r)*(i_x-r) + (i_y-r)*(i_y-r);
			pthread_mutex_lock(&mutex);
			move_cursor_NO_REASSGN(x + i_x, y + i_y);
			pthread_mutex_unlock(&mutex);
			printf("%d", dist);
			if (dist <= r_sq) {
				printf("%d", i_y);
			}
		}
	}
}

void* read_user_input(void* thread_id) {
	char c;
	int pos_x = 5;
	int pos_y = 5;

	int rec_x = 4;

	while (read(STDIN_FILENO, &c, 1) == 1) {
		switch (c) {
			case 'q':
				STOP_READING = 0;

			case 'w':
				if (D_DIST >= 0) {
					D_DIST += 1;
				}
				if (Term_Conf.cursor_pos_y > 1) {
					pos_y--;
				}
				break;
				// printf("W pressed\n");

			case 's':
				if (D_DIST > 0) {
					D_DIST -= 1;
				}
				if (Term_Conf.cursor_pos_y < Term_Conf.rows) {
					pos_y++;
				}
				break;
				// printf("S pressed\n");
			
			case 'd':
				if (Term_Conf.cursor_pos_x < Term_Conf.cols) {
					pos_x++;
				}
				break;

			case 'a':
				if (Term_Conf.cursor_pos_x > 1) {
					pos_x--;
				}
				break;
		}

		if (!STOP_READING) {
			break;
		}
		// clear_screen();

		// pthread_mutex_lock(&mutex);
		// move_cursor(pos_x, pos_y);
		// pthread_mutex_unlock(&mutex);

		// usleep(100000);

		// move_cursor_NO_REASSGN(1, 4);
		// printf("X: %d Y: %d", Term_Conf.cursor_pos_x, Term_Conf.cursor_pos_y);
		// move_cursor_NO_REASSGN(Term_Conf.cursor_pos_x, Term_Conf.cursor_pos_y);
	}

    pthread_exit(NULL);
}

void* read_ms_input(void* thread_id) {
	char buf[32];
	int nread;

	while (STOP_READING) {
		nread = read(STDIN_FILENO, buf, sizeof(buf) - 1);
		if (nread <= 0) {
			continue;
		}

		buf[nread] = '\0';

		if (buf[0] == '\x1b' && buf[1] == '[' && buf[2] == '<') {
			int button, x, y;

			if (sscanf(&buf[3], "%d;%d;%d", &button, &x, &y) == 3) {
				pthread_mutex_lock(&mutex);
				Term_Conf.ms_pos_x = x;
				Term_Conf.ms_pos_y = y;
				pthread_mutex_unlock(&mutex);

				// Exit on right-click (button == 3)
				if (button == 3) {
					break;
				}
			}
		}
	}

	pthread_exit(NULL);
}

void drawPoint(double x_orig, double y_orig, double z_orig, double d_dist, double* new_x, double* new_y) {
	*new_x = (d_dist * x_orig) / z_orig;
	*new_y = (d_dist * y_orig) / z_orig;
}

void drawSquare(double x, double y, double z, double w, double h, int col_code) {
	double x1 = x;
	double x2 = x + w;

	double y1 = y;
	double y2 = y + h;

	double new_x1 = (x1 * D_DIST) / z;
	double new_x2 = (x2 * D_DIST) / z;

	double new_y1 = (y1 * D_DIST) / z;
	double new_y2 = (y2 * D_DIST) / z;

	for (int i_x = new_x1; i_x < new_x2; i_x++) {
		for (int i_y = new_y1; i_y < new_y2; i_y++) {
			move_cursor_translate(i_x, i_y);
			// printf("#");
			printf("\033[%dm#\033[0m\n", col_code);
			fflush(stdout); // Not printing without this; when NUM_PARTICLES is 1
			pthread_mutex_unlock(&mutex);
		}
	}

	// for (int i_x = x; i_x < x + w; i_x++) {
	// 	for (int i_y = y; i_y < y + h; i_y++) {
	// 		double new_x = (i_x * D_DIST) / z;
	// 		double new_y = (i_y * D_DIST) / z;
	//
	// 		move_cursor_translate(new_x, new_y);
	// 		printf("#");
	// 		fflush(stdout); // Not printing without this; when NUM_PARTICLES is 1
	// 		pthread_mutex_unlock(&mutex);
	// 	}
	// }
}

void* animation(void* thread_id) {
	double degrees = 1.0;
    double radians = degrees * M_PI / 180.0;

	double x_s = 0;
	double sq_side = 10;
	double z_s = 5.0;

	Point3D sq1[4] = {
		{x_s, x_s, z_s},
		{x_s + sq_side, x_s, z_s},
		{x_s, x_s + sq_side, z_s},
		{x_s + sq_side, x_s + sq_side, z_s},
	};

	while (STOP_READING) {
		usleep(200000);
		clear_screen();

		// for (int i = 0; i < 4; i++) {
		// 		Point3D* p = &sq1[i];
		// 		pthread_mutex_lock(&mutex);
		//
		// 		double new_x, new_y;
		// 		new_x = (p->x * d_dist) / p->z;
		// 		new_y = (p->y * d_dist) / p->z;
		//
		// 		move_cursor_NO_REASSGN(new_x / 2.0 + (Term_Conf.cols / 2.0), new_y / 2.0 + (Term_Conf.rows / 2.0));
		// 		printf("#");
		// 		fflush(stdout); // Not printing without this; when NUM_PARTICLES is 1
		// 		pthread_mutex_unlock(&mutex);
		// }


		pthread_mutex_lock(&mutex);
		move_cursor_NO_REASSGN(1, 1);
		printf("WIDTH: %d; HEIGHT: %d", Term_Conf.cols, Term_Conf.rows);
		move_cursor_NO_REASSGN(1, 2);
		printf("DIST %d", D_DIST);
		fflush(stdout);
		pthread_mutex_unlock(&mutex);

		// Furthest objects first
		drawSquare(-10, -5, 15, 5, 10, 33);
		drawSquare(10, -5, 15, 5, 10, 33);

		drawSquare(-10, -5, 10, 5, 10, 32);
		drawSquare(10, -5, 10, 5, 10, 32);

		drawSquare(-10, -5, 5, 5, 10, 31);
		drawSquare(10, -5, 5, 5, 10, 31);

	}

    pthread_exit(NULL);
}

int main() {
	enable_raw_mode();
	init_window();

	srand((unsigned int)time(NULL));

	long read_input_id = 0;
	pthread_create(&threads[read_input_id], NULL, read_user_input, (void*)read_input_id);

	long animation_id = 1;
	pthread_create(&threads[animation_id], NULL, animation, (void*)animation_id);

	// long ms_event_id = 2;
	// pthread_create(&threads[ms_event_id], NULL, read_ms_input, (void*)ms_event_id);

	pthread_join(threads[read_input_id], NULL);
	pthread_join(threads[animation_id], NULL);
	// pthread_join(threads[ms_event_id], NULL);

    pthread_mutex_destroy(&mutex);
    pthread_exit(NULL);
}
