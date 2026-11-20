#version 300 es
precision mediump float;

in vec3 vNormal;
in vec3 vWorldPosition;
in vec2 vUv;

uniform vec4 uBaseColor;
uniform float uMetallic;
uniform float uRoughness;
uniform vec3 uLightDirection;
uniform vec3 uCameraPosition;

out vec4 outColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(uLightDirection);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
    vec3 halfDirection = normalize(lightDirection + viewDirection);
    float shininess = mix(12.0, 72.0, clamp(1.0 - uRoughness, 0.0, 1.0));
    float specularStrength = mix(0.04, 0.45, clamp(uMetallic, 0.0, 1.0));
    float specular = pow(max(dot(normal, halfDirection), 0.0), shininess) * specularStrength;
    vec3 lit = uBaseColor.rgb * (0.28 + diffuse * 0.85) + vec3(specular);
    float distanceToCamera = length(uCameraPosition - vWorldPosition);
    float fog = clamp((distanceToCamera - 45.0) / 120.0, 0.0, 0.65);
    vec3 fogColor = vec3(0.72, 0.80, 0.88);
    outColor = vec4(mix(lit, fogColor, fog), uBaseColor.a);
}
