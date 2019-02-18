<template>
  <div id="app">
    <h1>Hello, gRPC!</h1>
    <SendButton msg="Larry" @send-msg="sendMessage" />
    <SendButton msg="Curly" @send-msg="sendMessage" />
    <SendButton msg="Moe" @send-msg="sendMessage" />
    <p>Server transactions:</p>
    <p v-if="grpcErrors">
      <font color="red">{{ grpcErrors }}</font>
    </p>
    <p v-else v-for="item in grpcTransactions" :key="item.id">
      <font color="green">
        {{ item.toArray() }}
      </font>
    </p>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import SendButton from './components/SendButton.vue';
import {
  HelloRequest,
  HelloResponse,
  HelloTransaction,
} from '@gen/hello/hello_pb';
import {
  GreeterClient,
  ResponseStream,
  ServiceError,
} from '@gen/hello/hello_pb_service';
import { grpc } from '@improbable-eng/grpc-web';
import { Empty } from 'google-protobuf/google/protobuf/empty_pb';

@Component({
  components: {
    SendButton,
  },
})
export default class App extends Vue {
  // Local proxy server that forwards calls to and from the actual server
  private client: GreeterClient | null = new GreeterClient(
    'http://' + process.env.VUE_APP_PROXY_ADDRESS
  );
  private grpcTransactions: HelloTransaction[] = [];
  private grpcErrors: string = '';

  public sendMessage(name: string): void {
    if (!this.client) {
      return;
    }

    const transaction: HelloTransaction = new HelloTransaction();

    const request: HelloRequest = new HelloRequest();
    request.setName(name);

    this.client.maybeSayHello(
      request,
      new grpc.Metadata(),
      (error: ServiceError | null, response: Empty | null) => {
        if (error) {
          this.grpcErrors = 'Error: ' + error.message;
        }
      }
    );
  }

  private mounted(): void {
    if (!this.client) {
      return;
    }

    // const client: GreeterClient = this.client;
    // const grpcTransactions: HelloTransaction[] = this.grpcTransactions;

    // let transactions: ResponseStream<HelloTransaction>;

    // transactions = this.client.getAllTransactions(new Empty());

    // // Receive all existing transactions
    // transactions.on('data', (message: HelloTransaction) => {
    //   grpcTransactions.push(message);
    // });

    // transactions.on('end', () => {
    //   // All existing transactions received. Set up a stream
    //   // to wait for continuous updates from the server.
    //   transactions = client.getTransactionUpdates(new Empty());

    //   transactions.on('data', (message: HelloTransaction) => {
    //     grpcTransactions.push(message);
    //   });
    // });
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
