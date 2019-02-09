<template>
  <div id="app">
    <SendButton msg="First"/>
    <SendButton msg="Second"/>
    <SendButton msg="Third"/>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import SendButton from './components/SendButton.vue';

@Component({
  components: {
    SendButton,
  },
})
export default class App extends Vue {}
</script>

<style>
#app {
  font-family: "Avenir", Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  margin-top: 60px;
}
</style>
