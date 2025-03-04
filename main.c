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

// homogeneous 4D vector
typedef struct {
	float x, y, z, w;
} Vec4 ;

// 4x4 matrix
typedef struct {
	float m[4][4];
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

int STOP_READING = 1;
pthread_t threads[NUM_THREADS];
pthread_mutex_t mutex;

int D_DIST = 1;
int W_DEF = 1;
float ASPECT_RATIO;
double FOV_ANGLE = 90;
double FOV;
double Z_NORM;
float fNear = 0.1f;
float fFar = 1000.0f;
float fFov = 45.0f;
float fFovRad;
Mat4 projection_matrix = { 0 };

void DrawLine(int x1, int y1, int x2, int y2) {
	// Calculate the differences
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    int sx = (x1 < x2) ? 1 : -1;  // step in x direction
    int sy = (y1 < y2) ? 1 : -1;  // step in y direction

    int err = dx - dy;  // initial error term

    // Use a for loop to replace the while loop
    // We iterate through x or y for the total number of steps
    int maxIter = (dx > dy) ? dx : dy;  // The number of steps is the larger of dx or dy

    for (int i = 0; i <= maxIter; i++) {
        // Plot the point (x1, y1)
        // printf("Plotting pixel at (%d, %d)\n", x1, y1);

		int t_x = x1 + Term_Conf.cols / 2;
		int t_y = (-1 * y1) + Term_Conf.rows / 2;
		move_cursor_NO_REASSGN(t_x, t_y);
		printf("#");

        // Calculate error
        int e2 = err * 2;

        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }

        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }}

