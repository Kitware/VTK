package vtk;

import vtk.*;
import java.awt.*;
import java.awt.event.*;
import java.lang.Math;
import sun.awt.*;
import java.beans.*;

public class vtkPanelBeans extends Canvas implements MouseListener, MouseMotionListener, KeyListener
{
  protected vtkRenderWindow rw = new vtkRenderWindow();     
  protected vtkRenderer ren = new vtkRenderer();
  protected vtkCamera cam = null;
  protected vtkLight lgt = new vtkLight();
  protected int lastX;
  protected int lastY;
  int windowset = 0;
  int LightFollowCamera = 1;
  int InteractionMode = 1;
  boolean rendering = false;
  
  static { System.loadLibrary("vtkJava"); }

  public void setActor(vtkActor id0)
    { 
    // add the actor if it isnt present
    if (ren.getActors().isItemPresent(id0) == 0)
      {
      ren.addActor(id0); 
      this.Render();
      }
    }

  public void setVolume(vtkVolume id0)
    { 
    // add the actor if it isnt present
    if (ren.getVolumes().isItemPresent(id0) == 0)
      {
      ren.addVolume(id0); 
      this.Render();
      }
    }

public vtkPanelBeans()
  {
    rw.addRenderer(ren);
    addMouseListener(this);
    addMouseMotionListener(this);
    addKeyListener(this);
    super.setSize(200,200);
    rw.setSize(200,200);
  }

public int getWindowID() 
  {
    DrawingSurfaceInfo surfaceInfo =
      ((DrawingSurface)this.getPeer()).getDrawingSurfaceInfo();
    surfaceInfo.lock();

// MS windows support inthe next three lines 	
//     Win32DrawingSurface wds =
//       (Win32DrawingSurface)surfaceInfo.getSurface();
//     int hWnd = wds.getHWnd();


// X windows support in the next three lines
    X11DrawingSurface wds =
      (X11DrawingSurface)surfaceInfo.getSurface();
    int hWnd = wds.getDrawable();


    surfaceInfo.unlock();
    return hWnd;
  }
 
public String getWindowInfo()
  {
    String result;
    result = "" + this.getWindowID();
    return result; 
  }

public void setSize(int x, int y)
  {
    super.setSize(x,y);
    rw.setSize(x,y);
  }

public vtkRenderer getRenderer()
  {
    return ren;
  }

public vtkRenderWindow getRenderWindow()
  {
    return rw;
  }

public synchronized void Render() 
  {
    if (!rendering)
      {
      rendering = true;
      if (ren.visibleActorCount() == 0) return;
      if (rw != null)
	{
	if (windowset == 0)
	  { 
	  // set the window id and the active camera
	  rw.setWindowInfo(this.getWindowInfo());
	  cam = ren.getActiveCamera();
	  ren.addLight(lgt);
	  lgt.setPosition(cam.getPosition());
	  lgt.setFocalPoint(cam.getFocalPoint());
	  windowset = 1;
	  }
	rw.render();
	rendering = false;
	}
      }
  }

public void paint(Graphics g)
  {
  this.Render();
  }
        
public void LightFollowCameraOn()
  {
    this.LightFollowCamera = 1;
  }

public void LightFollowCameraOff()
  {
    this.LightFollowCamera = 0;
  }

public void InteractionModeRotate()
  {
    this.InteractionMode = 1;
  }

public void InteractionModeTranslate()
  {
    this.InteractionMode = 2;
  }

public void InteractionModeZoom()
  {
    this.InteractionMode = 3;
  }

public void UpdateLight()
  {
    lgt.setPosition(cam.getPosition());
    lgt.setFocalPoint(cam.getFocalPoint());
  }

public void mouseClicked(MouseEvent e) {

}

public void mousePressed(MouseEvent e)
  {

    if (ren.visibleActorCount() == 0) return;
    rw.setDesiredUpdateRate(5.0);
    lastX = e.getX();
    lastY = e.getY();
    if ((e.getModifiers()==InputEvent.BUTTON2_MASK) ||
	     (e.getModifiers()==(InputEvent.BUTTON1_MASK | InputEvent.SHIFT_MASK)))
      {
      InteractionModeTranslate();
      }
    else if (e.getModifiers()==InputEvent.BUTTON3_MASK)
      {
      InteractionModeZoom();
      }
    else 
      {
      InteractionModeRotate();
      }
  }

public void mouseReleased(MouseEvent e)
  {
    rw.setDesiredUpdateRate(0.01);
  }

public void mouseEntered(MouseEvent e) {
	this.requestFocus();
	}

public void mouseExited(MouseEvent e) {}

public void mouseMoved(MouseEvent e) {
    lastX = e.getX();
    lastY = e.getY();
}


public void mouseDragged(MouseEvent e)
  {
    if (ren.visibleActorCount() == 0) return;
    int x = e.getX();
    int y = e.getY();
    // rotate
    if (this.InteractionMode == 1)
      {
      cam.azimuth(lastX - x);
      cam.elevation(y - lastY);
      cam.orthogonalizeViewUp();
      if (this.LightFollowCamera == 1)
	{
	lgt.setPosition(cam.getPosition());
	lgt.setFocalPoint(cam.getFocalPoint());
	}
      }
    // translate
    if (this.InteractionMode == 2)
      {
      double  FPoint[];
      double  PPoint[];
      double  APoint[] = new double[3];
      double  RPoint[];
      double focalDepth;

      // get the current focal point and position
      FPoint = cam.getFocalPoint();
      PPoint = cam.getPosition();

      // calculate the focal depth since we'll be using it a lot
      ren.setWorldPoint(FPoint[0],FPoint[1],FPoint[2],1.0);
      ren.worldToDisplay();
      focalDepth = ren.getDisplayPoint()[2];

      APoint[0] = rw.getSize()[0]/2.0 + (x - lastX);
      APoint[1] = rw.getSize()[1]/2.0 - (y - lastY);
      APoint[2] = focalDepth;
      ren.setDisplayPoint(APoint);
      ren.displayToWorld();
      RPoint = ren.getWorldPoint();
      if (RPoint[3] != 0.0)
	{
	RPoint[0] = RPoint[0]/RPoint[3];
	RPoint[1] = RPoint[1]/RPoint[3];
	RPoint[2] = RPoint[2]/RPoint[3];
	}

      /*
       * Compute a translation vector, moving everything 1/2 
       * the distance to the cursor. (Arbitrary scale factor)
       */
      cam.setFocalPoint(
			(FPoint[0]-RPoint[0])/2.0 + FPoint[0],
			(FPoint[1]-RPoint[1])/2.0 + FPoint[1],
			(FPoint[2]-RPoint[2])/2.0 + FPoint[2]);
      cam.setPosition(
		      (FPoint[0]-RPoint[0])/2.0 + PPoint[0],
		      (FPoint[1]-RPoint[1])/2.0 + PPoint[1],
		      (FPoint[2]-RPoint[2])/2.0 + PPoint[2]);
      }
    // zoom
    if (this.InteractionMode == 3)
      {
      double zoomFactor;
      double clippingRange[];

      zoomFactor = Math.pow(1.02,(y - lastY));
      if (cam.getParallelProjection() == 1)
	{
	cam.setParallelScale(cam.getParallelScale()/zoomFactor);
	}
      else
	{
	clippingRange = cam.getClippingRange();
	cam.setClippingRange(clippingRange[0]/zoomFactor,
			     clippingRange[1]/zoomFactor);
	cam.dolly(zoomFactor);
	}
      }
    lastX = x;
    lastY = y;
    this.Render();
  }

public void keyTyped(KeyEvent e) {}
        
public void keyPressed(KeyEvent e)
  {
    if (ren.visibleActorCount() == 0) return;
    char keyChar = e.getKeyChar();

    if ('r' == keyChar)
      {
      ren.resetCamera();
      this.Render();
      }
    if ('u' == keyChar)
      {
      vtkPicker picker = new vtkPicker();
      picker.pick(lastX,700 - lastY,0.0,ren);

      }
    if ('w' == keyChar)
      {
      vtkActorCollection ac;
      vtkActor anActor;
      vtkActor aPart;
      int i, j;
          
      ac = ren.getActors();
      ac.initTraversal();
      for (i = 0; i < ac.getNumberOfItems(); i++)
	{
	anActor = ac.getNextItem();
	anActor.initPartTraversal();
	for (j = 0; j < anActor.getNumberOfParts(); j++)
	  { 
	  aPart = anActor.getNextPart();
	  aPart.getProperty().setRepresentationToWireframe();
	  }
	}
      this.Render();
      }
    if ('s' == keyChar)
      {
      vtkActorCollection ac;
      vtkActor anActor;
      vtkActor aPart;
      int i, j;
          
      ac = ren.getActors();
      ac.initTraversal();
      for (i = 0; i < ac.getNumberOfItems(); i++)
	{
	anActor = ac.getNextItem();
	anActor.initPartTraversal();
	for (j = 0; j < anActor.getNumberOfParts(); j++)
	  { 
	  aPart = anActor.getNextPart();
	  aPart.getProperty().setRepresentationToSurface();
	  }
	}
      this.Render();
      }
  }

  public void addPropertyChangeListener(PropertyChangeListener l)
    {
    changes.addPropertyChangeListener(l);
    }
  public void removePropertyChangeListener(PropertyChangeListener l)
    {
    changes.removePropertyChangeListener(l);
    }
  protected PropertyChangeSupport changes = new PropertyChangeSupport(this);

  public void keyReleased(KeyEvent e) {}

}
