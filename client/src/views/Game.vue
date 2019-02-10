<template>
  <div id="game">
    <canvas-3d></canvas-3d>
    <!-- <AirWaves
      ref="airWaves"
      :gameLoop="gameLoop"
      :sharedState="sharedState"
    ></AirWaves> -->
    <!-- <GUI></GUI> -->
  </div>
</template>

<script lang="ts">
import Vue from 'vue';
import Component from 'vue-class-component';
import GameLoop from '@/engine/GameLoop';
import SharedState from '@/engine/SharedState';
import Canvas3d from '@/components/Canvas.vue';

@Component({
  components: {
    Canvas3d,
    // AirWaves,
  },
})
export default class App extends Vue {
  // public $refs!: {
  // airWaves: AirWaves;
  // };
  private gameLoop: GameLoop;
  private sharedState: SharedState;

  constructor() {
    super();
    this.gameLoop = new GameLoop(() => undefined, () => undefined);
    this.sharedState = this.gameLoop.sharedState;
  }

  public mounted() {
    // const airWaves = this.$refs.airWaves as AirWaves;

    // this.gameLoop.updateFunction = airWaves.updateVMP.bind(airWaves);
    // this.gameLoop.renderFunction = airWaves.renderVMP.bind(airWaves);

    this.gameLoop.runLoop();
  }
}
</script>

<style>
#game {
  width: 100vw;
  height: 100vh;
}
</style>
