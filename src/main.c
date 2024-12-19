
#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>
#include <glfw3.h>

typedef unsigned char ubyte;

typedef struct pixel {
    ubyte r, g, b, a;
} pixel;

typedef GLuint shader_program;

void init_map_pixels(pixel *m, int w, int h);
GLuint generate_map_texture(pixel *m, int w, int h);
void setup_map_renderer(GLuint *vao, GLuint *vbo, GLuint *ebo);
shader_program shader_create_program(const char *vertex_shader_path, const char *fragment_shader_path);
void shader_delete_program(shader_program program);
void shader_set_int32(shader_program s, const char* name, int value);
void shader_set_float(shader_program s, const char* name, float value);
void shader_set_vec3f(shader_program s, const char* name, float x, float y, float z);

#define WIDTH 800
#define HEIGHT 600

#define VOID (pixel){ .r = 255, .g = 255, .b = 255, .a = 255 }
#define OBSTACLE (pixel){ .r = 0, .g = 0, .b = 0, .a = 0 }
#define RED_LIGHT (pixel){ .r = 255, .g = 0, .b = 0, .a = 255 }
#define GREEN_LIGHT (pixel){ .r = 0, .g = 255, .b = 0, .a = 255 }
#define BLUE_LIGHT (pixel){ .r = 0, .g = 0, .b = 255, .a = 255 }


int main(void) {
    // variables
    pixel map[HEIGHT * WIDTH];
    GLFWwindow *glfw_win;
    float delta_time;
    unsigned long fps_counter;
    GLuint vao, vbo, ebo;
    shader_program map_shader;

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
    setup_map_renderer(&vao, &vbo, &ebo);
    map_shader =
        shader_create_program("res/shaders/map.vs", "res/shaders/map.fs");
    glUseProgram(map_shader);
    shader_set_int32(map_shader, "map_texture", 0);

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

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // # render map texture
        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, map_texture);
        // render container
        glUseProgram(map_shader);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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

