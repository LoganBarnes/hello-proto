import { ClientData, HelloRequest, HelloReply } from '@gen/minecraft/world_pb';
import { WorldUpdate } from '@gen/minecraft/updates_pb';
import {
  WorldClient,
  ResponseStream,
  Status,
  ServiceError,
} from '@gen/minecraft/world_pb_service';
import { v4 as uuid } from 'uuid';

class MinecraftServer {
  private client: WorldClient = new WorldClient(
    'http://' + process.env.VUE_APP_PROXY_ADDRESS
  );
  private updateStream: ResponseStream<WorldUpdate>;
  private userUpdateCallback: (update: WorldUpdate) => void;

  constructor() {
    this.userUpdateCallback = (update: WorldUpdate) => undefined; // empty

    const id: string = uuid();
    const clientData: ClientData = new ClientData();
    clientData.setName(id);

    this.updateStream = this.client.worldUpdates(clientData);

    this.updateStream.on('data', this.processUpdate.bind(this));
    this.updateStream.on('end', this.processEndStream.bind(this));
    this.updateStream.on('status', this.processStatus.bind(this));

    const request: HelloRequest = new HelloRequest();
    request.setName('Larry');

    this.client.sayHello(
      request,
      (error: ServiceError | null, response: HelloReply | null) => {
        console.error(error);
        console.log(response);
      }
    );
  }

  set updateCallback(callback: (update: WorldUpdate) => void) {
    this.userUpdateCallback = callback;
  }

  private processUpdate(update: WorldUpdate): void {
    this.userUpdateCallback(update);
  }

  private processEndStream(): void {
    // console.log('end');
  }

  private processStatus(status: Status): void {
    // console.log('status: ' + status.details);
  }
}

export default MinecraftServer;
