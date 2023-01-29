#include <vector>

#include "opengl.h"
#include "assets.h"

static GLenum err;

void GLError(
    GLenum err, 
    int line,
    const char* file,
    const char* funcName
) {
    LOGI("[FAIL GL] %s, %s, %d", file, funcName, line);

    switch(err) {

        case GL_INVALID_ENUM:
            LOGE("GLenum argument out of range");
            break;

        case GL_INVALID_VALUE:
            LOGE("numeric argument out of range");
            break;

        case GL_INVALID_OPERATION:
            LOGE("operation illegal in current state");
            break;

        case GL_INVALID_FRAMEBUFFER_OPERATION:
            LOGE("framebuffer object is not complete");
            break;

        case GL_OUT_OF_MEMORY:
            LOGE("not enough memory left to execute command");
            break;

        default:
            LOGE("unlisted error, %d", err);
            break;
    }
}

GLuint create_shader(
    const char* path, 
    GLenum type
) {
    if (!asset::manager->open(path)) return 0;
    GLuint shader       = glCreateShader(type);
    const char* cstr    = asset::manager->read();

    glShaderSource(shader, 1, &cstr, NULL);
    glCompileShader(shader);

    GLint is_compiled=0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);

    if (is_compiled==GL_FALSE) {
        
        GLint length=0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<GLchar> log(length);
        glGetShaderInfoLog(shader, length, &length, log.data());
        LOGI("Could not compile shader %s - %s", cstr, log.data());        
        glDeleteShader(shader);
        shader=0;
    }
    asset::manager->close();
    return shader;
}

GLuint create_programm(
    GLuint vertex_shader, 
    GLuint fragment_shader
) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vertex_shader);
    glAttachShader(prog, fragment_shader);
    glLinkProgram(prog);
    GLint is_linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &is_linked);

    if (is_linked == GL_FALSE) {

        LOGI("Could not link program");
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

GLuint ext_texture() {

    GLuint result=0;
    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, result);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return result;
}

GLuint d2_texture(
    GLsizei width,
    GLsizei height,
    const uint8_t* data
) {
    GLuint result=0;
    
    glGenTextures(1, &result);
    CALLGL(glBindTexture(GL_TEXTURE_2D, result))
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CALLGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
        width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data))
    return result;
}

_Program::_Program() {

    glGenVertexArrays(1, &VAO);
    glGenBuffers(ARRAY_COUNT, array);

    LOGI("Create program")
}

_Program::~_Program() {

    LOGI("Destroy program")
    if (id==0) return;

    glDeleteTextures(1, &texture);
    glDetachShader(id, vertex_shader);
    glDetachShader(id, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    glDeleteProgram(id); 

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(ARRAY_COUNT, array);
}