#ifndef UTILS_H
#define UTILS_H

#include <termios.h>

// Terminal configuration structure
typedef struct {
    int HEIGHT;
    int WIDTH;
    int cursor_pos_x;
    int cursor_pos_y;
    int ms_pos_x;
    int ms_pos_y;
	int MS_POS_DX;
	int MS_POS_DY;
    struct termios original_state;
} terminal_conf;

extern terminal_conf Term_Conf;

// Struct for a 2D point
struct Point2D {
    double x;
    double y;
};

// Struct for a 3D point
typedef struct {
    double x;
    double y;
    double z;
} Point3D;

// Terminal and raw mode functions
void enable_virtual_window();
void disable_virtual_window();
void enable_raw_mode();
void disable_raw_mode();
int get_terminal_size(int *rows, int *cols);
int get_terminal_size2(int *rows, int *cols);

// Cursor-related functions
void move_cursor(int x, int y);
void move_cursor_NO_REASSGN(int x, int y);
void move_cursor_translate(double x, double y);

// Utility functions
void clear_screen();
double translateX(double x);
double translateY(double y);
void cleanup_threads();
void die(const char *s);

#endif // UTILS_H

