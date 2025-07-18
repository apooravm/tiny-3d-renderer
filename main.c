#include <pthread.h>
#include <asm-generic/ioctls.h>
#include <bits/pthreadtypes.h>
#include <stddef.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <math.h>
#include "utils/window_setup.h"

#define NUM_THREADS 3
#define SQUARE_TRIANGLE_COUNT 2
#define CUBE_TRIANGLE_COUNT 12
#define vec1 vecs[0]
#define vec2 vecs[1]
#define vec3 vecs[2]

// homogeneous 4D vector
typedef struct {
	double x, y, z, w;
} Vec4 ;

// 4x4 matrix
typedef struct {
	double m[4][4];
} Mat4;

typedef struct {
	Vec4 vecs[3];
} Triangle;

typedef struct {
	Triangle *tris;
	size_t numTris;
} Mesh;

Mesh double_tris;
size_t double_tris_size = 1;

Mesh* SquareMesh;
Mesh* CubeMesh;

// using const to prevent input modification
Vec4 mat4_mult_vec4(const Mat4* mat, const Vec4* vec) {
	Vec4 result;

	result.x = mat->m[0][0] * vec->x + mat->m[1][0] * vec->y + mat->m[2][0] * vec->z + mat->m[3][0];
	result.y = mat->m[0][1] * vec->x + mat->m[1][1] * vec->y + mat->m[2][1] * vec->z + mat->m[3][1];
	result.z = mat->m[0][2] * vec->x + mat->m[1][2] * vec->y + mat->m[2][2] * vec->z + mat->m[3][2];
	result.w = mat->m[0][3] * vec->x + mat->m[1][3] * vec->y + mat->m[2][3] * vec->z + mat->m[3][3];

	return result;
}

Vec4 mat4_mult_vec4_2(const Mat4* mat, const Vec4* vec) {
	Vec4 result;

	result.x = vec->x * mat->m[0][0] + vec->y * mat->m[0][1] + vec->z * mat->m[0][2] + vec->w * mat->m[0][3];
	result.y = vec->x * mat->m[1][0] + vec->y * mat->m[1][1] + vec->z * mat->m[1][2] + vec->w * mat->m[1][3];
	result.z = vec->x * mat->m[2][0] + vec->y * mat->m[2][1] + vec->z * mat->m[2][2] + vec->w * mat->m[2][3];
	result.w = vec->x * mat->m[3][0] + vec->y * mat->m[3][1] + vec->z * mat->m[3][2] + vec->w * mat->m[3][3];

	// if (result.z <= 0.9f) {
	// 	result.z = 0.9f;
	// }

	return result;
}

int STOP_READING = 1;
int PAUSE_LOOP = 1;
int debug_on = 1;

pthread_t threads[NUM_THREADS];
pthread_mutex_t mutex;

int D_DIST = 1;
int W_DEF = 1;
double ASPECT_RATIO;
double FOV_ANGLE = 90;
double FOV;
double Z_NORM;
double fNear = 0.1f;
double fFar = 1000.0f;
double fFov = 90.0f;
double fFovRad;
Mat4 projection_matrix = { 0 };

// Convert to cartesian coordinates
// Works with int. Idk if that bad
void conv_from_cart(int x, int y, int* ox, int* oy)  {
	*ox = x + (int)Term_Conf.cols / 2;
	*oy = (-1 * y) + (int)Term_Conf.rows / 2;
}

// Print character from cartesian coord
// Converts cartesian to whatever this is bruh idk
void print_from_cart(double ix, double iy) {
	int ox, oy;
	conv_from_cart(ix, iy, &ox, &oy);

	pthread_mutex_lock(&mutex);
	move_cursor_NO_REASSGN(ox, oy);
	printf("#");
	pthread_mutex_unlock(&mutex);
}

