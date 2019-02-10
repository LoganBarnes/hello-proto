<template>
  <div id="home">
    <h1>McMinecraft</h1>
    <div id="canvas-wrapper">
      <canvas-3d ref="canvas"></canvas-3d>
      <resize-observer @notify="handleResize" />
    </div>
    <div v-if="!mobileDetected()" id="nav">
      <router-link to="/game">Play!</router-link>
    </div>
    <div v-else>
      <font color="gray">(Playing requires a keyboard and mouse)</font>
    </div>
  </div>
</template>

<script lang="ts">
import { Component, Watch, Vue } from 'vue-property-decorator';
import Canvas3d from '@/components/Canvas.vue';

@Component({
  components: {
    Canvas3d,
  },
})
export default class Home extends Vue {
  /**
   * Pass resize events to the canvas so it can update
   * the appropriate variables.
   */
  public handleResize() {
    (this.$refs.canvas as Canvas3d).resize();
  }

  /**
   * Detect mobile devices so we can prevent them from attempting
   * to play the game that requires a mouse and keyboard.
   */
  public mobileDetected() {
    return /Android|webOS|iPhone|iPad|iPod|BlackBerry|BB|PlayBook|IEMobile|Windows Phone|Kindle|Silk|Opera Mini/i.test(
      navigator.userAgent
    );
  }
}
</script>

<style>
#canvas-wrapper {
  width: 80vw;
  height: 60vw;
  margin: auto;
  background-color: black; /*ideally matches WebGL clear color*/
}
</style>
