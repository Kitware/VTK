package vtk.rendering.swt;

import org.eclipse.swt.opengl.GLCanvas;
import org.eclipse.swt.widgets.Composite;

import vtk.vtkRenderWindow;
import vtk.rendering.vtkAbstractComponent;

/**
 * Provide SWT based vtk rendering component
 *
 * @author    Joachim Pouderoux - joachim.pouderoux@kitware.com, Kitware SAS 2012
 * @copyright This work was supported by CEA/CESTA
 *            Commissariat a l'Energie Atomique et aux Energies Alternatives,
 *            15 avenue des Sablieres, CS 60001, 33116 Le Barp, France.
 */
public class vtkSwtComponent extends vtkAbstractComponent<GLCanvas> {

  protected vtkInternalSwtComponent uiComponent;
  protected boolean isWindowCreated;

  public vtkSwtComponent(Composite parentComposite) {
    this(new vtkRenderWindow(), parentComposite);
  }

  public vtkSwtComponent(vtkRenderWindow renderWindowToUse, Composite parentComposite) {
    super(renderWindowToUse);
    this.eventForwarder = new vtkSwtInteractorForwarderDecorator(this, this.eventForwarder);
    this.isWindowCreated = true;
    this.uiComponent = new vtkInternalSwtComponent(this, parentComposite);
  }

  /**
   * Set the size of the VTK component
   * @param x width
   * @param y height
   */
  public void setSize(int x, int y) {
    x = x < 1 ? 1 : x;
    y = y < 1 ? 1 : y;
    super.setSize(x, y);
    this.uiComponent.setSize(x, y);
    this.uiComponent.redraw();
    this.uiComponent.update();
  }

  /**
   * Render the VTK component. Should not be called externally.
   * Call update() to refresh the window content.
   */
  public void Render() {
    // Make sure we can render
    if (inRenderCall || renderer == null || renderWindow == null) {
      return;
    }

    // Try to render
    try {
      lock.lockInterruptibly();
      inRenderCall = true;
      // Trigger the real render
      renderWindow.Render();
    } catch (InterruptedException e) {
      // Nothing that we can do except skipping execution
    } finally {
      lock.unlock();
      inRenderCall = false;
    }
  }

  /**
   * Redraw the VTK component
   */
  public void update() {
    this.uiComponent.redraw();
    this.uiComponent.update();
  }

  /**
   * @return the encapsulated SWT component (a GLCanvas instance)
   * @see vtk.rendering.vtkAbstractComponent#getComponent()
   */
  public GLCanvas getComponent() {
    return this.uiComponent;
  }

  public void Delete() {
    this.lock.lock();
    // We prevent any further rendering
    this.inRenderCall = true;
    this.renderWindow = null;
    super.Delete();
    this.lock.unlock();
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
