package vtk.rendering;

import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.MouseWheelEvent;
import java.awt.event.MouseWheelListener;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

/**
 * Helper class used to implement all Mouse/Key Java listener and convert them
 * into the vtkInteractor proper event.
 *
 * @authors   Sebastien Jourdain - sebastien.jourdain@kitware.com, Kitware Inc 2012
 *            Joachim Pouderoux - joachim.pouderoux@kitware.com, Kitware SAS 2012
 * @copyright This work was supported by CEA/CESTA
 *            Commissariat a l'Energie Atomique et aux Energies Alternatives,
 *            15 avenue des Sablieres, CS 60001, 33116 Le Barp, France.
 */
public class vtkInteractorForwarder implements MouseListener, MouseMotionListener, MouseWheelListener, KeyListener {
  final public static int MOUSE_BUTTON_1 = 1;
  final public static int MOUSE_BUTTON_2 = 2;
  final public static int MOUSE_BUTTON_3 = 4;
  final public static int MOUSE_MODIFIER_SHIFT = 1;
  final public static int MOUSE_MODIFIER_CTRL = 2;
  final public static int MOUSE_MODIFIER_ALT = 4;

  private int lastX;
  private int lastY;
  private int ctrlPressed;
  private int shiftPressed;
  private vtkComponent<?> component;
  private double updateRate, updateRateRelease;
  private ScheduledExecutorService scheduler;
  private Runnable eventTick;
  private vtkEventInterceptor eventInterceptor;

  public vtkInteractorForwarder(vtkComponent<?> component) {
    this.component = component;
    this.lastX = this.lastY = this.ctrlPressed = 0;
    this.updateRate = 5.0;
    this.updateRateRelease = 0.01;

    this.eventTick = new Runnable() {

      public void run() {
        vtkInteractorForwarder.this.component.getVTKLock().lock();
        vtkInteractorForwarder.this.component.getRenderWindowInteractor().TimerEvent();
        vtkInteractorForwarder.this.component.getVTKLock().unlock();
      }
    };

    // Schedule time events
    this.scheduler = Executors.newSingleThreadScheduledExecutor();
  }

  /**
   * Provide a custom event interceptor
   *
   * @param eventInterceptor
   */
  public void setEventInterceptor(vtkEventInterceptor eventInterceptor) {
    this.eventInterceptor = eventInterceptor;
  }

  /**
   * @return the custom event interceptor if any otherwise return null
   */
  public vtkEventInterceptor getEventInterceptor() {
    return eventInterceptor;
  }

  /**
   * Method called by VTK to start a timer
   */
  public void StartTimer() {
    this.scheduler.scheduleAtFixedRate(this.eventTick, 10, 10, TimeUnit.MILLISECONDS);
  }

  /**
   * Method called by VTK to stop a timer
   */
  public void DestroyTimer() {
    this.scheduler.shutdown();
  }

  /**
   * Allow the user to change the update rate
   *
   * @param updateRate
   * @param updateRateRelease
   */
  public void setUpdateRate(double updateRate, double updateRateRelease) {
    this.updateRate = updateRate;
    this.updateRateRelease = updateRateRelease;
  }

  /**
   * @return the update rate that is currently used
   */
  public double getUpdateRate() {
    return updateRate;
  }

  /**
   * @return the update rate after release that is currently used
   */
  public double getUpdateRateRelease() {
    return updateRateRelease;
  }

  public void mousePressed(MouseEvent e) {
    if (component == null || component.getRenderer() == null) {
      return;
    }

    // Allow user to override some behavior
    if (this.eventInterceptor != null && this.eventInterceptor.mousePressed(e)) {
      return;
    }

    try {
      component.getVTKLock().lockInterruptibly();
      component.getRenderWindow().SetDesiredUpdateRate(this.updateRate);
      lastX = e.getX();
      lastY = e.getY();
      ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
      shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;
      component.getRenderWindowInteractor().SetEventInformationFlipY(lastX, lastY, ctrlPressed, shiftPressed, '0', 0, "0");
      switch (e.getButton()) {
      case MouseEvent.BUTTON3:
        component.getRenderWindowInteractor().RightButtonPressEvent();
        break;
      case MouseEvent.BUTTON2:
        component.getRenderWindowInteractor().MiddleButtonPressEvent();
        break;
      case MouseEvent.BUTTON1:
      default:
        component.getRenderWindowInteractor().LeftButtonPressEvent();
        break;
      }
    } catch (InterruptedException interupt) {
      // Nothing to do
    } finally {
      component.getVTKLock().unlock();
    }
  }

  public void mouseReleased(MouseEvent e) {
    if (component == null || component.getRenderer() == null) {
      return;
    }

    // Allow user to override some behavior
    if (this.eventInterceptor != null && this.eventInterceptor.mouseReleased(e)) {
      return;
    }

    try {
      component.getVTKLock().lockInterruptibly();
      component.getRenderWindow().SetDesiredUpdateRate(this.updateRateRelease);
      lastX = e.getX();
      lastY = e.getY();
      ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
      shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;
      component.getRenderWindowInteractor().SetEventInformationFlipY(lastX, lastY, ctrlPressed, shiftPressed, '0', 0, "0");
      switch (e.getButton()) {
      case MouseEvent.BUTTON3:
        component.getRenderWindowInteractor().RightButtonReleaseEvent();
        break;
      case MouseEvent.BUTTON2:
        component.getRenderWindowInteractor().MiddleButtonReleaseEvent();
        break;
      case MouseEvent.BUTTON1:
      default:
        component.getRenderWindowInteractor().LeftButtonReleaseEvent();
        break;
      }
    } catch (InterruptedException interupt) {
      // Nothing to do
    } finally {
      component.getVTKLock().unlock();
    }
  }

