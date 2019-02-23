import {
  WorldClient,
  ResponseStream,
  Status,
  ServiceError,
} from '@gen/minecraft/world_pb_service';
import {
  WorldActionRequest,
  AddAdjacentBlockRequest,
} from '@gen/minecraft/requests_pb';
import { ClientData, Errors, Metadata } from '@gen/minecraft/world_pb';
import { WorldUpdate } from '@gen/minecraft/updates_pb';
import { v4 as uuid } from 'uuid';
import { Block, IVec3, BlockFace } from '@gen/minecraft/components_pb';
import { Empty } from 'google-protobuf/google/protobuf/empty_pb';

class MinecraftServer {
  private client: WorldClient = new WorldClient(
    'http://' + process.env.VUE_APP_PROXY_ADDRESS
  );
  private metadataStream: ResponseStream<Metadata> | null = null;
  private updateStream: ResponseStream<WorldUpdate>;
  private userMetadataCallback: (metadata: Metadata) => void;
  private userUpdateCallback: (update: WorldUpdate) => void;
  private userStatusCallback: (update: Status) => void;

  constructor(streamMetadata: boolean) {
    this.userMetadataCallback = (metadata: Metadata) => undefined; // empty
    this.userUpdateCallback = (update: WorldUpdate) => undefined; // empty
    this.userStatusCallback = (status: Status) => undefined; // empty

    // Set up the metadata stream
    if (streamMetadata) {
      this.metadataStream = this.client.metadataUpdates(new Empty());

      this.metadataStream.on('data', this.processMetadata.bind(this));
      // this.metadataStream.on('end', this.processEndStream.bind(this));
      this.metadataStream.on('status', this.processStatus.bind(this));
    }

    // Set up the world update stream
    const id: string = uuid();
    const clientData: ClientData = new ClientData();
    clientData.setName(id);
    clientData.setSendExistingState(true);

    this.updateStream = this.client.worldUpdates(clientData);

    this.updateStream.on('data', this.processUpdate.bind(this));
    // this.updateStream.on('end', this.processEndStream.bind(this));
    this.updateStream.on('status', this.processStatus.bind(this));
  }

  public addBlock(existingBlockPosition: IVec3, blockFace: BlockFace) {
    const block: Block = new Block();
    block.setPosition(existingBlockPosition);

    const request: AddAdjacentBlockRequest = new AddAdjacentBlockRequest();
    request.setExistingBlock(block);
    request.setAdjacentDirection(blockFace);

    const action: WorldActionRequest = new WorldActionRequest();
    action.setAddAdjacentBlock(request);

    this.client.modifyWorld(
      action,
      (error: ServiceError | null, response: Errors | null) => {
        if (error) {
          throw new Error(error.message); // temporary
        } else if (response && response.getErrorMessage()) {
          throw new Error(response.getErrorMessage()); // temporary
        }
      }
    );
  }

  set metadataCallback(callback: (metadata: Metadata) => void) {
    this.userMetadataCallback = callback;
  }

  set updateCallback(callback: (update: WorldUpdate) => void) {
    this.userUpdateCallback = callback;
  }

  set statusCallback(callback: (status: Status) => void) {
    this.userStatusCallback = callback;
  }

  private processMetadata(metadata: Metadata): void {
    this.userMetadataCallback(metadata);
  }

  private processUpdate(update: WorldUpdate): void {
    this.userUpdateCallback(update);
  }

  // private processEndStream(): void {}

  private processStatus(status: Status): void {
    this.userStatusCallback(status);
  }
}

export default MinecraftServer;
