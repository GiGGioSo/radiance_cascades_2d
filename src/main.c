
#include <stdio.h>

#include <glad/glad.h>
#include <glfw3.h>

typedef unsigned char ubyte;

typedef struct pixel {
    ubyte r, g, b, a;
} pixel;

void init_map_pixels(pixel *m, int w, int h);
GLuint generate_map_texture(pixel *m, int w, int h);

#define WIDTH 800
#define HEIGHT 600

#define VOID (pixel){ .r = 0, .g = 0, .b = 0, .a = 0 }
#define RED_LIGHT (pixel){ .r = 255, .g = 0, .b = 0, .a = 255 }
#define GREEN_LIGHT (pixel){ .r = 0, .g = 255, .b = 0, .a = 255 }
#define BLUE_LIGHT (pixel){ .r = 0, .g = 0, .b = 255, .a = 255 }
#define OBSTACLE (pixel){ .r = 255, .g = 255, .b = 255, .a = 255 }

pixel map[HEIGHT * WIDTH];

GLFWwindow *glfw_win;
float delta_time;
unsigned long fps_counter;

int main(void) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glfw_win = glfwCreateWindow(WIDTH, HEIGHT, "Radiance Cascades", NULL, NULL);

    if (glfw_win == NULL) {
        fprintf(stderr, "[ERROR] Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(glfw_win);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        fprintf(stderr, "[ERROR] Failed to initialize GLAD\n");
        return 1;
    }

    init_map_pixels(map, WIDTH, HEIGHT);
    GLuint map_texture = generate_map_texture(map, WIDTH, HEIGHT);

    float this_frame = 0.f;
    float last_frame = 0.f;
    float time_from_last_fps_update = 0.f;
    while (!glfwWindowShouldClose(glfw_win)) {
        // # fps stuff
        this_frame = (float)glfwGetTime();
        delta_time = this_frame - last_frame;
        last_frame = this_frame;
        fps_counter++;
        time_from_last_fps_update += delta_time;
        if (time_from_last_fps_update > 1.f) {
            printf("FPS: %lu\n", fps_counter);
            fps_counter = 0;
            time_from_last_fps_update -= 1.f;
        }

        // # keyboard input stuff
        if (glfwGetKey(glfw_win, GLFW_KEY_Q) == GLFW_PRESS) {
            glfwSetWindowShouldClose(glfw_win, GLFW_TRUE);
        }

        // # finishing touch of the current frame
        glfwSwapBuffers(glfw_win);
        glfwPollEvents();
    }
}

GLuint generate_map_texture(pixel *m, int w, int h) {
    GLuint tex;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            w,
            h,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE, 
            m);
    return tex;
}

void init_map_pixels(pixel *m, int w, int h) {
    // fill the void
    for(int index = 0; index < w * h; ++index) {
        m[index] = VOID;
    }

    // light source: middle left
    int light_w = 20;
    int light_h = h / 3;
    int light_x = (w / 4) - (light_w / 2);
    int light_y = (h / 2) - (light_h / 2);
    for(int y = light_y; y < light_y + light_h; ++y) {
        for(int x = light_x; x < light_x + light_w; ++x) {
            int index = y * w + x;
            m[index] = RED_LIGHT;
        }
    }

    // obstacle: center down
    int obstacle_w = 30;
    int obstacle_h = h / 3;
    int obstacle_x = (w / 2) - (obstacle_w / 2);
    int obstacle_y = (h / 2);
    for(int y = obstacle_y; y < obstacle_y + obstacle_h; ++y) {
        for(int x = obstacle_x; x < obstacle_x + obstacle_w; ++x) {
            int index = y * w + x;
            m[index] = OBSTACLE;
        }
    }
}

void generate_cascade(int cascade_index) {
}
