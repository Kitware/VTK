package vtk.rendering.awt;

import java.awt.Canvas;
import java.awt.Graphics;

import vtk.vtkRenderWindow;

public class vtkInternalAwtComponent extends Canvas {
  protected native int RenderCreate(vtkRenderWindow renderWindow);

  private static final long serialVersionUID = -7756069664577797620L;
  private vtkAwtComponent parent;

  public vtkInternalAwtComponent(vtkAwtComponent parent) {
    this.parent = parent;
    this.addMouseListener(this.parent.getInteractorForwarder());
    this.addMouseMotionListener(this.parent.getInteractorForwarder());
    this.addMouseWheelListener(this.parent.getInteractorForwarder());
    this.addKeyListener(this.parent.getInteractorForwarder());
  }

  public void addNotify() {
    super.addNotify();
    parent.isWindowCreated = false;
    parent.getRenderWindow().SetForceMakeCurrent();
    parent.updateInRenderCall(false);
  }

  public void removeNotify() {
    parent.updateInRenderCall(true);
    super.removeNotify();
  }

  public void paint(Graphics g) {
    parent.Render();
  }

  public void update(Graphics g) {
    parent.Render();
  }
}
