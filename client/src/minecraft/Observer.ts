import { CameraMover, Camera } from 'ts-graphics';
import { Viewer } from '@/minecraft/Viewer';
import ViewerUtil from '@/minecraft/ViewerUtil';
import { vec2, vec3 } from 'gl-matrix';

/**
 * Current movement type based on what buttons are down.
 *
 * Current controls:
 *      Left Drag      -  Orbit around point
 *   Ctrl + Left Drag  -  Pan perpendicular to view direction
 */
enum CameraMovement {
  None,
  Orbit,
  Pan,
}

/**
 * A mouse driven camera class used to orbit around a scene.
 */
class Observer implements Viewer {
  private cameraMover: CameraMover;
  private movementType: CameraMovement;
  private previousScreenPosition: vec2 | null;
  private viewport: vec2;

  constructor() {
    this.cameraMover = new CameraMover(new Camera());
    this.movementType = CameraMovement.None;
    this.previousScreenPosition = null;
    this.viewport = vec2.create();

    this.cameraMover.offsetFromAnchor = 5.0;
    this.cameraMover.pitch(-23.0);
    this.cameraMover.yaw(37.0);
  }

  public getCamera(): Camera {
    return this.cameraMover.camera;
  }

  /**
   * Reset the camera aspect ratio and save the dimensions
   * @param width The viewport width
   * @param height The viewport height
   */
  public resize(width: number, height: number): void {
    this.cameraMover.camera.aspectRatio = width / height;
    vec2.set(this.viewport, width, height);
  }

  /**
   * Determine the current mouse position (if on the canvas)
   * and set the camera movement type (pan or orbit).
   * @param event The mouse event
   */
  public handleMousePress(event: MouseEvent): void {
    event.preventDefault();
    const ctrlPressed = event.ctrlKey || event.metaKey;
    this.movementType = ctrlPressed ? CameraMovement.Pan : CameraMovement.Orbit;
    this.previousScreenPosition = ViewerUtil.getScreenPos(event);
  }

  /**
   * Reset parameters to null values so we know the
   * mouse button is not pressed.
   * @param event The mouse event
   */
  public handleMouseRelease(event: MouseEvent): void {
    this.movementType = CameraMovement.None;
    this.previousScreenPosition = null;
  }

  /**
   * If the mouse is pressed, this will orbit the camera
   * around a point or translate the camera sideways relative
   * to the screen (pan).
   * @param event The mouse event
   */
  public handleMouseMove(event: MouseEvent): void {
    if (this.movementType === CameraMovement.None) {
      return;
    }
    event.preventDefault();

    const currentScreenPosition: vec2 | null = ViewerUtil.getScreenPos(event);

    switch (this.movementType) {
      case CameraMovement.Pan: {
        this.pan(event, currentScreenPosition);
        break;
      }

      case CameraMovement.Orbit: {
        this.orbit(event, currentScreenPosition);
        break;
      }
    }
    this.previousScreenPosition = currentScreenPosition;
  }

  /**
   * Move the camera perpendicular to the view direction.
   * @param event The mouse event
   * @param currentScreenPosition The position of the mouse
   */
  private pan(event: MouseEvent, currentScreenPosition: vec2 | null): void {
    if (
      this.previousScreenPosition !== null &&
      currentScreenPosition !== null
    ) {
      const previousPosition: vec3 | null = ViewerUtil.getWorldPos(
        this.previousScreenPosition,
        this.viewport,
        this.cameraMover
      );

      const currentPosition: vec3 | null = ViewerUtil.getWorldPos(
        currentScreenPosition,
        this.viewport,
        this.cameraMover
      );

      if (previousPosition !== null && currentPosition !== null) {
        const diff = vec3.sub(vec3.create(), previousPosition, currentPosition);
        this.cameraMover.pan(diff);
      }
    }
  }

  /**
   * Move the camera around a central point.
   * @param event The mouse event
   * @param currentScreenPosition The position of the mouse
   */
  private orbit(event: MouseEvent, currentScreenPosition: vec2 | null): void {
    if (
      this.previousScreenPosition !== null &&
      currentScreenPosition !== null
    ) {
      this.cameraMover.yaw(
        (this.previousScreenPosition[0] - currentScreenPosition[0]) * 0.5
      );
      this.cameraMover.pitch(
        (this.previousScreenPosition[1] - currentScreenPosition[1]) * 0.5
      );
    }
  }
}

export default Observer;
