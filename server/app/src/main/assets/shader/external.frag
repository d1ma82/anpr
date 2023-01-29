#version 320 es
#extension GL_OES_EGL_image_external_essl3 : require

precision mediump float;
out vec4 FragColor;
in vec2 TexCoord;
uniform samplerExternalOES camera;

void main() {
    
    FragColor=texture(camera, TexCoord);
}