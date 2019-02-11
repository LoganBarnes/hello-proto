import { CameraUtils, CameraMover, Ray3 } from 'ts-graphics';
import { vec2, vec3 } from 'gl-matrix';

function isTouchEvent(event: MouseEvent | TouchEvent): event is TouchEvent {
  return (event as TouchEvent).touches !== undefined;
}

class ViewerUtil {
  /**
   * Gets the world position of the mouse if it intersected
   * a plane at the point where the camera is focused.
   * @param mousePosition The current position (x, y) of the mouse
   * @param viewport The size of the viewport (w, h)
   * @param cameraMover The object that manipulates the camera
   */
  public static getWorldPos(
    mousePosition: vec2,
    viewport: vec2,
    cameraMover: CameraMover
  ): vec3 | null {
    const ray: Ray3 = CameraUtils.getRayFromScreenPos(
      cameraMover.camera,
      mousePosition[0], // x
      mousePosition[1], // y
      viewport[0], // width
      viewport[1] // height
    );

    return cameraMover.intersectPlaneAtAnchor(ray);
  }

  /**
   * Gets the screen position of a mouse or touch event.
   * @param event The mouse or touch event
   */
  public static getScreenPos(event: MouseEvent | TouchEvent): vec2 | null {
    let x: number;
    let y: number;
    if (isTouchEvent(event)) {
      x = event.touches[0].clientX;
      y = event.touches[0].clientY;
    } else {
      x = event.clientX;
      y = event.clientY;
    }
    if (x !== undefined && y !== undefined) {
      return vec2.fromValues(x, y);
    }
    return null;
  }
}

export default ViewerUtil;
