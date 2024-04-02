#version 430 core

layout (location = 0) in vec2 pos;

out vec2 uv;

void main() {
    uv = vec2(pos.x, pos.y);
    gl_Position = vec4(uv, 0.0f, 1.0);
}
