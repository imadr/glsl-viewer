#version 330 core

in vec2 frag_coord;

uniform float time;

out vec4 frag_color;

void main(){
    vec3 col = 0.5+0.5*cos(time+frag_coord.xyx+vec3(0, 2, 4));
    frag_color = vec4(col, 1);
}