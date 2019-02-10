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

    transaction.setRequest(request);

    this.client.sayHello(
      request,
      new grpc.Metadata(),
      (error: ServiceError | null, response: HelloResponse | null) => {
        if (error) {
          const errorResponse: HelloResponse = new HelloResponse();
          errorResponse.setMessage('ERROR');
          transaction.setResponse(errorResponse);

          this.grpcErrors = 'Error: ' + error.message;
        } else if (response) {
          this.grpcErrors = '';
          transaction.setResponse(response);
        } else {
          // assert?
        }

        this.grpcTransactions.push(transaction);
      }
    );
  }

  private mounted(): void {
    if (!this.client) {
      return;
    }
    const transactions: ResponseStream<
      HelloTransaction
    > = this.client.getAllTransactions(new Empty());

    const grpcTransactions: HelloTransaction[] = this.grpcTransactions;

    transactions.on('data', (message: HelloTransaction) => {
      grpcTransactions.push(message);
    });
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
