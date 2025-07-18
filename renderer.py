import curses
import math
import time

# Cube vertices (homogeneous coordinates: [x, y, z, 1])
vertices = [
    [-1, -1, -1, 1],
    [ 1, -1, -1, 1],
    [ 1,  1, -1, 1],
    [-1,  1, -1, 1],
    [-1, -1,  1, 1],
    [ 1, -1,  1, 1],
    [ 1,  1,  1, 1],
    [-1,  1,  1, 1],
]

# Edges to connect vertices
edges = [
    (0,1), (1,2), (2,3), (3,0),
    (4,5), (5,6), (6,7), (7,4),
    (0,4), (1,5), (2,6), (3,7)
]

def make_projection_matrix(fov, aspect, near, far):
    f = 1 / math.tan(fov / 2)
    return [
        [f/aspect, 0, 0, 0],
        [0, f, 0, 0],
        [0, 0, (far+near)/(near-far), (2*far*near)/(near-far)],
        [0, 0, -1, 0]
    ]

def multiply_matrix_vector(mat, vec):
    result = [0, 0, 0, 0]
    for i in range(4):
        result[i] = sum(mat[i][j] * vec[j] for j in range(4))

    with open("debug_log.txt", "a") as f:
        f.write(f"original: ")
        f.write(f"{vec[3]}\n")
        f.write(f"updated: ")
        f.write(f"{result[3]}\n\n")
    return result

def rotate_xyz(v, ax, ay, az):
    x, y, z, w = v

    # Rotate X
    cosx, sinx = math.cos(ax), math.sin(ax)
    y, z = y * cosx - z * sinx, y * sinx + z * cosx

    # Rotate Y
    cosy, siny = math.cos(ay), math.sin(ay)
    x, z = x * cosy + z * siny, -x * siny + z * cosy

    # Rotate Z
    cosz, sinz = math.cos(az), math.sin(az)
    x, y = x * cosz - y * sinz, x * sinz + y * cosz

    return [x, y, z, w]

def main(stdscr):
    curses.curs_set(0)
    stdscr.nodelay(1)
    width = curses.COLS
    height = curses.LINES
    width = 132
    height = 40

    fov = math.radians(90)
    aspect = width / height
    near = 0.1
    far = 1000
    projection = make_projection_matrix(fov, aspect, near, far)


    with open("debug_log.txt", "a") as f:
        f.write(f"Projection Mat\n")
        for v in projection:
            f.write(f"{v}\n")

    angle = 183.7

    while True:
        stdscr.clear()
        transformed = []
        old_ws = []

        stdscr.addstr(0, 1, f"{angle}")

        for vertex in vertices:
            rotated = rotate_xyz(vertex, angle, angle * 0.5, angle * 0.33)

            # Move cube forward in Z
            translated = [rotated[0], rotated[1], rotated[2] + 3, 1]

            # Apply projection matrix
            p1 = multiply_matrix_vector(projection, translated)

            # with open("debug_log.txt", "a") as f:
            #     f.write(f"1 Projected: {projected}\n\n")

            # Perspective divide
            if p1[3] != 0:
                projected = [c / p1[3] for c in p1]

            with open("debug_log.txt", "a") as f:
                f.write(f"original: {vertex}\n")
                f.write(f"rotated: {rotated}\n")
                f.write(f"translated: {translated}\n")
                f.write(f"projected: {p1}\n")
                f.write(f"divide: {projected}\n")
                # f.write(f"z: {projected[2]}, w: {projected[3]}\n")

            # with open("debug_log.txt", "a") as f:
            #     f.write(f"2 Projected: {projected}\n\n")

            # Convert to screen coordinates
            x = int((projected[0] + 1) * width / 2)
            y = int((1 - projected[1]) * height / 2)
            transformed.append((x, y))
            old_ws.append(projected[3])

            try:
                stdscr.addch(y, x, '█')
            except curses.error:
                pass

        with open("debug_log.txt", "a") as f:
            f.write(f"transformed: {old_ws}\n\n")

        for a, b in edges:
            draw_line(stdscr, *transformed[a], *transformed[b])

        stdscr.refresh()
        time.sleep(0.05)
        angle += 0.03

        if stdscr.getch() != -1:
            break

