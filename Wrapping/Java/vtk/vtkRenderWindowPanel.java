package vtk;

import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.awt.event.MouseEvent;

// vtkRenderWindowPanel is a vtkCanvas which allows additional
// vtkRenderers to be added.  vtkPanel and vtkCanvas force you to
// add actors to the internal vtkRenderer.  vtkRenderWindowPanel 
// always renders, even if the internal renderer has no visible actors.
public class vtkRenderWindowPanel extends vtkCanvas {
  public vtkRenderWindowPanel() {
    cam = new vtkCamera();
    lgt = new vtkLight();
  }

  public synchronized void Render() {
    if (!rendering) {
      rendering = true;
      if (rw != null) {
        if (windowset == 0) {
          // set the window id and the active camera
          RenderCreate(rw);
          Lock();
          rw.SetSize(getWidth(), getHeight());
          UnLock();
          windowset = 1;
          // notify observers that we have a renderwindow created
          // windowSetObservable.notifyObservers();
        }
        Lock();
        rw.Render();
        UnLock();
      }
      rendering = false;
    }
  }

  public void mousePressed(MouseEvent e) {
    Lock();
    rw.SetDesiredUpdateRate(5.0);
    lastX = e.getX();
    lastY = e.getY();

    ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1
        : 0;
    shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1
        : 0;

    iren.SetEventInformationFlipY(e.getX(), e.getY(), ctrlPressed,
        shiftPressed, '0', 0, "0");

    if ((e.getModifiers() & InputEvent.BUTTON1_MASK) == InputEvent.BUTTON1_MASK) {
      iren.LeftButtonPressEvent();
    }

    else if ((e.getModifiers() & InputEvent.BUTTON2_MASK) == InputEvent.BUTTON2_MASK) {
      iren.MiddleButtonPressEvent();
    }

    else if ((e.getModifiers() & InputEvent.BUTTON3_MASK) == InputEvent.BUTTON3_MASK) {
      iren.RightButtonPressEvent();
    }

    UnLock();
  }

  public void mouseDragged(MouseEvent e) {
    ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1
        : 0;
    shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1
        : 0;

    iren.SetEventInformationFlipY(e.getX(), e.getY(), ctrlPressed,
        shiftPressed, '0', 0, "0");

    Lock();
    iren.MouseMoveEvent();
    UnLock();
  }

  public void keyPressed(KeyEvent e) {
    char keyChar = e.getKeyChar();

    ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1
        : 0;
    shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1
        : 0;

    iren.SetEventInformationFlipY(lastX, lastY, ctrlPressed, shiftPressed,
        keyChar, 0, String.valueOf(keyChar));

    Lock();
    iren.KeyPressEvent();
    iren.CharEvent();
    UnLock();
  }
}
