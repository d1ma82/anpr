#pragma once

#include <vector>
#include "opengl.h"
#include "filter.h"
#include "log.h"

static GLfloat flip_v[16] {
        1,  0, 0, 0,
        0, -1, 0, 0,
        0,  0, 1, 0,
        0,  1, 0, 1  
};

static GLfloat ident[16] {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1  
};


class Render {
    dims          viewport;
    Program       input, output;
    GLenum        err;
    bool          need_shot {false};
    bool          finish    {true};         //  |
    bool          stop      {false};        //  |   Flags to sync GLThread
    std::string   shot_path;
    uint8_t*      shot_pixels   {nullptr};
    std::vector<filter::Filter*> filters;

    const GLfloat vertex[20] {
        // positions              texture coords
         1.0f,  1.0f, 0.0f,        1.0f, 1.0f,     // top right
         1.0f, -1.0f, 0.0f,        1.0f, 0.0f,     // bottom right
        -1.0f, -1.0f, 0.0f,        0.0f, 0.0f,     // bottom left
        -1.0f,  1.0f, 0.0f,        0.0f, 1.0f      // top left             
    };

    const GLuint indices[6] {
        0, 1, 3,    // first triangle
        1, 2, 3     // second
    };

    enum { VERTEX_ATTRIB, TEXTURE_ATTRIB };

    void init_io(
        Program& io, 
        int sensor_orientation, 
        int mode                    // 0- input; 1-output
    ) {
        if (mode==0) {

            LOGI("Configure input")
            io.fragment_shader =   create_shader("shader/external.frag", GL_FRAGMENT_SHADER);
            io.texture         =   ext_texture();  
        } else {
            LOGI("Configure output")
            io.fragment_shader =   create_shader("shader/sampler2D.frag", GL_FRAGMENT_SHADER);
            io.texture         =   0;
        }
        io.vertex_shader    =   create_shader("shader/screen.vert", GL_VERTEX_SHADER);
        io.id               =   create_programm(io.vertex_shader, io.fragment_shader);
        
        glBindVertexArray(io.VAO);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, io.array[ELEMENT]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, io.array[VERTEX]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(VERTEX_ATTRIB, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(TEXTURE_ATTRIB, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);

        glUseProgram(io.id);
        GLint transform = glGetUniformLocation(io.id, "transform");
        switch (sensor_orientation) {
            case 90: glUniformMatrix4fv(transform, 1, GL_FALSE, flip_v); break;
            default: glUniformMatrix4fv(transform, 1, GL_FALSE, ident);
        }
        glUniform1i(glGetUniformLocation(io.id, "camera"), 0);
        glBindVertexArray(0);
    }

    void toogle_finish() { 
        if (!stop) finish=!finish;
    }

public:
    Render(
        dims viewport_input, 
        int sensor_orientation):
            viewport{viewport_input} {
        
        LOGI("GL_VERSION: %s, GL_SHADER_LANG_VERSION: %s", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION))

        glViewport(0, 0, viewport.first, viewport.second);

        shot_pixels =  new uint8_t[viewport.first*viewport.second*3];
        for (int i=0; i<2; i++) init_io(i==0? input: output, i==0? sensor_orientation: 0, i);
    }

    void render(int orientation) { 

        toogle_finish();
        if (finish) return;
        
        for (auto filter: filters) filter->apply(orientation);

        /*if (need_shot) {
            
            filter::Filter* shot_filter = 
                new filter::Shot(*filters.rbegin(), shot_path.c_str(), viewport); 

            shot_filter->apply(orientation);  
            delete shot_filter;  
            need_shot=false;          
        }*/
        CALLGL(glUseProgram(output.id))

        CALLGL(glBindVertexArray(output.VAO))
        CALLGL(glBindTexture(GL_TEXTURE_2D, ((filter::Output*) *filters.rbegin())->texture))
        CALLGL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0))
        glBindVertexArray(0);
        toogle_finish();
    }

    void wait_finish() { while (!finish) {}; stop=true; }
    void take_shot( const char* path ) {

        if (need_shot) return;      // return if busy with last shot

        shot_path = path;
        need_shot=true;
    }

    void attach_filters(const std::vector<filter::Filter*>& filters_in) { filters=filters_in; }
    void surface_chanded(const dims& new_viewport) { glViewport(0, 0, viewport.first, viewport.second); }
    const Program& get_input() const { return input; }
    ~Render() { delete[] shot_pixels; }
};