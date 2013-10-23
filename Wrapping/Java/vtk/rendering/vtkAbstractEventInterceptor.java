package vtk.rendering;

import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;

/**
 * This class implement vtkEventInterceptor with no event interception at all.
 *
 * @see {@link MouseMotionListener} {@link MouseListener} {@link KeyListener}
 *
 * @author    Sebastien Jourdain - sebastien.jourdain@kitware.com, Kitware Inc 2013
 */

public class vtkAbstractEventInterceptor implements vtkEventInterceptor {

	@Override
	public boolean keyPressed(KeyEvent e) {
		return false;
	}

	@Override
	public boolean keyReleased(KeyEvent e) {
		return false;
	}

	@Override
	public boolean keyTyped(KeyEvent e) {
		return false;
	}

	@Override
	public boolean mouseDragged(MouseEvent e) {
		return false;
	}

	@Override
	public boolean mouseMoved(MouseEvent e) {
		return false;
	}

	@Override
	public boolean mouseClicked(MouseEvent e) {
		return false;
	}

	@Override
	public boolean mouseEntered(MouseEvent e) {
		return false;
	}

	@Override
	public boolean mouseExited(MouseEvent e) {
		return false;
	}

	@Override
	public boolean mousePressed(MouseEvent e) {
		return false;
	}

	@Override
	public boolean mouseReleased(MouseEvent e) {
		return false;
	}
}
