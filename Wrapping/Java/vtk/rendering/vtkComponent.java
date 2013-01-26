package vtk.rendering;

import java.util.concurrent.locks.ReentrantLock;

import vtk.vtkCamera;
import vtk.vtkGenericRenderWindowInteractor;
import vtk.vtkInteractorStyle;
import vtk.vtkRenderWindow;
import vtk.vtkRenderer;

/**
 * Generic API for any new VTK based graphical components.
 *
 * @param <T>
 *            The concrete type of the graphical component that will contains
 *            the vtkRenderWindow.
 *
 * @author Sebastien Jourdain - sebastien.jourdain@kitware.com
 */

public interface vtkComponent<T> {

  /**
   * @return the lock that is used to prevent concurrency inside this
   *         rendering component. This lock can also be used outside to make
   *         sure any VTK processing happen in a save manner.
   */
  ReentrantLock getVTKLock();

  /**
   * Adjust the camera position so any object in the scene will be fully seen.
   */
  void resetCamera();

  /**
   * Update the clipping range of the camera
   */
  void resetCameraClippingRange();

  /**
   * @return the active camera of the renderer
   */
  vtkCamera getActiveCamera();

  /**
   * @return a reference to the Renderer used internally
   */
  vtkRenderer getRenderer();

  /**
   * Useful for screen capture or exporter.
   *
   * @return a reference to the RenderWindow used internally
   */
  vtkRenderWindow getRenderWindow();

  /**
   * vtkWindowInteractor is useful if you want to attach 3DWidget into your
   * view.
   *
   * @return a reference to the vtkWindowInteractor used internally
   */
  vtkGenericRenderWindowInteractor getRenderWindowInteractor();

  /**
   * Shortcut method to bind an vtkInteractorStyle to our interactor.
   *
   * @param style
   */
  void setInteractorStyle(vtkInteractorStyle style);

  /**
   * Update width and height of the given component
   *
   * @param w
   * @param h
   */
  void setSize(int w, int h);

  /**
   * @return the concrete implementation of the graphical container such as
   *         java.awt.Canvas / java.swing.JComponent /
   *         org.eclipse.swt.opengl.GLCanvas
   */
  T getComponent();

  /**
   * Remove any reference from Java to vtkObject to allow the VTK Garbage
   * collector to free any remaining memory. This is specially needed for
   * internal hidden reference to vtkObject.
   */
  void Delete();

  /**
   * Request a render.
   */
  void Render();

  /**
   * @return the vtkInteractor Java event converter.
   */
  vtkInteractorForwarder getInteractorForwarder();
}
