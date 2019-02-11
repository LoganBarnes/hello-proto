/**
 * Interface to handle input callbacks and camera updates
 */
export interface Viewer {
  resize(width: number, height: number): void;
  handleMousePress(event: MouseEvent): void;
  handleMouseRelease(event: MouseEvent): void;
  handleMouseMove(event: MouseEvent): void;
}

export class EmptyViewer implements Viewer {
  public resize(width: number, height: number): void {
    /*No-Op*/
  }
  public handleMousePress(event: MouseEvent): void {
    /*No-Op*/
  }
  public handleMouseRelease(event: MouseEvent): void {
    /*No-Op*/
  }
  public handleMouseMove(event: MouseEvent): void {
    /*No-Op*/
  }
}