  public void mouseMoved(MouseEvent e) {
    if (component == null || component.getRenderer() == null) {
      return;
    }

    // Allow user to override some behavior
    if (this.eventInterceptor != null && this.eventInterceptor.mouseMoved(e)) {
      return;
    }

    try {
      component.getVTKLock().lockInterruptibly();
      component.getRenderWindow().SetDesiredUpdateRate(this.updateRateRelease);
      lastX = e.getX();
      lastY = e.getY();
      ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
      shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;
      component.getRenderWindowInteractor().SetEventInformationFlipY(lastX, lastY, ctrlPressed, shiftPressed, '0', 0, "0");
      component.getRenderWindowInteractor().MouseMoveEvent();
    } catch (InterruptedException interupt) {
      // Nothing to do
    } finally {
      component.getVTKLock().unlock();
    }
  }

  public void mouseDragged(MouseEvent e) {
    // Allow user to override some behavior
    if (this.eventInterceptor != null && this.eventInterceptor.mouseDragged(e)) {
      return;
    }

    this.mouseMoved(e);
  }

  public void mouseEntered(MouseEvent e) {
    if (component == null || component.getRenderer() == null) {
      return;
    }

    // Allow user to override some behavior
    if (this.eventInterceptor != null && this.eventInterceptor.mouseEntered(e)) {
      return;
    }

    try {
      component.getVTKLock().lockInterruptibly();
      component.getRenderWindow().SetDesiredUpdateRate(this.updateRateRelease);
      lastX = e.getX();
      lastY = e.getY();
      ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
      shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;
      component.getRenderWindowInteractor().SetEventInformationFlipY(lastX, lastY, ctrlPressed, shiftPressed, '0', 0, "0");
      component.getRenderWindowInteractor().EnterEvent();
    } catch (InterruptedException interupt) {
      // Nothing to do
    } finally {
      component.getVTKLock().unlock();
    }
  }

  public void mouseExited(MouseEvent e) {
    if (component == null || component.getRenderer() == null) {
      return;
    }

    // Allow user to override some behavior
    if (this.eventInterceptor != null && this.eventInterceptor.mouseExited(e)) {
      return;
    }

    try {
      component.getVTKLock().lockInterruptibly();
      component.getRenderWindow().SetDesiredUpdateRate(this.updateRateRelease);
      lastX = e.getX();
      lastY = e.getY();
      ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
      shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;
      component.getRenderWindowInteractor().SetEventInformationFlipY(lastX, lastY, ctrlPressed, shiftPressed, '0', 0, "0");
      component.getRenderWindowInteractor().LeaveEvent();
    } catch (InterruptedException interupt) {
      // Nothing to do
    } finally {
      component.getVTKLock().unlock();
    }
  }

  public void mouseClicked(MouseEvent e) {
    // Allow user to override some behavior
    if (this.eventInterceptor != null && this.eventInterceptor.mouseClicked(e)) {
      return;
    }
  }

  public void mouseWheelMoved(MouseWheelEvent e) {
    if (component == null || component.getRenderer() == null) {
      return;
    }

    // Allow user to override some behavior
    if (this.eventInterceptor != null && this.eventInterceptor.mouseWheelMoved(e)) {
      return;
    }

    try {
      component.getVTKLock().lockInterruptibly();
      component.getRenderWindow().SetDesiredUpdateRate(this.updateRateRelease);
      lastX = e.getX();
      lastY = e.getY();
      ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
      shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;
      if (e.getWheelRotation() > 0) {
        component.getRenderWindowInteractor().SetEventInformationFlipY(lastX, lastY, ctrlPressed, shiftPressed, '0', 0, "0");
        component.getRenderWindowInteractor().MouseWheelBackwardEvent();
      }
      else if (e.getWheelRotation() < 0) {
        component.getRenderWindowInteractor().SetEventInformationFlipY(lastX, lastY, ctrlPressed, shiftPressed, '0', 0, "0");
        component.getRenderWindowInteractor().MouseWheelForwardEvent();
      }
    } catch (InterruptedException interupt) {
      // Nothing to do
    } finally {
      component.getVTKLock().unlock();
    }
  }

  public void keyPressed(KeyEvent e) {
    if (component == null || component.getRenderer() == null) {
      return;
    }

    // Allow user to override some behavior
    if (this.eventInterceptor != null && this.eventInterceptor.keyPressed(e)) {
      return;
    }

    try {
      component.getVTKLock().lockInterruptibly();
      component.getRenderWindow().SetDesiredUpdateRate(this.updateRateRelease);
      ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
      shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;
      char keyChar = e.getKeyChar();
      component.getRenderWindowInteractor().SetEventInformationFlipY(lastX, lastY, ctrlPressed, shiftPressed, keyChar, 0, String.valueOf(keyChar));
      component.getRenderWindowInteractor().KeyPressEvent();
      component.getRenderWindowInteractor().CharEvent();
    } catch (InterruptedException interupt) {
      // Nothing to do
    } finally {
      component.getVTKLock().unlock();
    }
  }

  public void keyReleased(KeyEvent e) {
    // Allow user to override some behavior
    if (this.eventInterceptor != null && this.eventInterceptor.keyReleased(e)) {
      return;
    }
  }

  public void keyTyped(KeyEvent e) {
    // Allow user to override some behavior
    if (this.eventInterceptor != null && this.eventInterceptor.keyTyped(e)) {
      return;
    }
  }
}
