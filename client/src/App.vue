<template>
  <div id="app">
    <router-view />
  </div>
</template>

<script lang="ts">
import { Component, Vue, Watch } from 'vue-property-decorator';

@Component
export default class App extends Vue {
  /**
   * Set the 'game' view to use fullscreen settings. Otherwise,
   * restore the settings to their defaults.
   */
  public handleStyles() {
    if (['/game'].includes(this.$route.path)) {
      document.body.className = 'fullscreen';
    } else {
      document.body.className = 'normal-size';
    }
  }

  /**
   * Triggered with the view changes
   */
  @Watch('$route')
  public onRouteChanged() {
    this.handleStyles();
  }
}
</script>

<style>
#app {
  font-family: 'Avenir', Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
}

#nav {
  padding: 30px;
}

#nav a {
  font-weight: bold;
  color: #2c3e50;
}

.normal-size {
  height: auto;
  margin: auto;
  overflow: auto;
}

.fullscreen {
  height: 100%;
  margin: 0px;
  overflow: hidden;
}
</style>
