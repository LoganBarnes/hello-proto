#version 100
precision highp float;
precision highp int;

varying vec3 worldPosition;
varying vec3 worldNormal;
varying vec2 texc;
varying vec3 vertexColor;

uniform int displayMode;

void main() {
  vec3 outColor = vec3(1.0);

  if (displayMode == 0) {
      outColor = worldPosition;

  } else if (displayMode == 1) {
      outColor = worldNormal * 0.5 + 0.5;

  } else if (displayMode == 2) {
      outColor = vec3(texc, 1.0);

  } else if (displayMode == 3) {
      outColor = vertexColor;

  }

  gl_FragColor = vec4(outColor, 1.0);
}
