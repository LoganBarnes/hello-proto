import { Scene, GLUtils } from 'ts-graphics';

class MinecraftWorld {
  private scene: Scene;
  private glUtils: GLUtils;

  constructor(gl: WebGLRenderingContext) {
    this.scene = new Scene();
    this.glUtils = new GLUtils(gl);
  }
}

export default MinecraftWorld;
