import { vec3 } from 'gl-matrix';
import MinecraftWorld from '@/minecraft/MinecraftWorld';
import Observer from '@/minecraft/Observer';
import { Viewer } from '@/minecraft/Viewer';
import GameLoop from '@/engine/GameLoop';
import SharedState from '@/engine/SharedState';
import { WorldUpdate, BlockAdded } from '@gen/minecraft/updates_pb';
import { Block, IVec3 } from '@gen/minecraft/components_pb';

// TODO move this to a util class
function fromProto(proto: IVec3 | undefined): vec3 {
  if (proto) {
    return vec3.fromValues(proto.getX(), proto.getY(), proto.getZ());
  }
  return vec3.create();
}

class ObservableWorld {
  private gl: WebGLRenderingContext;
  private observer: Observer;
  private world: MinecraftWorld;
  private gameLoop: GameLoop;

  constructor(gl: WebGLRenderingContext) {
    this.gl = gl;
    this.observer = new Observer();
    this.world = new MinecraftWorld(gl);
    this.gameLoop = new GameLoop(
      this.update.bind(this),
      this.render.bind(this)
    );

    gl.clearColor(0.1, 0.1, 0.1, 1.0);
  }

  public run(): void {
    this.gameLoop.runLoop();
  }

  public handleServerUpdate(update: WorldUpdate) {
    switch (update.getUpdateCase()) {
      case WorldUpdate.UpdateCase.BLOCK_ADDED: {
        const blockAdded = update.getBlockAdded() as BlockAdded;
        let block: Block = new Block();
        if (blockAdded.hasBlock()) {
          block = blockAdded.getBlock() as Block;
        }
        this.world.addBlock(fromProto(block.getPosition()));
        break;
      }

      case WorldUpdate.UpdateCase.UPDATE_NOT_SET: {
        break;
      }
    }
  }

  private update(sharedState: SharedState): void {
    this.world.update(sharedState.timeStep);
  }

  private render(sharedState: SharedState): void {
    const gl: WebGLRenderingContext = this.gl;

    /* tslint:disable:no-bitwise */
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    /* tslint:enable:no-bitwise */
    gl.viewport(0, 0, gl.drawingBufferWidth, gl.drawingBufferHeight);

    this.world.render(this.observer.getCamera());
  }

  /*
   * Getters
   */

  get viewer(): Viewer {
    return this.observer;
  }
}

export default ObservableWorld;
