<template>
  <div id="app">
    <h1>Hello, gRPC!</h1>
    <SendButton msg="Larry" @send-msg="sendMessage"/>
    <SendButton msg="Curly" @send-msg="sendMessage"/>
    <SendButton msg="Moe" @send-msg="sendMessage"/>
    <p>Grpc output: {{grpcResponse}}</p>
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
  // Local proxy server that forwards calls to and from the actual server
  private client: GreeterClient|null = new GreeterClient('http://localhost:8080');
  private grpcResponse: string = '';

  public sendMessage(msg: string): void {
    if (!this.client) {
      return;
    }
    const request = new HelloRequest();
    request.setName(msg);

    this.client.sayHello(
      request,
      new grpc.Metadata(),
      (error: ServiceError | null, response: HelloReply | null) => {
        if (error) {
          this.grpcResponse = 'Error: ' + error.message;
        } else {
          this.grpcResponse = (!response ? 'null' : response.getMessage());
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
