<template>
  <canvas
    id="canvas"
    tabindex="1"
    @mousedown.left="inputHandler.handleMousePress($event)"
    @mouseup.left="inputHandler.handleMouseRelease($event)"
    @mousemove.left="inputHandler.handleMouseMove($event)"
    @touchstart.left="inputHandler.handleMousePress($event)"
    @touchend.left="inputHandler.handleMouseRelease($event)"
    @touchmove.left="inputHandler.handleMouseMove($event)"
  >
    HTML5 canvas is not supported in this browser :(
  </canvas>
</template>

<script lang="ts">
import Vue from 'vue';
import Component from 'vue-class-component';
import { vec2 } from 'gl-matrix';
import { Viewer, EmptyViewer } from '@/minecraft/Viewer';

/**
 *
 */
@Component
export default class Canvas3d extends Vue {
  private gl: null | WebGLRenderingContext = null;
  private inputHandler: Viewer = new EmptyViewer();

  /**
   * Initializes WebGL when the canvas is ready.
   */
  public mounted(): void {
    const canvas: HTMLCanvasElement = this.$el as HTMLCanvasElement;
    const options: WebGLContextAttributes = { preserveDrawingBuffer: true };
    const gl = canvas.getContext('webgl', options);

    if (!gl) {
      throw new Error('Failed to initialise WebGL');
    }
    this.gl = gl;

    canvas.focus();

    canvas.width = canvas.clientWidth;
    canvas.height = canvas.clientHeight;
  }

  /**
   * Adjust the canvas when the window is resized
   */
  public resize(): vec2 {
    const canvas: HTMLCanvasElement = this.$el as HTMLCanvasElement;
    canvas.width = canvas.clientWidth;
    canvas.height = canvas.clientHeight;

    return vec2.fromValues(canvas.width, canvas.height);
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

  /*
   * Setters
   */

  set viewer(inputHandler: Viewer) {
    this.inputHandler = inputHandler;

    const canvas: HTMLCanvasElement = this.$el as HTMLCanvasElement;
    this.inputHandler.resize(canvas.width, canvas.height);
  }

  get viewer(): Viewer {
    return this.inputHandler;
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
