package vtk.rendering;

import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.MouseWheelEvent;

/**
 * This class implement vtkEventInterceptor with no event interception at all.
 *
 * @see {@link MouseMotionListener} {@link MouseListener} {@link MouseWheelListener}
 *      {@link KeyListener}
 *
 * @author    Sebastien Jourdain - sebastien.jourdain@kitware.com, Kitware Inc 2013
 */

public class vtkAbstractEventInterceptor implements vtkEventInterceptor {

  public boolean keyPressed(KeyEvent e) {
    return false;
  }

  public boolean keyReleased(KeyEvent e) {
    return false;
  }

  public boolean keyTyped(KeyEvent e) {
    return false;
  }

  public boolean mouseDragged(MouseEvent e) {
    return false;
  }

  public boolean mouseMoved(MouseEvent e) {
    return false;
  }

  public boolean mouseClicked(MouseEvent e) {
    return false;
  }

  public boolean mouseEntered(MouseEvent e) {
    return false;
  }

  public boolean mouseExited(MouseEvent e) {
    return false;
  }

  public boolean mousePressed(MouseEvent e) {
    return false;
  }

  public boolean mouseReleased(MouseEvent e) {
    return false;
  }

  public boolean mouseWheelMoved(MouseWheelEvent e) {
    return false;
  }
}
