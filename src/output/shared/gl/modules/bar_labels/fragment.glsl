#version 330 core
in vec2 v_uv;
uniform sampler2D atlas;
uniform vec4 color;
out vec4 frag;
void main() {
    float a = texture(atlas, v_uv).r;
    frag = vec4(color.rgb, color.a * a);
}
