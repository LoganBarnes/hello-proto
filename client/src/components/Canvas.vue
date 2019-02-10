<template>
  <canvas id="canvas" tabindex="1">
    HTML5 canvas is not supported in this browser :(
  </canvas>
</template>

<script lang="ts">
import Vue from 'vue';
import Component from 'vue-class-component';
import { vec2 } from 'gl-matrix';

/**
 *
 */
@Component
export default class GameLoopCanvas extends Vue {
  private gl: null | WebGLRenderingContext = null;

  /**
   * Initializes WebGL when the canvas is ready.
   */
  public mounted(): void {
    const canvas: HTMLCanvasElement = this.$el as HTMLCanvasElement;
    const options: WebGLContextAttributes = { preserveDrawingBuffer: true };
    this.gl = canvas.getContext('webgl', options);

    if (!this.gl) {
      throw new Error('Failed to initialise WebGL');
    }

    canvas.focus();

    canvas.width = canvas.clientWidth;
    canvas.height = canvas.clientHeight;
  }

  /**
   * Adjust the canvas when the window is resized
   */
  public resize(): void {
    const canvas: HTMLCanvasElement = this.$el as HTMLCanvasElement;
    canvas.width = canvas.clientWidth;
    canvas.height = canvas.clientHeight;
  }

  /**
   * Get the position of the mouse on the canvas
   */
  public getScreenPos(event: MouseEvent): vec2 | null {
    const x: number = event.clientX;
    const y: number = event.clientY;
    if (x !== undefined && y !== undefined) {
      return vec2.fromValues(x, y);
    }
    return null;
  }

  /*
   * Getters
   */

  get glContext(): WebGLRenderingContext {
    return this.gl as WebGLRenderingContext;
  }

  public width(): number {
    return (this.$el as HTMLCanvasElement).clientWidth;
  }

  public height(): number {
    return (this.$el as HTMLCanvasElement).clientHeight;
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
canvas {
  /*fill container completely */
  width: 100%;
  height: 100%;
  background-color: black; /*ideally matches WebGL clear color*/
}
</style>
