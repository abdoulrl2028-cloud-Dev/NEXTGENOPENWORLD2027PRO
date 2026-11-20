#version 300 es
precision mediump float;

in vec2 vUv;

uniform sampler2D uTexture;
uniform vec4 uColor;

out vec4 outColor;

void main() {
    outColor = texture(uTexture, vUv) * uColor;
}
