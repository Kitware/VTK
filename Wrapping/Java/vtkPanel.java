import vtk.*;
import java.awt.*;
import java.awt.event.*;
import java.lang.Math;
import sun.awt.*;
import java.beans.*;

public class vtkPanel extends Canvas implements MouseListener, MouseMotionListener, KeyListener
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
  
  static { 
    System.loadLibrary("vtkCommonJava"); 
    System.loadLibrary("vtkFilteringJava"); 
    System.loadLibrary("vtkIOJava"); 
    System.loadLibrary("vtkImagingJava"); 
    System.loadLibrary("vtkGraphicsJava"); 
    System.loadLibrary("vtkRenderingJava"); 
  }

  public vtkPanel()
    {
      rw.AddRenderer(ren);
      addMouseListener(this);
      addMouseMotionListener(this);
      addKeyListener(this);
      super.setSize(200,200);
      rw.SetSize(200,200);
    }

  public vtkRenderer GetRenderer()
    {
      return ren;
    }
  
  public vtkRenderWindow GetRenderWindow()
    {
      return rw;
    }

  private native void RenderCreate(vtkRenderWindow id0);
  private native void RenderInternal(vtkRenderWindow id0);
  private native void SetSizeInternal(vtkRenderWindow id0, int x, int y);
  
  public void setSize(int x, int y)
    {
      super.setSize(x,y);
      SetSizeInternal(rw,x,y);
    }
  
  public synchronized void Render() 
    {
      if (!rendering)
        {
        rendering = true;
        if (ren.VisibleActorCount() == 0) return;
        if (rw != null)
          {
          if (windowset == 0)
            {
            // set the window id and the active camera
            cam = ren.GetActiveCamera();
            ren.AddLight(lgt);
            lgt.SetPosition(cam.GetPosition());
            lgt.SetFocalPoint(cam.GetFocalPoint());
            RenderCreate(rw);
            windowset = 1;
            }
          RenderInternal(rw);
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
      lgt.SetPosition(cam.GetPosition());
      lgt.SetFocalPoint(cam.GetFocalPoint());
    }
  
  public void mouseClicked(MouseEvent e) {
    
  }
  
  public void mousePressed(MouseEvent e)
    {
      
      if (ren.VisibleActorCount() == 0) return;
      rw.SetDesiredUpdateRate(5.0);
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
      rw.SetDesiredUpdateRate(0.01);
    }
  
  public void mouseEntered(MouseEvent e) 
    {
      this.requestFocus();
    }
  
  public void mouseExited(MouseEvent e) {}
  
  public void mouseMoved(MouseEvent e) 
    {
      lastX = e.getX();
      lastY = e.getY();
    }
  
  
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
        ren.ResetCameraClippingRange();
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
        ren.ResetCameraClippingRange();
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
          cam.Dolly(zoomFactor);
          ren.ResetCameraClippingRange();
          }
        }
      lastX = x;
      lastY = y;
      this.Render();
    }
  
  public void keyTyped(KeyEvent e) {}
  
  public void keyPressed(KeyEvent e)
    {
      if (ren.VisibleActorCount() == 0) return;
      char keyChar = e.getKeyChar();
      
      if ('r' == keyChar)
        {
        ren.ResetCamera();
        this.Render();
        }
      if ('u' == keyChar)
        {
        vtkPicker picker = new vtkPicker();
        picker.Pick(lastX,700 - lastY,0.0,ren);
        
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
          anActor = ac.GetNextActor();
          anActor.InitPartTraversal();
          for (j = 0; j < anActor.GetNumberOfParts(); j++)
            { 
            aPart = anActor.GetNextPart();
            aPart.GetProperty().SetRepresentationToWireframe();
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
        
        ac = ren.GetActors();
        ac.InitTraversal();
        for (i = 0; i < ac.GetNumberOfItems(); i++)
          {
          anActor = ac.GetNextActor();
          anActor.InitPartTraversal();
          for (j = 0; j < anActor.GetNumberOfParts(); j++)
            { 
            aPart = anActor.GetNextPart();
            aPart.GetProperty().SetRepresentationToSurface();
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
