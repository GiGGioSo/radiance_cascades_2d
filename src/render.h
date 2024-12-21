#ifndef _RC_RENDER_H_
#define _RC_RENDER_H_

#include <glad/glad.h>

typedef GLuint shader_program;
typedef GLuint texture;

shader_program
shader_create_program(const char *vertex_shader_path, const char *fragment_shader_path);

void
shader_delete_program(shader_program program);

void
shader_set_int32(shader_program s, const char* name, int value);

void
shader_set_float(shader_program s, const char* name, float value);

void
shader_set_vec3f(shader_program s, const char* name, float x, float y, float z);

#ifdef RADIANCE_CASCADES_RENDER_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>

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

#endif

#endif // _RC_RENDER_H_