void setup_map_renderer(GLuint *vao, GLuint *vbo, GLuint *ebo) {
    float vertices[] = {
        -1.f, -1.f, 0.f, 1.f, // up left
        -1.f,  1.f, 0.f, 0.f, // down left
         1.f,  1.f, 1.f, 0.f, // down right
         1.f, -1.f, 1.f, 1.f  // up right
    };
    int indices[] = {
        0, 1, 3,
        1, 2, 3
    };
    glGenVertexArrays(1, vao);
    glGenBuffers(1, vbo);
    glGenBuffers(1, ebo);

    glBindVertexArray(*vao);

    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(vertices),
            vertices,
            GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
    glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            sizeof(indices),
            indices,
            GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(
            0, 2, GL_FLOAT, GL_FALSE,
            4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord
    glVertexAttribPointer(
            1, 2, GL_FLOAT, GL_FALSE,
            4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void render_map(GLuint vao, pixel *m, int w, int h) {
}

void generate_cascade(int cascade_index) {
}

shader_program shader_create_program(
        const char *vertex_shader_path,
        const char *fragment_shader_path) {

    FILE *vertex_file = NULL;
    char *vertex_code = NULL;

    FILE *fragment_file = NULL;
    char *fragment_code = NULL;

    shader_program program = 0;
    GLuint vertex = 0;
    GLuint fragment = 0;

    // Reading the vertex shader code
    vertex_file = fopen(vertex_shader_path, "rb");
    if (vertex_file == NULL) {
        fprintf(stderr, "could not open vertex_file: %s\n", vertex_shader_path);
        goto defer_dealloc;
    }

    int vs_seek_end_result = fseek(vertex_file, 0, SEEK_END);
    if (vs_seek_end_result) {
        fprintf(stderr, "could not seek_end vertex_file: %s\n", vertex_shader_path);
        goto defer_dealloc;
    }

    unsigned long vertex_file_size = ftell(vertex_file);
    int vs_seek_set_result = fseek(vertex_file, 0, SEEK_SET);
    if (vs_seek_set_result) {
        fprintf(stderr, "could not seek_set vertex_file: %s\n", vertex_shader_path);
        goto defer_dealloc;
    }

    vertex_code = malloc(vertex_file_size + 1);
    if (vertex_code == NULL) {
        fprintf(stderr, "could not malloc memory for vertex_code\n");
        goto defer_dealloc;
    }

    int vs_read_result = fread(vertex_code, vertex_file_size, 1, vertex_file);
    if (vs_read_result != 1) {
        fprintf(stderr, "could not fread vertex_file: %s\n", vertex_shader_path);
        goto defer_dealloc;
    }

    fclose(vertex_file);
    vertex_file = NULL,
    vertex_code[vertex_file_size] = '\0';

    // Reading the fragment shader code
    fragment_file = fopen(fragment_shader_path, "rb");
    if (fragment_file == NULL) {
        fprintf(stderr, "could not open fragment_file: %s\n", fragment_shader_path);
        goto defer_dealloc;
    }

    int fs_seek_end_result = fseek(fragment_file, 0, SEEK_END);
    if (vs_seek_end_result) {
        fprintf(stderr, "could not seek_end fragment_file: %s\n", fragment_shader_path);
        goto defer_dealloc;
    }

    unsigned long fragment_file_size = ftell(fragment_file);
    int fs_seek_set_result = fseek(fragment_file, 0, SEEK_SET);
    if (fs_seek_set_result) {
        fprintf(stderr, "could not seek_set fragment_file: %s\n", fragment_shader_path);
        goto defer_dealloc;
    }

    fragment_code = malloc(fragment_file_size + 1);
    if (fragment_code == NULL) {
        fprintf(stderr, "could not malloc memory for fragment_code\n");
        goto defer_dealloc;
    }

    int fs_read_result = fread(fragment_code, fragment_file_size, 1, fragment_file);
    if (fs_read_result != 1) {
        fprintf(stderr, "could not fread fragment_file: %s\n", fragment_shader_path);
        goto defer_dealloc;
    }

    fclose(fragment_file);
    fragment_file = NULL;
    fragment_code[fragment_file_size] = '\0';

    // creating the program
    int success;
    char err_log[512];

    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, (const char **) &vertex_code, NULL);
    glCompileShader(vertex);
    // print compile errors if any
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertex, 512, NULL, (GLchar *) err_log);
        fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED (%s)\n%s\n",
                vertex_shader_path, err_log);
        goto defer_dealloc;
    }

    // fragment shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, (const char **) &fragment_code, NULL);
    glCompileShader(fragment);
    //print compile errors if any
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragment, sizeof(err_log), NULL, (GLchar *) err_log);
        fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED (%s)\n%s\n",
                fragment_shader_path, err_log);
        goto defer_dealloc;
    }

    // program
    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    // print linking errors if any
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(program, sizeof(err_log), NULL, (GLchar *) err_log);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED %s\n", err_log);
        program = 0;
        goto defer_dealloc;
    }

    defer_dealloc:
    {
        if (vertex_file) fclose(vertex_file);
        if (vertex_code) free(vertex_code);
        if (fragment_file) fclose(fragment_file);
        if (fragment_code) free(fragment_code);

        if (vertex) glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
        return program;
    }
}

void shader_delete_program(shader_program program) {
    glDeleteProgram(program);
}

void shader_set_int32(
        shader_program s,
        const char* name,
        int value) {
    glUseProgram(s);
    glUniform1i(glGetUniformLocation(s, name), value);
}

void shader_set_float(
        shader_program s,
        const char* name,
        float value) {
    glUseProgram(s);
    glUniform1f(glGetUniformLocation(s, name), value);
}

void shader_set_vec3f(
        shader_program s,
        const char* name,
        float x, float y, float z) {
    glUseProgram(s);
    glUniform3f(glGetUniformLocation(s, name), x, y, z);
}
