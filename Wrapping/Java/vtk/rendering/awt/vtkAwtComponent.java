package vtk.rendering.awt;

import java.awt.Canvas;
import java.awt.Dimension;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;

import vtk.vtkObject;
import vtk.vtkRenderWindow;
import vtk.rendering.vtkAbstractComponent;

/**
 * Provide AWT based vtk rendering component
 *
 * @authors Sebastien Jourdain - sebastien.jourdain@kitware.com
 *          Joachim Pouderoux - joachim.pouderoux@kitware.com
 */
public class vtkAwtComponent extends vtkAbstractComponent<Canvas> {
  protected vtkInternalAwtComponent uiComponent;
  protected boolean isWindowCreated;

  public vtkAwtComponent() {
    this(new vtkRenderWindow());
  }

  public vtkAwtComponent(vtkRenderWindow renderWindowToUse) {
    super(renderWindowToUse);
    this.isWindowCreated = false;
    this.uiComponent = new vtkInternalAwtComponent(this);
    this.uiComponent.addComponentListener(new ComponentAdapter() {

      public void componentResized(ComponentEvent arg0) {
        Dimension size = vtkAwtComponent.this.uiComponent.getSize();
        vtkAwtComponent.this.setSize(size.width, size.height);
      }
    });
  }

  public void Render() {
    // Make sure we can render
    if (inRenderCall || renderer == null || renderWindow == null) {
      return;
    }

    // Try to render
    try {
      lock.lockInterruptibly();
      inRenderCall = true;

      // Initialize the window only once
      if (!isWindowCreated) {
        uiComponent.RenderCreate(renderWindow);
        setSize(uiComponent.getWidth(), uiComponent.getHeight());
        isWindowCreated = true;
      }

      // Trigger the real render
      renderWindow.Render();
    } catch (InterruptedException e) {
      // Nothing that we can do except skipping execution
    } finally {
      lock.unlock();
      inRenderCall = false;
    }
  }

  public Canvas getComponent() {
    return this.uiComponent;
  }

  public void Delete() {
    this.lock.lock();

    // We prevent any further rendering
    inRenderCall = true;

    if (this.uiComponent.getParent() != null) {
      this.uiComponent.getParent().remove(this.uiComponent);
    }
    super.Delete();

    // On linux we prefer to have a memory leak instead of a crash
    if (!this.renderWindow.GetClassName().equals("vtkXOpenGLRenderWindow")) {
      this.renderWindow = null;
    } else {
      System.out.println("The renderwindow has been kept arount to prevent a crash");
    }
    this.lock.unlock();
    vtkObject.JAVA_OBJECT_MANAGER.gc(false);
  }

  /**
   * @return true if the graphical component has been properly set and
   *         operation can be performed on it.
   */
  public boolean isWindowSet() {
    return this.isWindowCreated;
  }

  /**
   * Just allow class in same package to affect inRenderCall boolean
   *
   * @param value
   */
  protected void updateInRenderCall(boolean value) {
    this.inRenderCall = value;
  }
}