def draw_line(stdscr, x1, y1, x2, y2):
    # Bresenham’s algorithm
    dx = abs(x2 - x1)
    dy = -abs(y2 - y1)
    sx = 1 if x1 < x2 else -1
    sy = 1 if y1 < y2 else -1
    err = dx + dy
    while True:
        try:
            stdscr.addch(y1, x1, '*')
        except curses.error:
            pass
        if x1 == x2 and y1 == y2:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x1 += sx
        if e2 <= dx:
            err += dx
            y1 += sy

if __name__ == "__main__":
    curses.wrapper(main)

if __name__ == "__main__":
    # width = curses.COLS
    # height = curses.LINES
    width = 132
    height = 40

    fov = math.radians(90)
    aspect = width / height
    near = 0.1
    far = 1000
    projection = make_projection_matrix(fov, aspect, near, far)
    # v1 = [-2.1159805685824358, -0.8014171113687694, -0.08949196252577796, 1]
    # v2 = multiply_matrix_vector(projection, v1)
    # print(v2)
    # exit()
    #
    # p2 = [[0.30167597765363136, 0, 0, 0],
    #       [0, 1.0000000000000002, 0, 0],
    #       [0, 0, -1.0002000200020003, -0.20002000200020004],
    #       [0, 0, -1, 0]]
    #
    #
    # v1 = [-1, -1, -1, -1]
    # v3 = [1.4588086729343932, 0.701155910634708, 3.6166503423767864, 1]
    # v2 = multiply_matrix_vector(p2, v3)
    # print(v2)

    # original: -2.000000, -2.000000, 5.000000, 1.000000                                                      **  ******
    # rotated: 4.061447, -0.521078, 4.029035, 1.000000                                                      **  *******
    # translated: 4.061447, -0.521078, 4.029035, 1.000000                                                 **  *******
    # projected: 1.249676, -0.521078, -4.229860, -4.029035                                              **  *******
    # normalised: -0.310168, 0.129331, 1.049845, 1.000000                                             **   ******
    #                                                                                           **   ******
    # z: -4.229860                                                                                **   ******
    # w: 1.000000                                                                               **   ******
    #                                                                                     **   ******
    #                                                                                   **   ******
    # angle: 122.400009                                                                   **   ******
    # angle (radians): 2.136283

    # v1 = [-2, -2, 5, 1]
    # angle = 122.400009
    # # angle = 1.005309
    # rotated = rotate_xyz(v1, angle, angle, angle)
    # print(rotated)

    print("PROJECTION")
    print("--")
    for p in range(len(projection)):
        print(f"{projection[p][0]} {projection[p][1]} {projection[p][2]} {projection[p][3]}")
    print("--")

    v1 = [-10, -10, -10, 1]
    angle = 98
    rotated = rotate_xyz(v1, angle, angle, angle)

    # Move cube forward in Z
    translated = [rotated[0], rotated[1], rotated[2] + 3, 1]

    # Apply projection matrix
    p1 = multiply_matrix_vector(projection, translated)

    # with open("debug_log.txt", "a") as f:
    #     f.write(f"1 Projected: {projected}\n\n")

    # # Perspective divide
    if p1[3] != 0:
        projected = [c / p1[3] for c in p1]
    #
    # # Convert to screen coordinates
    # x = int((projected[0] + 1) * width / 2)
    # y = int((1 - projected[1]) * height / 2)

    print(f"original x: {v1[0]} y: {v1[1]} z: {v1[2]} w: {v1[3]}")
    print(f"rotated x: {rotated[0]} y: {rotated[1]} z: {rotated[2]} w: {rotated[3]}")
    print(f"translated x: {translated[0]} y: {translated[1]} z: {translated[2]} w: {translated[3]}")
    print(f"projected x: {p1[0]} y: {p1[1]} z: {p1[2]} w: {p1[3]}")
    print(f"Final x: {projected[0]}, y: {projected[1]} z: {projected[2]} w: {projected[3]}")

    exit()
    # curses.wrapper(main)

