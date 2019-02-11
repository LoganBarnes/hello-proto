import SharedState from '@/engine/SharedState';

/**
 * Handles the main game loop logic
 */
class GameLoop {
  private timeOfLastIterationMilliseconds: number;
  private accumulator: number;
  private state: SharedState;

  private update: (sharedState: SharedState) => void;
  private render: (sharedState: SharedState) => void;

  /**
   * Sets the initial functions (optional) and all required
   * timing variables. This does not start the game loop.
   *
   * @param updateFunction The initial game update function
   * @param renderFunction The initial game render function
   */
  constructor(
    updateFunction: (sharedState: SharedState) => void,
    renderFunction: (sharedState: SharedState) => void
  ) {
    this.timeOfLastIterationMilliseconds = 0;
    this.accumulator = 0;
    this.state = new SharedState();
    this.update = updateFunction;
    this.render = renderFunction;
  }

  /**
   * The callback function for window.requestAnimationFrame().
   *
   * Performs a single iteration of the game loop. Updates will
   * be throttled to match the rate of the specified timeStep.
   * Rendering will be done on every iteration of the loop.
   *
   * Times are in seconds unless specified otherwise.
   *
   * @param currentTimeMilliseconds The time when the function is called
   */
  public loop(currentTimeMilliseconds: number): void {
    const timeDiff =
      (currentTimeMilliseconds - this.timeOfLastIterationMilliseconds) * 1e-3; // ms to s
    this.timeOfLastIterationMilliseconds = currentTimeMilliseconds;

    // Make sure there are no unusually large jumps in time
    const frameTime: number = Math.min(timeDiff, 0.1);
    this.accumulator += frameTime;

    // Throttle the updates to keep then at a constant timeStep rate
    while (this.accumulator >= this.state.timeStep) {
      this.state.worldTime += this.state.timeStep;
      this.accumulator -= this.state.timeStep;
      this.update(this.state);
    }

    // Interpolated value [0-1) between updates
    this.state.renderAlpha = this.accumulator / this.state.timeStep;
    this.render(this.state);

    window.requestAnimationFrame(this.loop.bind(this));
  }

  /**
   * Starts the game loop.
   */
  public runLoop() {
    this.timeOfLastIterationMilliseconds = window.performance.now();
    this.loop(window.performance.now());
  }

  /*
   * Getters
   */

  get sharedState(): SharedState {
    return this.state;
  }

  set updateFunction(updateFunction: (sharedState: SharedState) => void) {
    this.updateFunction = updateFunction;
  }

  set renderFunction(renderFunction: (sharedState: SharedState) => void) {
    this.renderFunction = renderFunction;
  }
}

export default GameLoop;
