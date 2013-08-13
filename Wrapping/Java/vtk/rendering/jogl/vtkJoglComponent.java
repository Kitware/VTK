package vtk.rendering.jogl;

import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.GLCapabilities;
import javax.media.opengl.GLContext;
import javax.media.opengl.GLEventListener;
import javax.media.opengl.GLProfile;
import javax.media.opengl.awt.GLCanvas;

import vtk.vtkGenericOpenGLRenderWindow;
import vtk.vtkObject;
import vtk.vtkOpenGLRenderWindow;
import vtk.vtkRenderWindow;
import vtk.rendering.vtkAbstractComponent;
import vtk.rendering.vtkInteractorForwarder;

/**
 * Provide JOGL based rendering component for VTK
 *
 * @author Sebastien Jourdain - sebastien.jourdain@kitware.com
 */
public class vtkJoglComponent extends vtkAbstractComponent<GLCanvas> {

    protected GLCanvas uiComponent;
    protected boolean isWindowCreated;
    protected GLEventListener glEventListener;
    protected vtkGenericOpenGLRenderWindow glRenderWindow;

    public vtkJoglComponent() {
        this(new vtkGenericOpenGLRenderWindow());
    }

    public vtkJoglComponent(vtkRenderWindow renderWindowToUse) {
        super(renderWindowToUse);
        this.isWindowCreated = false;
        this.uiComponent = new GLCanvas(new GLCapabilities(GLProfile.getDefault()));
        this.glRenderWindow = (vtkGenericOpenGLRenderWindow) renderWindowToUse;
        this.glRenderWindow.SetIsDirect(1);
        this.glRenderWindow.SetSupportsOpenGL(1);
        this.glRenderWindow.SetIsCurrent(true);

        // Create the JOGL Event Listener
        this.glEventListener = new GLEventListener() {
            public void init(GLAutoDrawable drawable) {
                vtkJoglComponent.this.isWindowCreated = true;

                // Make sure the JOGL Context is current
                GLContext ctx = drawable.getContext();
                if (!ctx.isCurrent()) {
                    ctx.makeCurrent();
                }

                // Init VTK OpenGL RenderWindow
                vtkJoglComponent.this.glRenderWindow.SetMapped(1);
                vtkJoglComponent.this.glRenderWindow.SetPosition(0, 0);
                vtkJoglComponent.this.setSize(drawable.getWidth(), drawable.getHeight());
                vtkJoglComponent.this.glRenderWindow.OpenGLInit();
            }

            public void reshape(GLAutoDrawable drawable, int x, int y, int width, int height) {
                vtkJoglComponent.this.setSize(width, height);
            }

            public void display(GLAutoDrawable drawable) {
                vtkJoglComponent.this.inRenderCall = true;
                vtkJoglComponent.this.glRenderWindow.Render();
                vtkJoglComponent.this.inRenderCall = false;
            }

            public void dispose(GLAutoDrawable drawable) {
                vtkJoglComponent.this.Delete();
                vtkObject.JAVA_OBJECT_MANAGER.gc(false);
            }
        };
        this.uiComponent.addGLEventListener(this.glEventListener);

        // Bind the interactor forwarder
        vtkInteractorForwarder forwarder = this.getInteractorForwarder();
        this.uiComponent.addMouseListener(forwarder);
        this.uiComponent.addMouseMotionListener(forwarder);
        this.uiComponent.addKeyListener(forwarder);

        // Make sure when VTK internaly request a Render, the Render get
        // properly triggered
        renderWindowToUse.AddObserver("WindowFrameEvent", this, "Render");
    }

    public GLCanvas getComponent() {
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
