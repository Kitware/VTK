package vtk.rendering.jogl;

import com.jogamp.opengl.GLAutoDrawable;
import com.jogamp.opengl.GLContext;
import com.jogamp.opengl.GLEventListener;

import vtk.vtkGenericOpenGLRenderWindow;
import vtk.vtkObject;
import vtk.vtkRenderWindow;
import vtk.rendering.vtkAbstractComponent;
import vtk.rendering.vtkInteractorForwarder;

/**
 * Provide JOGL based rendering component for VTK
 *
 * @author Sebastien Jourdain - sebastien.jourdain@kitware.com
 */
public class vtkAbstractJoglComponent<T extends java.awt.Component> extends vtkAbstractComponent<T> {

  protected T uiComponent;
  protected boolean isWindowCreated;
  protected GLEventListener glEventListener;
  protected vtkGenericOpenGLRenderWindow glRenderWindow;


  public vtkAbstractJoglComponent(vtkRenderWindow renderWindowToUse, T glContainer) {
    super(renderWindowToUse);
    this.isWindowCreated = false;
    this.uiComponent = glContainer;
    this.glRenderWindow = (vtkGenericOpenGLRenderWindow) renderWindowToUse;
    this.glRenderWindow.SetIsDirect(1);
    this.glRenderWindow.SetSupportsOpenGL(1);
    this.glRenderWindow.SetIsCurrent(true);

    // Create the JOGL Event Listener
    this.glEventListener = new GLEventListener() {
      public void init(GLAutoDrawable drawable) {
        vtkAbstractJoglComponent.this.isWindowCreated = true;

        // Make sure the JOGL Context is current
        GLContext ctx = drawable.getContext();
        if (!ctx.isCurrent()) {
          ctx.makeCurrent();
        }

        // Init VTK OpenGL RenderWindow
        vtkAbstractJoglComponent.this.glRenderWindow.SetMapped(1);
        vtkAbstractJoglComponent.this.glRenderWindow.SetPosition(0, 0);
        vtkAbstractJoglComponent.this.setSize(drawable.getSurfaceWidth(), drawable.getSurfaceHeight());
        vtkAbstractJoglComponent.this.glRenderWindow.OpenGLInit();
      }

      public void reshape(GLAutoDrawable drawable, int x, int y, int width, int height) {
        vtkAbstractJoglComponent.this.setSize(width, height);
      }

      public void display(GLAutoDrawable drawable) {
        vtkAbstractJoglComponent.this.inRenderCall = true;
        vtkAbstractJoglComponent.this.glRenderWindow.Render();
        vtkAbstractJoglComponent.this.inRenderCall = false;
      }

      public void dispose(GLAutoDrawable drawable) {
        vtkAbstractJoglComponent.this.Delete();
        vtkObject.JAVA_OBJECT_MANAGER.gc(false);
      }
    };

    // Bind the interactor forwarder
    vtkInteractorForwarder forwarder = this.getInteractorForwarder();
    this.uiComponent.addMouseListener(forwarder);
    this.uiComponent.addMouseMotionListener(forwarder);
    this.uiComponent.addMouseWheelListener(forwarder);
    this.uiComponent.addKeyListener(forwarder);

    // Make sure when VTK internaly request a Render, the Render get
    // properly triggered
    renderWindowToUse.AddObserver("WindowFrameEvent", this, "Render");
    renderWindowToUse.GetInteractor().AddObserver("RenderEvent", this, "Render");
    renderWindowToUse.GetInteractor().SetEnableRender(false);
  }

  public T getComponent() {
    return this.uiComponent;
  }

  /**
   * Render the internal component
   */
  public void Render() {
    // Make sure we can render
    if (!inRenderCall) {
      this.uiComponent.repaint();
    }
  }

  /**
   * @return true if the graphical component has been properly set and
   * operation can be performed on it.
   */
  public boolean isWindowSet() {
    return this.isWindowCreated;
  }
}
