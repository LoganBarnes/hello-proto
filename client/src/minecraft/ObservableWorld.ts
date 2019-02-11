import MinecraftWorld from '@/minecraft/MinecraftWorld';
import Observer from '@/minecraft/Observer';
import { Viewer } from '@/minecraft/Viewer';

class ObservableWorld {
  private observer: Observer;
  private world: MinecraftWorld;

  constructor(gl: WebGLRenderingContext) {
    this.observer = new Observer();
    this.world = new MinecraftWorld(gl);
  }

  /*
   * Getters
   */

  get viewer(): Viewer {
    return this.observer;
  }
}

export default ObservableWorld;
