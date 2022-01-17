#version 330 core

layout (location = 0) in vec3 a_position;

out vec2 frag_coord;

void main(){
    gl_Position = vec4(a_position, 1.0);
    frag_coord = a_position.xy;
}