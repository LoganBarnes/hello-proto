<template>
  <div id="home">
    <div id="nav">
      <router-link to="/game">Play!</router-link>
    </div>
    <h1>McMinecraft</h1>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import SendButton from '@/components/SendButton.vue';
import { HelloRequest, HelloReply } from '@gen/minecraft/world_pb';
import { WorldClient, ServiceError } from '@gen/minecraft/world_pb_service';
import { grpc } from '@improbable-eng/grpc-web';

@Component({
  components: {
    SendButton,
  },
})
export default class Home extends Vue {
  // Local proxy server that forwards calls to and from the actual server
  private client: WorldClient | null = new WorldClient(
    'http://' + process.env.VUE_APP_PROXY_ADDRESS
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
