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
      <font color="gray">Advanced interaction not available on mobile</font>
    </div>
  </div>
</template>

<script lang="ts">
import { Component, Watch, Vue } from 'vue-property-decorator';
import Canvas3d from '@/components/Canvas.vue';
import ObservableWorld from '@/minecraft/ObservableWorld';
import MinecraftServer from '@/net/MinecraftServer';

@Component({
  components: {
    Canvas3d,
  },
})
export default class Home extends Vue {
  private world: ObservableWorld | null = null;
  private server: MinecraftServer = new MinecraftServer();

  public mounted() {
    const canvas: Canvas3d = this.$refs.canvas as Canvas3d;

    this.world = new ObservableWorld(canvas.glContext);
    (this.$refs.canvas as Canvas3d).viewer = this.world.viewer;

    this.server.updateCallback = this.world.handleServerUpdate.bind(this.world);

    this.world.run();
  }

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
