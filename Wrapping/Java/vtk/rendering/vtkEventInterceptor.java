package vtk.rendering;

import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;

/**
 * This interface defines what should be implemented to intercept interaction
 * events and create custom behavior.
 *
 * @see {@link MouseMotionListener} {@link MouseListener} {@link KeyListener}
 *
 * @author    Sebastien Jourdain - sebastien.jourdain@kitware.com, Kitware Inc 2012
 * @copyright This work was supported by CEA/CESTA
 *            Commissariat a l'Energie Atomique et aux Energies Alternatives,
 *            15 avenue des Sablieres, CS 60001, 33116 Le Barp, France.
 */
public interface vtkEventInterceptor {

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the vtkInteractor
   */
  boolean keyPressed(KeyEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the vtkInteractor
   */
  boolean keyReleased(KeyEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the vtkInteractor
   */
  boolean keyTyped(KeyEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the vtkInteractor
   */
  boolean mouseDragged(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the vtkInteractor
   */
  boolean mouseMoved(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the vtkInteractor
   */
  boolean mouseClicked(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the vtkInteractor
   */
  boolean mouseEntered(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the vtkInteractor
   */
  boolean mouseExited(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the vtkInteractor
   */
  boolean mousePressed(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the vtkInteractor
   */
  boolean mouseReleased(MouseEvent e);
}
