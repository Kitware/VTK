package vtk.rendering;

import java.util.concurrent.locks.ReentrantLock;

import vtk.vtkAxesActor;
import vtk.vtkCamera;
import vtk.vtkGenericRenderWindowInteractor;
import vtk.vtkInteractorStyle;
import vtk.vtkInteractorStyleTrackballCamera;
import vtk.vtkOrientationMarkerWidget;
import vtk.vtkRenderWindow;
import vtk.vtkRenderer;

/**
 * Abstract class that bring most of the VTK logic to any rendering component
 * regardless its origin. (awt, swt, sing, ...)
 *
 * @param <T>
 *            The concrete type of the graphical component that will contains
 *            the vtkRenderWindow.
 *
 * @authors Sebastien Jourdain - sebastien.jourdain@kitware.com, Kitware Inc 2012
 *          Joachim Pouderoux - joachim.pouderoux@kitware.com, Kitware SAS 2012
 * @copyright This work was supported by CEA/CESTA
 *            Commissariat a l'Energie Atomique et aux Energies Alternatives,
 *            15 avenue des Sablieres, CS 60001, 33116 Le Barp, France.
 */
public abstract class vtkAbstractComponent<T> implements vtkComponent<T> {
  protected vtkRenderWindow renderWindow;
  protected vtkRenderer renderer;
  protected vtkCamera camera;
  protected vtkGenericRenderWindowInteractor windowInteractor;
  protected vtkInteractorForwarder eventForwarder;
  protected ReentrantLock lock;
  protected boolean inRenderCall;

  public vtkAbstractComponent() {
    this(new vtkRenderWindow());
  }

  public vtkAbstractComponent(vtkRenderWindow renderWindowToUse) {
    this.inRenderCall = false;
    this.renderWindow = renderWindowToUse;
    this.renderer = new vtkRenderer();
    this.windowInteractor = new vtkGenericRenderWindowInteractor();
    this.lock = new ReentrantLock();

    // Init interactor
    this.windowInteractor.SetRenderWindow(this.renderWindow);
    this.windowInteractor.TimerEventResetsTimerOff();

    this.windowInteractor.SetSize(200, 200);
    this.windowInteractor.ConfigureEvent();

    // Update style
    vtkInteractorStyleTrackballCamera style = new vtkInteractorStyleTrackballCamera();
    this.windowInteractor.SetInteractorStyle(style);

    // Setup event forwarder
    this.eventForwarder = new vtkInteractorForwarder(this);
    this.windowInteractor.AddObserver("CreateTimerEvent", this.eventForwarder, "StartTimer");
    this.windowInteractor.AddObserver("DestroyTimerEvent", this.eventForwarder, "DestroyTimer");

    // Link renderWindow with renderer
    this.renderWindow.AddRenderer(this.renderer);

    // Keep camera around to prevent its creation/deletion in Java world
    this.camera = this.renderer.GetActiveCamera();
  }

  public ReentrantLock getVTKLock() {
    return this.lock;
  }

  public void resetCamera() {
    if (renderer == null) {
      return; // Nothing to do we are deleted...
    }

    try {
      lock.lockInterruptibly();
      renderer.ResetCamera();
    } catch (InterruptedException e) {
      // Nothing that we can do
    } finally {
      this.lock.unlock();
    }
  }

  public void resetCameraClippingRange() {
    if (renderWindow == null) {
      return; // Nothing to do we are deleted...
    }

    try {
      this.lock.lockInterruptibly();
      renderer.ResetCameraClippingRange();
    } catch (InterruptedException e) {
      // Nothing that we can do
    } finally {
      this.lock.unlock();
    }
  }

  public vtkCamera getActiveCamera() {
    return this.camera;
  }

  public vtkRenderer getRenderer() {
    return this.renderer;
  }

  public vtkRenderWindow getRenderWindow() {
    return this.renderWindow;
  }

  public vtkGenericRenderWindowInteractor getRenderWindowInteractor() {
    return this.windowInteractor;
  }

  public void setInteractorStyle(vtkInteractorStyle style) {
    if (this.windowInteractor != null) {
      this.lock.lock();
      this.windowInteractor.SetInteractorStyle(style);
      this.lock.unlock();
    }
  }

  public void setSize(int w, int h) {
    if (renderWindow == null || windowInteractor == null) {
      return; // Nothing to do we are deleted...
    }

    try {
      lock.lockInterruptibly();
      renderWindow.SetSize(w, h);
      windowInteractor.SetSize(w, h);
    } catch (InterruptedException e) {
      // Nothing that we can do
    } finally {
      this.lock.unlock();
    }
  }

  public void Delete() {
    this.lock.lock();
    this.renderer = null;
    this.camera = null;
    this.windowInteractor = null;
    // removing the renderWindow is let to the superclass
    // because in the very special case of an AWT component
    // under Linux, destroying renderWindow crashes.
    this.lock.unlock();
  }

  public vtkInteractorForwarder getInteractorForwarder() {
    return this.eventForwarder;
  }

  public abstract T getComponent();

  /**
   * Generic helper method used to attach orientation axes to a vtkComponent
   *
   * @param vtkComponent<?>
   */
  public static void attachOrientationAxes(vtkComponent<?> component) {
      // only build this once, because it creates its own renderer.
      // Extra renderers causes issues with resetting.
      vtkAxesActor axes = new vtkAxesActor();
      vtkOrientationMarkerWidget axesWidget = new vtkOrientationMarkerWidget();

      axesWidget.SetOutlineColor(0.9300, 0.5700, 0.1300);
      axesWidget.SetOrientationMarker(axes);
      axesWidget.SetInteractor(component.getRenderWindowInteractor());
      axesWidget.SetDefaultRenderer(component.getRenderer());
      axesWidget.SetViewport(0.0, 0.0, .2, .2);
      axesWidget.EnabledOn();
      axesWidget.InteractiveOff();
  }
}
