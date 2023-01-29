#version 320 es

layout (location=0) in vec3 aPos;
layout (location=1) in vec2 aTexCoord;
uniform mat4 transform;
out vec2 TexCoord;

void main() {
    
    gl_Position =   vec4(aPos.x, aPos.y, aPos.z, 1.0);
    TexCoord    =   (transform * vec4(aTexCoord.x, aTexCoord.y, 0, 1.0)).xy;
}