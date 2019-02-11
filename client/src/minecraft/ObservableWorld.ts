import { vec3 } from 'gl-matrix';
import MinecraftWorld from '@/minecraft/MinecraftWorld';
import Observer from '@/minecraft/Observer';
import { Viewer } from '@/minecraft/Viewer';
import GameLoop from '@/engine/GameLoop';
import SharedState from '@/engine/SharedState';

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

    // for testing until this is dynamic
    this.world.addBlock(vec3.fromValues(-1, 0, 0));
    this.world.addBlock(vec3.fromValues(0, -1, 0));
    this.world.addBlock(vec3.fromValues(0, 0, -1));
    this.world.addBlock(vec3.fromValues(1, 0, 0));
    this.world.addBlock(vec3.fromValues(0, 1, 0));
    this.world.addBlock(vec3.fromValues(0, 0, 1));
  }

  public run(): void {
    this.gameLoop.runLoop();
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
