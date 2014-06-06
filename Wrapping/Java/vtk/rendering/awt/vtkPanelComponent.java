package vtk.rendering.awt;

import java.awt.event.MouseEvent;
import java.awt.event.MouseMotionAdapter;
import java.util.concurrent.locks.ReentrantLock;

import vtk.vtkCamera;
import vtk.vtkGenericRenderWindowInteractor;
import vtk.vtkInteractorStyle;
import vtk.vtkInteractorStyleTrackballCamera;
import vtk.vtkPanel;
import vtk.vtkRenderWindow;
import vtk.vtkRenderer;
import vtk.rendering.vtkComponent;
import vtk.rendering.vtkInteractorForwarder;

/**
 * Provide AWT based vtk rendering component using the vtkPanel class
 * while exposing everything as a new rendering component.
 *
 * @author Sebastien Jourdain - sebastien.jourdain@kitware.com
 *
 * Notes: This class should be replaced down the road by the vtkAwtComponent
 *        but on some platform such as Windows the vtkAwtComponent
 *        produce a runtime error regarding invalid pixel format while
 *        the vtkPanelComponent which use vtkPanel works fine.
 *        For now, this class provide a good substitute with just a minor overhead.
 */

public class vtkPanelComponent implements vtkComponent<vtkPanel> {
  protected vtkPanel panel;
  protected ReentrantLock lock;
  protected vtkGenericRenderWindowInteractor windowInteractor;
  protected vtkInteractorForwarder eventForwarder;

  public vtkPanelComponent() {
    this.panel = new vtkPanel();
    this.lock = new ReentrantLock();

    // Init interactor
    this.windowInteractor = new vtkGenericRenderWindowInteractor();
    this.windowInteractor.SetRenderWindow(this.panel.GetRenderWindow());
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

    // Remove unwanted listeners
    this.panel.removeKeyListener(this.panel);
    this.panel.removeMouseListener(this.panel);
    this.panel.removeMouseMotionListener(this.panel);
    this.panel.removeMouseWheelListener(this.panel);

    // Add mouse listener that update interactor
    this.panel.addMouseListener(this.eventForwarder);
    this.panel.addMouseMotionListener(this.eventForwarder);
    this.panel.addMouseWheelListener(this.eventForwarder);

    // Make sure we update the light position when interacting
    this.panel.addMouseMotionListener(new MouseMotionAdapter() {
      public void mouseDragged(MouseEvent e) {
        panel.UpdateLight();
      }
    });
  }

  public void resetCamera() {
    this.panel.resetCamera();
  }

  public void resetCameraClippingRange() {
    this.panel.resetCameraClippingRange();
  }

  public vtkCamera getActiveCamera() {
    return this.panel.GetRenderer().GetActiveCamera();
  }

  public vtkRenderer getRenderer() {
    return this.panel.GetRenderer();
  }

  public vtkRenderWindow getRenderWindow() {
    return this.panel.GetRenderWindow();
  }

  public vtkGenericRenderWindowInteractor getRenderWindowInteractor() {
    return this.windowInteractor;
  }

  public void setInteractorStyle(vtkInteractorStyle style) {
    this.getRenderWindowInteractor().SetInteractorStyle(style);
  }

  public void setSize(int w, int h) {
    this.panel.setSize(w, h);
    this.getRenderWindowInteractor().SetSize(w, h);
  }

  public vtkPanel getComponent() {
    return this.panel;
  }

  public void Delete() {
    this.panel.Delete();
  }

  public void Render() {
    this.panel.Render();
  }

  public vtkInteractorForwarder getInteractorForwarder() {
    return this.eventForwarder;
  }

  public ReentrantLock getVTKLock() {
    return this.lock;
  }
}
