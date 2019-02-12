import { ClientData } from '@gen/minecraft/world_pb';
import { WorldUpdate } from '@gen/minecraft/updates_pb';
import {
  WorldClient,
  ResponseStream,
  ServiceError,
  Status,
} from '@gen/minecraft/world_pb_service';
import { v4 as uuid } from 'uuid';

class MinecraftServer {
  private client: WorldClient = new WorldClient(
    'http://' + process.env.VUE_APP_PROXY_ADDRESS
  );
  private updateStream: ResponseStream<WorldUpdate>;
  private updateCallback: (update: WorldUpdate) => void;

  constructor() {
    this.updateCallback = (update: WorldUpdate) => undefined; // empty

    const id: string = uuid();
    const clientData: ClientData = new ClientData();
    clientData.setName(id);

    this.updateStream = this.client.worldUpdates(clientData);

    this.updateStream.on('data', this.processUpdate.bind(this));
    this.updateStream.on('end', this.processEndStream.bind(this));
    this.updateStream.on('status', this.processStatus.bind(this));
  }

  private processUpdate(update: WorldUpdate): void {
    this.updateCallback(update);
  }

  private processEndStream(): void {
    // console.log('end');
  }

  private processStatus(status: Status): void {
    // console.log('status: ' + status.details);
  }
}

export default MinecraftServer;
