import { Camera, CameraMover } from 'ts-graphics';

/**
 * The state of the simulation. This will be passed around
 * to each class that needs to be updated and/or rendered.
 */
class SharedState {
  public worldTime: number;
  public renderAlpha: number;
  public timeStep: number;
  public cameraMover: CameraMover;

  /**
   * Initializes all the variables.
   * @param worldTime The global game time
   * @param renderAlpha The interpolated [0 - 1) value between timeSteps
   * @param timeStep The desired time between updates
   */
  constructor(
    worldTime: number = 0.0,
    renderAlpha: number = 0.0,
    timeStep: number = 1.0 / 60.0
  ) {
    this.worldTime = worldTime;
    this.renderAlpha = renderAlpha;
    this.timeStep = timeStep;
    this.cameraMover = new CameraMover(new Camera());
  }
}

export default SharedState;
