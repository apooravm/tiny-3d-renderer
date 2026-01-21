### Making sense of worldUp matrix

Few things...
- Point in space P = (1, 1, 0)
- Target point T = (4, 1, 4)
- Looking at target from point (vector) V = T - P = (3, 0, 4)
- If we dont care about the distance - normalize -> normalize(V) = |v| = √(3² + 0² + 4²) = √4 = 2 = (1/2, 2/2, 2/2)
- Thus before starting, worldUp vector is perpendicular to the camera = (0, 1, 0), since y is up
- cross product of 2 vectors is another vector perpendicular to both
- dot product of 2 perpendicular vectors is 0 (just to check)

Thus in lookAt calculation, when deriving forward, right and up

Say
- C = (0, 0, 0)
- T = (1, 2, 2)
- WorldUp = (0, 1, 0)

Then
- forward = T - C = normalize(1, 2, 2) = |v| = √(1² + 2² + 2²) = forward = (1/3, 2/3, 2/3)
- right = normalize(cross(forward, worldUp)) we use worldUp since we dont have an up already

### Horizontal plane is perpendicular to y, so both x and z
In the XZ plane:

Y is vertical (up/down)

X and Z define all possible directions on the horizontal plane

Then, yaw rotation naturally spreads the horizontal length between X and Z:

x = cos(pitch) * sin(yaw)
z = cos(pitch) * cos(yaw)

Yaw = 0 → forward along +Z

Yaw = 90° → right along +X

Yaw = 45° → diagonal forward-right (X+Z)

If you only used X, this flexibility disappears.
