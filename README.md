# Fractal4D
Fractal4D is a simple cross-platform GLFW/OpenGL app to run a function on the GPU for every pixel.

# Controls
- Comma: Lower resolution
- Dot/period: Increase resolution
- Mouse: Look around
- WASD: Fly in camera direction
- Space: Fly up
- Shift: Fly down

# Usage
Edit the `getPixel(in vec2 pixel_coords)` function inside /res/raymarcher.comp with the GLSL code you'd like to run on the GPU.
The shader will be run as a compute shader, which requires at least a GPU supporting OpenGL 4.3.

# Building
Make sure you have the GLFW library installed in your system! I used the `glfw-x11` package from the AUR.

To do an out-of-source build, run the following commands:
1. `mkdir build`
2. `cd build`
3. `cmake ..`
4. `make`

The executable Fractal4D will be output. Make sure the res folder is in the working directory!