void DrawLine(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int err = dx - dy;
    int maxIter = (dx > dy) ? dx : dy;

    for (int i = 0; i <= maxIter; i++) {
        int t_x = x1 + Term_Conf.cols / 2;
        int t_y = (-1 * y1) + Term_Conf.rows / 2;

        // Boundary check before drawing
        if (t_x >= 0 && t_x < Term_Conf.cols && t_y >= 0 && t_y < Term_Conf.rows) {
            move_cursor_NO_REASSGN(t_x, t_y);
            printf("*");
        }

        int e2 = err * 2;

        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }

        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void draw_triangle(Triangle* tri) {
	DrawLine(tri->vecs[0].x, tri->vecs[0].y, tri->vecs[1].x, tri->vecs[1].y);
	DrawLine(tri->vecs[1].x, tri->vecs[1].y, tri->vecs[2].x, tri->vecs[2].y);
	DrawLine(tri->vecs[2].x, tri->vecs[2].y, tri->vecs[0].x, tri->vecs[0].y);
}

void init_thread() {
	pthread_t threads[NUM_THREADS];
	pthread_mutex_init(&mutex, NULL);
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

			case 'n':
				PAUSE_LOOP = 0;

			case 'w':
				if (W_DEF >= 0) {
					W_DEF += 1;
				}
				if (Term_Conf.cursor_pos_y > 1) {
					pos_y--;
				}
				break;
				// printf("W pressed\n");

			case 's':
				if (W_DEF > 0) {
					W_DEF -= 1;
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

// Should NOT update the points in place
// Return the new point to be drawn
Vec4 rotate_xyz(Vec4* v, double ax, double ay, double az) {
    double x = v->x;
    double y = v->y;
    double z = v->z;

    // Rotate X
    double cosx = cosf(ax);
    double sinx = sinf(ax);
    double y1 = y * cosx - z * sinx;
    double z1 = y * sinx + z * cosx;

    // Rotate Y
    double cosy = cosf(ay);
    double siny = sinf(ay);
    double x2 = x * cosy + z1 * siny;
    double z2 = -x * siny + z1 * cosy;

    // Rotate Z
    double cosz = cosf(az);
    double sinz = sinf(az);
    double x3 = x2 * cosz - y1 * sinz;
    double y3 = x2 * sinz + y1 * cosz;

    Vec4 result;
    result.x = x3;
    result.y = y3;
    result.z = z2;
    result.w = v->w;

    return result;
}


void rotate_x(double* x, double* y, double* z, double angle) {
	double new_y = *y * cos(angle) - *z * sin(angle);
	double new_z = *y * sin(angle) + *z * cos(angle);

	*y = new_y;
	*z = new_z;
}

void rotate_y(double* x, double* y, double* z, double angle) {
    double new_x = *x * cos(angle) + *z * sin(angle);
    double new_z = -(*x) * sin(angle) + *z * cos(angle);

    *x = new_x;
    *z = new_z;
}

void rotate_z(double* x, double* y, double* z, double angle) {
    double new_x = *x * cos(angle) - *y * sin(angle);
    double new_y = *x * sin(angle) + *y * cos(angle);

    *x = new_x;
    *y = new_y;
}

void rotate_triangle(Triangle* tri, double ax, double ay, double az) {
	for (int j = 0; j < 3; j++) {
		tri->vecs[j] = rotate_xyz(&tri->vecs[j], ax, ay, az);

	}
}

void translate_triangle(Triangle* tri, double val) {
	for (int j = 0; j < 3; j++) {
		tri->vecs[j].z += val;
	}
}

void project_triangle(Triangle* tri) {
	tri->vecs[0] = mat4_mult_vec4_2(&projection_matrix, &tri->vecs[0]);
	tri->vecs[1] = mat4_mult_vec4_2(&projection_matrix, &tri->vecs[1]);
	tri->vecs[2] = mat4_mult_vec4_2(&projection_matrix, &tri->vecs[2]);
}

void normalise_triangle(Triangle* tri) {
	for (int i = 0; i < 3; i++) {
		// if (fabs(tri->vecs[i].w) < 0.01f) {
		// 	continue;
		// }

		// move_cursor_NO_REASSGN(0, 14);
		// printf("inside func 1: %f, %f, %f, %f", tri->vecs[0].x, tri->vecs[0].y, tri->vecs[0].z, tri->vecs[0].w);
		if (tri->vecs[i].w != 0.0f) {
			// Full perspective divide FIRST
			tri->vecs[i].x /= tri->vecs[i].w;
			tri->vecs[i].y /= tri->vecs[i].w;
			tri->vecs[i].z /= tri->vecs[i].w;
			tri->vecs[i].w /= tri->vecs[i].w;
		}
		// move_cursor_NO_REASSGN(0, 15);
		// printf("inside func 2: %f, %f, %f, %f", tri->vecs[0].x, tri->vecs[0].y, tri->vecs[0].z, tri->vecs[0].w);
	}
}

void dump_vertex_to_debug_file(Triangle* tri, int val) {
    FILE* debugFile = fopen("debug.txt", "a");
    if (debugFile == NULL) {
        perror("Failed to open debug file");
        exit(1);
    }

    // Write vertex 0
    fprintf(debugFile, "WHICH %d: x=%f, y=%f, z=%f, w=%f\n", val,
            tri->vecs[0].x, tri->vecs[0].y, tri->vecs[0].z, tri->vecs[0].w);

    fclose(debugFile);
}

void* animation(void* thread_id) {
	// sinf and cosf work with degrees instead of radians
	double angle = 120.0f;
	// double angle_rad = angle * (M_PI / 180.0f);
	// double angle2 = 5 * (3.14159 / 180);
	// angle_rad = angle;

	while (STOP_READING) {
		if (PAUSE_LOOP && debug_on && 0) {
			continue;
		}

		usleep(5000);
		clear_screen();

		pthread_mutex_lock(&mutex);
		move_cursor_NO_REASSGN(1, 1);
		printf("WIDTH: %d; HEIGHT: %d", Term_Conf.cols, Term_Conf.rows);
		move_cursor_NO_REASSGN(1, 2);
		printf("DIST %d", W_DEF);
		fflush(stdout);
		pthread_mutex_unlock(&mutex);

		print_from_cart(0, 0);

		// get rotated idiot
		for (int i = 0; i < CubeMesh->numTris; i++) {
			Triangle tri_updated = CubeMesh->tris[i];
			if (i == 0) {
				move_cursor_NO_REASSGN(0, 6);
				printf("original: %f, %f, %f, %f", tri_updated.vecs[0].x, tri_updated.vecs[0].y, tri_updated.vecs[0].z, tri_updated.vecs[0].w);
			}

			if (i == 0) {
				dump_vertex_to_debug_file(&tri_updated, 0);
			}
			rotate_triangle(&tri_updated, angle, angle * 0.5, angle * 0.33);
			if (i == 0) {
				move_cursor_NO_REASSGN(0, 7);
				printf("rotated: %f, %f, %f, %f", tri_updated.vecs[0].x, tri_updated.vecs[0].y, tri_updated.vecs[0].z, tri_updated.vecs[0].w);
				dump_vertex_to_debug_file(&tri_updated, 1);
			}

			translate_triangle(&tri_updated, 0);
			if (i == 0) {
				move_cursor_NO_REASSGN(0, 8);
				printf("translated: %f, %f, %f, %f", tri_updated.vecs[0].x, tri_updated.vecs[0].y, tri_updated.vecs[0].z, tri_updated.vecs[0].w);
			}

			project_triangle(&tri_updated);
			if (i == 0) {
				move_cursor_NO_REASSGN(0, 9);
				printf("projected: %f, %f, %f, %f", tri_updated.vecs[0].x, tri_updated.vecs[0].y, tri_updated.vecs[0].z, tri_updated.vecs[0].w);
			}

			if (i == 0) {
				move_cursor_NO_REASSGN(0, 12);
				printf("z: %f", tri_updated.vecs[0].z);
			}

			normalise_triangle(&tri_updated);
			if (i == 0) {
				move_cursor_NO_REASSGN(0, 10);
				printf("normalised: %f, %f, %f, %f", tri_updated.vecs[0].x, tri_updated.vecs[0].y, tri_updated.vecs[0].z, tri_updated.vecs[0].w);
			}

			if (i == 0) {
				move_cursor_NO_REASSGN(0, 16);
				printf("angle: %f", angle);
				move_cursor_NO_REASSGN(0, 17);
				printf("angle (radians): %f", angle);

				move_cursor_NO_REASSGN(0, 13);
				printf("w: %f", tri_updated.vecs[0].w);
			}

			draw_triangle(&tri_updated);
		}

		angle += 0.02;

		fflush(stdout);

		if (debug_on) {
			PAUSE_LOOP = 1;
		}
	}

    pthread_exit(NULL);
}

Vec4 create_vec4(double x, double y, double z, double w) {
	Vec4 vec = {x, y, z, w};
	return vec;
}
Triangle create_triangle(Vec4 v0, Vec4 v1, Vec4 v2) {
    Triangle tri = { {v0, v1, v2} };
    return tri;
}

// Allocate sq_mesh on its own on heap mem
// Otherwise after the func ends, it frees the obj and pointer is left hanging
Mesh* get_square(double x, double y, double side, double z) {
	Mesh* sq_mesh = (Mesh *)malloc(sizeof(Mesh));
	if (sq_mesh == NULL) {
		printf("Mem allocation failed!\n");
		return NULL;
	}

	// seperate allocation for triangles
	sq_mesh->tris = (Triangle *)malloc(SQUARE_TRIANGLE_COUNT * sizeof(Triangle));
    if (sq_mesh->tris == NULL) {
        printf("Memory allocation for triangles failed!\n");
        free(sq_mesh);
        return NULL;
    }

	sq_mesh->numTris = SQUARE_TRIANGLE_COUNT;

	sq_mesh->tris[0] = create_triangle(
		create_vec4(x, y, z, W_DEF),
		create_vec4(x + side, y + side, z, W_DEF),
		create_vec4(x, y + side, z, W_DEF)
	);

	sq_mesh->tris[1] = create_triangle(
		create_vec4(x, y, z, W_DEF),
		create_vec4(x + side, y, z, W_DEF),
		create_vec4(x + side, y + side, z, W_DEF)
	);

	return sq_mesh;
}

Mesh* get_cube(double x, double y, double z, double side) {
    // Allocate memory for the cube
    Mesh* cube_mesh = (Mesh *)malloc(sizeof(Mesh));
    if (cube_mesh == NULL) {
        printf("Failed to allocate memory for cube.\n");
        return NULL;
    }

    // Allocate memory for the triangles
    cube_mesh->tris = (Triangle *)malloc(CUBE_TRIANGLE_COUNT * sizeof(Triangle));
    if (cube_mesh->tris == NULL) {
        printf("Failed to allocate memory for cube's triangles.\n");
        free(cube_mesh);  // Free mesh if triangle allocation fails
        return NULL;
    }

    // Define the number of triangles
    cube_mesh->numTris = CUBE_TRIANGLE_COUNT;

    // Define vertices of the cube
    Vec4 vertices[8] = {
        create_vec4(x, y, z, W_DEF),                   // 0: Bottom-front-left
        create_vec4(x + side, y, z, W_DEF),           // 1: Bottom-front-right
        create_vec4(x, y + side, z, W_DEF),           // 2: Top-front-left
        create_vec4(x + side, y + side, z, W_DEF),    // 3: Top-front-right
        create_vec4(x, y, z + side, W_DEF),           // 4: Bottom-back-left
        create_vec4(x + side, y, z + side, W_DEF),    // 5: Bottom-back-right
        create_vec4(x, y + side, z + side, W_DEF),    // 6: Top-back-left
        create_vec4(x + side, y + side, z + side, W_DEF) // 7: Top-back-right
    };

    // Define triangles for each face
    int indices[12][3] = {
        {0, 3, 2}, {0, 1, 3},  // Front face
        {1, 7, 3}, {1, 5, 7},  // Right face
        {5, 6, 7}, {5, 4, 6},  // Back face
        {4, 2, 6}, {4, 0, 2},  // Left face
        {2, 7, 6}, {2, 3, 7},  // Top face
        {4, 1, 0}, {4, 5, 1}   // Bottom face
    };

    for (int i = 0; i < CUBE_TRIANGLE_COUNT; i++) {
        cube_mesh->tris[i] = create_triangle(
            vertices[indices[i][0]],
            vertices[indices[i][1]],
            vertices[indices[i][2]]
        );
    }

    return cube_mesh;
}

void* foo(void* thread_id) {
    pthread_exit(NULL);
}

void init_projection_mat() {
	fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * M_PI);
	ASPECT_RATIO = (double)Term_Conf.rows / (double)Term_Conf.cols;

	// printf("ffovrad: %f\n", ASPECT_RATIO);
	// ASPECT_RATIO = 1601.0 / 2560.0;
	projection_matrix.m[0][0] = ASPECT_RATIO * fFovRad;
	projection_matrix.m[1][1] = fFovRad;
	projection_matrix.m[2][2] = (fFar + fNear) / (fNear - fFar);
	// projection_matrix.m[2][3] = 1.0f;
	projection_matrix.m[2][3] = (2 * fFar * fNear) / (fNear - fFar);
	// projection_matrix.m[3][2] = (-fFar * fNear) / (fFar - fNear);
	projection_matrix.m[3][2] = -1.0f;
	projection_matrix.m[3][3] = 0.0f;
	
}

void debug_funcs() {
	Vec4 v1 = {-10, -10, -10, 1};
	Vec4 v2;
	double angle = 98.0f;
	double angle_rad = angle * (M_PI / 180.0f);
	angle_rad = angle;

	printf("original: x: %f y: %f z: %f w: %f\n", v1.x, v1.y, v1.z, v1.w);
	v2 = rotate_xyz(&v1, angle_rad, angle_rad, angle_rad);
	printf("rotated: x: %f y: %f z: %f w: %f\n", v2.x, v2.y, v2.z, v2.w);
	// translate
	v2.z += 3;
	printf("translated: x: %f y: %f z: %f w: %f\n", v2.x, v2.y, v2.z, v2.w);

	v2 = mat4_mult_vec4_2(&projection_matrix, &v2);
	printf("projected: x: %f y: %f z: %f w: %f\n", v2.x, v2.y, v2.z, v2.w);

	v2.x /= v2.w;
	v2.y /= v2.w;
	v2.z /= v2.w;
	v2.w /= v2.w;

	printf("Final: x: %f y: %f\n", v2.x, v2.y);
	return;
}

int main() {
	int debug_funcs_bool = 1;
	int rows;
	int cols;

	switch (debug_funcs_bool) {
		case 0:
			if (get_terminal_size(&rows, &cols) == -1) {
				die("get_terminal_size");
				return 0;
			}

			Term_Conf.rows = rows;
			Term_Conf.cols = cols;

			printf("Rows %d\n", Term_Conf.rows);
			printf("Cols %d\n", Term_Conf.cols);

			init_projection_mat();

			printf("PROJECTION\n");
			printf("--\n");
			for (int i = 0; i < 4; i++) {
				printf("%f %f %f %f\n", projection_matrix.m[i][0], projection_matrix.m[i][1], projection_matrix.m[i][2], projection_matrix.m[i][3]);
			}
			printf("--\n");

			debug_funcs();
			return 0;

		case 1:
			enable_raw_mode();
			init_window();
	}

	init_projection_mat();

	srand((unsigned int)time(NULL));

	SquareMesh = get_square(10, 10, 200, 1);
	CubeMesh = get_cube(-20, -20, -20, 40);
	for (int i = 0; i < CubeMesh->numTris; i++) {
		printf("[%f, %f, %f, %f] ", CubeMesh->tris[i].vecs[0].x, CubeMesh->tris[i].vecs[0].x, CubeMesh->tris[i].vecs[0].z, CubeMesh->tris[i].vecs[0].w);
		printf("[%f, %f, %f, %f] ", CubeMesh->tris[i].vecs[1].x, CubeMesh->tris[i].vecs[1].x, CubeMesh->tris[i].vecs[1].z, CubeMesh->tris[i].vecs[1].w);
		printf("[%f, %f, %f, %f]\n", CubeMesh->tris[i].vecs[2].x, CubeMesh->tris[i].vecs[2].x, CubeMesh->tris[i].vecs[2].z, CubeMesh->tris[i].vecs[2].w);
	}

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

	free(SquareMesh->tris);
	free(SquareMesh);

	free(CubeMesh->tris);
	free(CubeMesh);
}

// void animation_old() {
// 	// Furthest objects first
// 	// drawSquare(-10, -5, 15, 5, 10, 33);
// 	// drawSquare(10, -5, 15, 5, 10, 33);
// 	//
// 	// drawSquare(-10, -5, 10, 5, 10, 32);
// 	// drawSquare(10, -5, 10, 5, 10, 32);
// 	//
// 	// drawSquare(-10, -5, 5, 2, 10, 31);
// 	// drawSquare(10, -5, 5, 2, 10, 31);
// }
