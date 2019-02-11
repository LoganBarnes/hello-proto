import { vec3 } from 'gl-matrix';
import blocksVert from 'ts-shader-loader!@/assets/shaders/blocks.vert';
import debugFrag from 'ts-shader-loader!@/assets/shaders/debug.frag';
import {
  Cube,
  GLUtils,
  GLProgram,
  GLBuffer,
  Camera,
  Renderable,
  Scene,
} from 'ts-graphics';
import { VertexArray } from '@/graphics/VertexArray';

/**
 * A single Minecraft block.
 */
class Blocks extends Renderable {
  private glUtils: GLUtils;
  private program: GLProgram;
  private vbo: GLBuffer;
  private vao: VertexArray;
  private ibo: GLBuffer;
  private drawCount: number;
  private positions: vec3[]; // TODO: make this a map

  constructor(glUtils: GLUtils) {
    super();
    this.glUtils = glUtils;
    this.positions = [];

    const gl = this.glUtils.getContext();

    this.program = this.glUtils.createProgram(
      {
        text: blocksVert,
        type: gl.VERTEX_SHADER,
      },
      {
        text: debugFrag,
        type: gl.FRAGMENT_SHADER,
      }
    );

    const cube: Cube = new Cube();

    const verts: Float32Array = cube.vertexData;
    const norms: Float32Array = cube.normalData;
    const tcrds: Float32Array = cube.texCoordData;

    const data: Float32Array = new Float32Array(
      verts.length + norms.length + tcrds.length
    );
    data.set(verts);
    data.set(norms, verts.length);
    data.set(tcrds, verts.length + norms.length);
    this.vbo = this.glUtils.createVbo(data);

    const indices: Uint16Array = cube.indexData;
    this.ibo = this.glUtils.createIbo(indices);

    this.drawCount = indices.length;

    this.vao = new VertexArray(gl, this.program, this.vbo, 0, [
      {
        name: 'localPosition',
        size: 3,
        type: gl.FLOAT,
        offset: 0,
      },
      {
        name: 'localNormal',
        size: 3,
        type: gl.FLOAT,
        offset: cube.getVertexDataSizeInBytes(),
      },
      {
        name: 'texCoords',
        size: 2,
        type: gl.FLOAT,
        offset:
          cube.getVertexDataSizeInBytes() + cube.getNormalDataSizeInBytes(),
      },
    ]);
  }

  public onRender(camera: Camera, scene: Scene): void {
    const gl: WebGLRenderingContext = this.glUtils.getContext();

    this.program.use(
      (): void => {
        /*
         * Bind everything that doesn't change
         */
        this.program.setMatrixUniform(
          camera.screenFromWorldMatrix,
          'screenFromWorld'
        );
        this.program.setIntUniform(scene.displayMode, 'displayMode');

        this.vao.prepBuffer();
        this.ibo.bind();

        /*
         * Iterate over each block updating only what is necessary
         */
        for (const position of this.positions) {
          this.program.setFloatUniform(position, 'offset');
          this.vao.indexedDraw(gl.TRIANGLES, 0, this.drawCount, this.ibo);
        }
      }
    );
  }

  public addBlock(position: vec3) {
    this.positions.push(position);
  }
}

export default Blocks;