void draw_triangle(Triangle* tri) {
	Vec4 new_tri_1 = mat4_mult_vec4(&projection_matrix, &tri->vecs[0]);
	Vec4 new_tri_2 = mat4_mult_vec4(&projection_matrix, &tri->vecs[1]);
	Vec4 new_tri_3 = mat4_mult_vec4(&projection_matrix, &tri->vecs[2]);

	move_cursor_NO_REASSGN(1, 4);
	printf("z: %f\nw: %f\n", new_tri_1.z, new_tri_1.w);

	// NOTE: Updating the xyz wrt the w (or z) breaks it. idk whats wrong
	// Heads up future self :D

	// if (new_tri_1.w != 0.0f) {
	// 	new_tri_1.x /= new_tri_1.w;
	// 	new_tri_1.y /= new_tri_1.w;
	// 	new_tri_1.z /= new_tri_1.w;
	// }
	//
	// if (new_tri_2.w != 0.0f) {
	// 	new_tri_2.x /= new_tri_2.w;
	// 	new_tri_2.y /= new_tri_2.w;
	// 	new_tri_2.z /= new_tri_2.w;
	// }
	//
	// if (new_tri_3.w != 0.0f) {
	// 	new_tri_3.x /= new_tri_3.w;
	// 	new_tri_3.y /= new_tri_3.w;
	// 	new_tri_3.z /= new_tri_3.w;
	// }

	DrawLine(new_tri_1.x, new_tri_1.y, new_tri_2.x, new_tri_2.y);
	DrawLine(new_tri_2.x, new_tri_2.y, new_tri_3.x, new_tri_3.y);
	DrawLine(new_tri_3.x, new_tri_3.y, new_tri_1.x, new_tri_1.y);
	// DrawLine(tri->vecs[0].x, tri->vecs[0].y, tri->vecs[1].x, tri->vecs[1].y);
	// DrawLine(tri->vecs[1].x, tri->vecs[1].y, tri->vecs[2].x, tri->vecs[2].y);
	// DrawLine(tri->vecs[2].x, tri->vecs[2].y, tri->vecs[0].x, tri->vecs[0].y);
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

void rotate_x(float* x, float* y, float* z, float angle) {
	float new_y = *y * cos(angle) - *z * sin(angle);
	float new_z = *y * sin(angle) + *z * cos(angle);

	*y = new_y;
	*z = new_z;
}

void rotate_y(float* x, float* y, float* z, float angle) {
    float new_x = *x * cos(angle) + *z * sin(angle);
    float new_z = -(*x) * sin(angle) + *z * cos(angle);

    *x = new_x;
    *z = new_z;
}

void rotate_z(float* x, float* y, float* z, float angle) {
    float new_x = *x * cos(angle) - *y * sin(angle);
    float new_y = *x * sin(angle) + *y * cos(angle);

    *x = new_x;
    *y = new_y;
}

void* animation(void* thread_id) {
	double degrees = 1.0;
    double radians = degrees * M_PI / 180.0;
	double x_s = 0;
	double sq_side = 10;
	double z_s = 5.0;

	Vec4 init_p = {20, 20, 10, 1};
	Vec4 init_p_trans = mat4_mult_vec4(&projection_matrix, &init_p);

	Point3D sq1[4] = {
		{x_s, x_s, z_s},
		{x_s + sq_side, x_s, z_s},
		{x_s, x_s + sq_side, z_s},
		{x_s + sq_side, x_s + sq_side, z_s},
	};

	float angle = 10 * (3.14159 / 180);

	while (STOP_READING) {
		usleep(100000);
		clear_screen();

		pthread_mutex_lock(&mutex);
		move_cursor_NO_REASSGN(1, 1);
		printf("WIDTH: %d; HEIGHT: %d", Term_Conf.cols, Term_Conf.rows);
		move_cursor_NO_REASSGN(1, 2);
		printf("DIST %d", W_DEF);
		fflush(stdout);
		pthread_mutex_unlock(&mutex);

		for (int i = 0; i < CubeMesh->numTris; i++) {
			draw_triangle(&CubeMesh->tris[i]);
		}

		// get rotated idiot
		for (int i = 0; i < CubeMesh->numTris; i++) {
			for (int j = 0; j < 3; j++) {
				rotate_x(&CubeMesh->tris[i].vecs[j].x, 
						&CubeMesh->tris[i].vecs[j].y, 
						&CubeMesh->tris[i].vecs[j].z, angle);

				rotate_y(&CubeMesh->tris[i].vecs[j].x, 
						&CubeMesh->tris[i].vecs[j].y, 
						&CubeMesh->tris[i].vecs[j].z, angle);

				// rotate_z(&CubeMesh->tris[i].vecs[j].x, 
				// 		&CubeMesh->tris[i].vecs[j].y, 
				// 		&CubeMesh->tris[i].vecs[j].z, angle);

			}
		}

		fflush(stdout);
	}

    pthread_exit(NULL);
}

Vec4 create_vec4(float x, float y, float z, float w) {
	Vec4 vec = {x, y, z, w};
	return vec;
}

Triangle create_triangle(Vec4 v0, Vec4 v1, Vec4 v2) {
    Triangle tri = { {v0, v1, v2} };
    return tri;
}

// Allocate sq_mesh on its own on heap mem
// Otherwise after the func ends, it frees the obj and pointer is left hanging
Mesh* get_square(float x, float y, float side, float z) {
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

Mesh* get_cube(float x, float y, float side, float z) {
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

int main() {
	// fFovRad = 1.0f / tanf((fFov * 3.14159f / 180.0f) * 0.5f);
	fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * 3.14159f);

	enable_raw_mode();
	init_window();

	ASPECT_RATIO = (float)Term_Conf.rows / (float)Term_Conf.cols;
	ASPECT_RATIO = 1601.0 / 2560.0;
	projection_matrix.m[0][0] = ASPECT_RATIO * fFovRad;
	projection_matrix.m[1][1] = fFovRad;
	projection_matrix.m[2][2] = fFar / (fFar - fNear);
	projection_matrix.m[2][3] = 1.0f;
	projection_matrix.m[3][2] = (-fFar * fNear) / (fFar - fNear);
	projection_matrix.m[3][3] = 0.0f;

	srand((unsigned int)time(NULL));

	SquareMesh = get_square(10, 10, 200, 1);
	CubeMesh = get_cube(0, 0, 10, -20);

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
