package vtk;

import vtk.*;
import java.awt.*;
import java.awt.event.*;
import java.lang.Math;
import sun.awt.*;

public class vtkPanel extends Canvas implements MouseListener, MouseMotionListener, KeyListener
{
  vtkRenderWindow rw = new vtkRenderWindow();     
  vtkRenderer ren = new vtkRenderer();
  vtkCamera cam = null;
  vtkLight lgt = new vtkLight();
  int lastX;
  int lastY;
  int windowSet = 0;
  int LightFollowCamera = 1;
  int InteractionMode = 1;

  static { System.loadLibrary("vtkJava"); }

public vtkPanel()
  {
    rw.AddRenderer(ren);
    addMouseListener(this);
    addMouseMotionListener(this);
    addKeyListener(this);
  }

public int getWindowID() 
  {
    DrawingSurfaceInfo surfaceInfo =
      ((DrawingSurface)this.getPeer()).getDrawingSurfaceInfo();
    surfaceInfo.lock();
    Win32DrawingSurface wds =
      (Win32DrawingSurface)surfaceInfo.getSurface();
    int hWnd = wds.getHWnd();
    surfaceInfo.unlock();
    return hWnd;
  }
 
public String GetWindowInfo()
  {
    String result;
    result = "" + this.getWindowID();
    return result; 
  }

public void setSize(int x, int y)
  {
    super.setSize(x,y);
    rw.SetSize(x,y);
  }

public vtkRenderer GetRenderer()
  {
    return ren;
  }

public vtkRenderWindow GetRenderWindow()
  {
    return rw;
  }

public void Render() 
  {
    if (ren.VisibleActorCount() == 0) return;
    if (rw != null)
      {
      if (windowSet == 0)
	{ 
	// set the window id and the active camera
	rw.SetWindowInfo(this.GetWindowInfo());
	cam = ren.GetActiveCamera();
	ren.AddLight(lgt);
	lgt.SetPosition(cam.GetPosition());
	lgt.SetFocalPoint(cam.GetFocalPoint());
	windowSet = 1;
	}
      rw.Render();
      }
  }

public void paint(Graphics g)
  {
    if (ren.VisibleActorCount() == 0) return;
    if (rw != null)
      {
      if (windowSet == 0)
	{ 
	// set the window id and the active camera
	rw.SetWindowInfo(this.GetWindowInfo());
	cam = ren.GetActiveCamera();
	ren.AddLight(lgt);
	lgt.SetPosition(cam.GetPosition());
	lgt.SetFocalPoint(cam.GetFocalPoint());
	windowSet = 1;
	}
      rw.Render();
      }
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
    lgt.SetPosition(cam.GetPosition());
    lgt.SetFocalPoint(cam.GetFocalPoint());
  }

public void mouseClicked(MouseEvent e) {}

public void mousePressed(MouseEvent e)
  {
    if (ren.VisibleActorCount() == 0) return;
    rw.SetDesiredUpdateRate(5.0);
    lastX = e.getX();
    lastY = e.getY();
    if(e.getModifiers()==InputEvent.BUTTON1_MASK)
      {
      InteractionModeRotate();
      }
    else if ((e.getModifiers()==InputEvent.BUTTON2_MASK) ||
	     (e.getModifiers()==(InputEvent.BUTTON1_MASK | InputEvent.SHIFT_MASK)))
      {
      InteractionModeTranslate();
      }
    else if (e.getModifiers()==InputEvent.BUTTON3_MASK)
      {
      InteractionModeZoom();
      }
  }

public void mouseReleased(MouseEvent e)
  {
    rw.SetDesiredUpdateRate(0.01);
  }

public void mouseEntered(MouseEvent e) {}

public void mouseExited(MouseEvent e) {}

public void mouseMoved(MouseEvent e) {}


public void mouseDragged(MouseEvent e)
  {
    if (ren.VisibleActorCount() == 0) return;
    int x = e.getX();
    int y = e.getY();
    // rotate
    if (this.InteractionMode == 1)
      {
      cam.Azimuth(lastX - x);
      cam.Elevation(y - lastY);
      cam.OrthogonalizeViewUp();
      if (this.LightFollowCamera == 1)
	{
	lgt.SetPosition(cam.GetPosition());
	lgt.SetFocalPoint(cam.GetFocalPoint());
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
      FPoint = cam.GetFocalPoint();
      PPoint = cam.GetPosition();

      // calculate the focal depth since we'll be using it a lot
      ren.SetWorldPoint(FPoint[0],FPoint[1],FPoint[2],1.0);
      ren.WorldToDisplay();
      focalDepth = ren.GetDisplayPoint()[2];

      APoint[0] = rw.GetSize()[0]/2.0 + (x - lastX);
      APoint[1] = rw.GetSize()[1]/2.0 - (y - lastY);
      APoint[2] = focalDepth;
      ren.SetDisplayPoint(APoint);
      ren.DisplayToWorld();
      RPoint = ren.GetWorldPoint();
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
      cam.SetFocalPoint(
			(FPoint[0]-RPoint[0])/2.0 + FPoint[0],
			(FPoint[1]-RPoint[1])/2.0 + FPoint[1],
			(FPoint[2]-RPoint[2])/2.0 + FPoint[2]);
      cam.SetPosition(
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
      if (cam.GetParallelProjection() == 1)
	{
	cam.SetParallelScale(cam.GetParallelScale()/zoomFactor);
	}
      else
	{
	clippingRange = cam.GetClippingRange();
	cam.SetClippingRange(clippingRange[0]/zoomFactor,
			     clippingRange[1]/zoomFactor);
	cam.Dolly(zoomFactor);
	}
      }
    lastX = x;
    lastY = y;
    rw.Render();
  }

public void keyTyped(KeyEvent e) {}
        
public void keyPressed(KeyEvent e)
  {
    if (ren.VisibleActorCount() == 0) return;
    char keyChar = e.getKeyChar();

    if ('r' == keyChar)
      {
      ren.ResetCamera();
      rw.Render();
      }
    if ('w' == keyChar)
      {
      vtkActorCollection ac;
      vtkActor anActor;
      vtkActor aPart;
      int i, j;
          
      ac = ren.GetActors();
      ac.InitTraversal();
      for (i = 0; i < ac.GetNumberOfItems(); i++)
	{
	anActor = ac.GetNextItem();
	anActor.InitPartTraversal();
	for (j = 0; j < anActor.GetNumberOfParts(); j++)
	  { 
	  aPart = anActor.GetNextPart();
	  aPart.GetProperty().SetRepresentationToWireframe();
	  }
	}
      rw.Render();
      }
    if ('s' == keyChar)
      {
      vtkActorCollection ac;
      vtkActor anActor;
      vtkActor aPart;
      int i, j;
          
      ac = ren.GetActors();
      ac.InitTraversal();
      for (i = 0; i < ac.GetNumberOfItems(); i++)
	{
	anActor = ac.GetNextItem();
	anActor.InitPartTraversal();
	for (j = 0; j < anActor.GetNumberOfParts(); j++)
	  { 
	  aPart = anActor.GetNextPart();
	  aPart.GetProperty().SetRepresentationToSurface();
	  }
	}
      rw.Render();
      }
  }

public void keyReleased(KeyEvent e) {}
}
