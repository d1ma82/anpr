#pragma once
#ifdef ANDROID
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#else
#include <glad/glad.h>
#endif

#ifdef NDEBUG
#define CALLGL(proc);

#else
#define CALLGL(proc) proc; \
        err = glGetError();\
        if (err != GL_NO_ERROR) GLError(err, __LINE__, __FILE__, #proc);
#endif

enum { VERTEX, ELEMENT, ARRAY_COUNT };          // Array types

typedef struct _Program {

    GLuint texture{0};
    GLuint id{0}, vertex_shader{0}, fragment_shader{0};
    GLuint VAO{0};
    GLuint array[ARRAY_COUNT];  // VBO, EBO

    _Program();
   ~_Program();
} Program;

void GLError(GLenum, int, const char*, const char*);
GLuint create_shader(const char* path, GLenum type);
GLuint create_programm(GLuint vertex_shader, GLuint fragment_shader);
GLuint ext_texture();
GLuint d2_texture(GLsizei width, GLsizei height, const uint8_t* data);