package vtk.rendering.jogl;

import javax.media.opengl.GLCapabilities;
import javax.media.opengl.GLProfile;
import javax.media.opengl.awt.GLCanvas;

import vtk.vtkGenericOpenGLRenderWindow;
import vtk.vtkRenderWindow;

public class vtkJoglCanvasComponent extends vtkAbstractJoglComponent<GLCanvas> {

  public vtkJoglCanvasComponent() {
    this(new vtkGenericOpenGLRenderWindow());
  }

  public vtkJoglCanvasComponent(vtkRenderWindow renderWindow) {
    this(renderWindow, new GLCapabilities(GLProfile.getDefault()));
  }

  public vtkJoglCanvasComponent(vtkRenderWindow renderWindow, GLCapabilities capabilities) {
    super(renderWindow, new GLCanvas(capabilities));
    this.getComponent().addGLEventListener(this.glEventListener);
  }
}
