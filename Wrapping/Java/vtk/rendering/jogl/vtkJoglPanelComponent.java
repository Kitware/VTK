package vtk.rendering.jogl;

import com.jogamp.opengl.GLCapabilities;
import com.jogamp.opengl.GLProfile;
import com.jogamp.opengl.awt.GLJPanel;

import vtk.vtkGenericOpenGLRenderWindow;
import vtk.vtkRenderWindow;

public class vtkJoglPanelComponent extends vtkAbstractJoglComponent<GLJPanel> {

  public vtkJoglPanelComponent() {
    this(new vtkGenericOpenGLRenderWindow());
  }

  public vtkJoglPanelComponent(vtkRenderWindow renderWindow) {
    this(renderWindow, new GLCapabilities(GLProfile.getDefault()));
  }

  public vtkJoglPanelComponent(vtkRenderWindow renderWindow, GLCapabilities capabilities) {
    super(renderWindow, new GLJPanel(capabilities));
    this.getComponent().addGLEventListener(this.glEventListener);
  }
}
