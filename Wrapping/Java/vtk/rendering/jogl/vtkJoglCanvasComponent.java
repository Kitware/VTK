package vtk.rendering.jogl;

import com.jogamp.opengl.GLCapabilities;
import com.jogamp.opengl.GLProfile;
import com.jogamp.opengl.awt.GLCanvas;

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
