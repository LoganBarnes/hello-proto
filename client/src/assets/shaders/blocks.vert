#version 100
precision highp float;
precision highp int;

attribute vec3 localPosition;
attribute vec3 localNormal;
attribute vec2 texCoords;
attribute vec3 color;

uniform mat4 screenFromWorld;
uniform vec3 offset;

varying vec3 worldPosition;
varying vec3 worldNormal;
varying vec2 texc;
varying vec3 vertexColor;

void main() {
  worldPosition = localPosition + offset;
  worldNormal = localNormal;
  texc = texCoords;
  vertexColor = color;
  gl_Position = screenFromWorld * vec4(worldPosition, 1.0);
}
