<template>
  <div id="app">
    <h1>Hello, gRPC!</h1>
    <SendButton msg="Larry" @send-msg="sendMessage" />
    <SendButton msg="Curly" @send-msg="sendMessage" />
    <SendButton msg="Moe" @send-msg="sendMessage" />
    <p>Server response:</p>
    <p v-if="grpcErrors">
      <font color="red">{{ grpcErrors }}</font>
    </p>
    <p v-else>
      <font color="green">{{ grpcResponse }}</font>
    </p>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import SendButton from './components/SendButton.vue';
import { HelloRequest, HelloReply } from '../gen/hello/hello_pb';
import { GreeterClient, ServiceError } from '../gen/hello/hello_pb_service';
import { grpc } from '@improbable-eng/grpc-web';

@Component({
  components: {
    SendButton,
  },
})
export default class App extends Vue {
  // Local proxy server that forwards calls to and from the actual server
  private client: GreeterClient | null = new GreeterClient(
    process.env.VUE_APP_PROXY_ADDRESS
  );
  private grpcResponse: string = '';
  private grpcErrors: string = '';

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
          this.grpcResponse = '';
          this.grpcErrors = 'Error: ' + error.message;
        } else {
          this.grpcErrors = '';
          this.grpcResponse = !response ? 'null' : response.getMessage();
        }
      }
    );
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
  margin-top: 60px;
}
</style>
