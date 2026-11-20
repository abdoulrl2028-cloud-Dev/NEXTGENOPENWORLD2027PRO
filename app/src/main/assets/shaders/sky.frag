#version 300 es
precision mediump float;

in vec2 vUv;
out vec4 outColor;

void main() {
    vec3 horizon = vec3(0.73, 0.81, 0.88);
    vec3 zenith = vec3(0.30, 0.50, 0.78);
    float height = clamp(vUv.y, 0.0, 1.0);
    outColor = vec4(mix(horizon, zenith, height), 1.0);
}
