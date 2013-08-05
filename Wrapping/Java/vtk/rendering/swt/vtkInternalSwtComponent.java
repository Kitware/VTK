package vtk.rendering.swt;

import org.eclipse.swt.SWT;
import org.eclipse.swt.opengl.GLCanvas;
import org.eclipse.swt.opengl.GLData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;

import vtk.vtkObject;

/**
 * @author    Joachim Pouderoux - joachim.pouderoux@kitware.com, Kitware SAS 2012
 * @copyright This work was supported by CEA/CESTA
 *            Commissariat a l'Energie Atomique et aux Energies Alternatives,
 *            15 avenue des Sablieres, CS 60001, 33116 Le Barp, France.
 */
public class vtkInternalSwtComponent extends GLCanvas implements Listener {

  private vtkSwtComponent parent;

  public static GLData GetGLData() {

    GLData gl = new GLData();
    gl.doubleBuffer = true;
    gl.depthSize = 1; // must be set to something on Linux
    return gl;
  }

  public vtkInternalSwtComponent(vtkSwtComponent parent, Composite parentComposite) {

    super(parentComposite, SWT.NO_BACKGROUND, GetGLData());
    this.parent = parent;

    vtkSwtInteractorForwarderDecorator forwarder =
        (vtkSwtInteractorForwarderDecorator)this.parent.getInteractorForwarder();

    this.addMouseListener(forwarder);
    this.addKeyListener(forwarder);
    this.addMouseMoveListener(forwarder);
    this.addMouseTrackListener(forwarder);
    this.addMouseWheelListener(forwarder);

    this.addListener(SWT.Paint, this);
    this.addListener(SWT.Close, this);
    this.addListener(SWT.Dispose, this);
    this.addListener(SWT.Resize, this);

    this.IntializeRenderWindow();
  }

  protected void IntializeRenderWindow() {

    setCurrent(); // need to be done so SetWindowIdFromCurrentContext can get the current context!
    parent.getRenderWindow().InitializeFromCurrentContext();
  }

  public void update() {
    super.update();
    parent.Render();
  }

  public void handleEvent(Event event) {
    switch (event.type) {
    case SWT.Paint:
      parent.Render();
      break;
    case SWT.Dispose:
      parent.Delete();
      vtkObject.JAVA_OBJECT_MANAGER.gc(false);
      break;
    case SWT.Close:
      //System.out.println("closing");
      break;
    case SWT.Resize:
      parent.setSize(getClientArea().width, getClientArea().height);
      break;
    }
  }
}
