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
import { HelloRequest, HelloReply } from '../generated/hello/hello_pb';
import { GreeterClient, ServiceError } from '../generated/hello/hello_pb_service';
import { grpc } from '@improbable-eng/grpc-web';

@Component({
  components: {
    SendButton,
  },
})
export default class App extends Vue {
  private client: GreeterClient|null = new GreeterClient('http://localhost:8080');

  public mounted(): void {
    if (!this.client) {
      return;
    }
    const request = new HelloRequest();
    request.setName('World');

    this.client.sayHello(
      request,
      new grpc.Metadata(),
      (error: ServiceError | null, response: HelloReply | null) => {
        if (error) {
          console.error(error.message);
        } else {
          console.log(!response ? 'null' : response.getMessage());
        }
      },
    );
  }
}
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
