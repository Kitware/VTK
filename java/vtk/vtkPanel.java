package vtk;

import vtk.*;
import java.awt.*;
import java.applet.*;
import java.lang.Math;

public class vtkPanel extends Canvas {
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
	  ren.SetRenderWindow(rw);
	}

	public void resize(int x, int y)
	{
	  super.resize(x,y);
          rw.SetSize(x,y);
	}

	public vtkRenderer GetRenderer()
 	{
	  return ren;
	}

        public void Render() 
        {
	  if (rw != null)
	    {
	    if (windowSet == 0)
              { 
	      // set the window id and the active camera
              //this.setWindow(rw);
              cam = ren.GetActiveCamera();
	      ren.AddLight(lgt);
              lgt.SetPosition(cam.GetPosition());
              lgt.SetFocalPoint(cam.GetFocalPoint());
              windowSet = 1;
              }
            // System.out.println("Start Render 2");
	    rw.Render();
            // System.out.println("Finish Render 2");
            }
	}

	public void paint(Graphics g)
	{
	  if (rw != null)
	    {
	    if (windowSet == 0)
              { 
	      // set the window id and the active camera
              //this.setWindow(rw);
              cam = ren.GetActiveCamera();
	      ren.AddLight(lgt);
              lgt.SetPosition(cam.GetPosition());
              lgt.SetFocalPoint(cam.GetFocalPoint());
              windowSet = 1;
              }
            // System.out.println("Starting Render 1");
	    rw.Render();
            // System.out.println("Finished Render 1");
            }
	}

	//public native void setWindow(vtkRenderWindow renWin);

	public boolean mouseUp(Event e, int x, int y)
	{
          rw.SetDesiredUpdateRate(0.01);
	  return true;
	}

	public boolean mouseDown(Event e, int x, int y)
	{
          rw.SetDesiredUpdateRate(5.0);
          lastX = x;
	  lastY = y;
	  return true;
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

	public boolean mouseDrag(Event e, int x, int y)
	{
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
	  return true;
	}

      public boolean keyDown(Event e, int key) 
        {
        if (e.id == Event.KEY_PRESS)
	  {
	  if ('1' == e.key)
	    {
	    this.InteractionModeRotate();
            }
	  if ('2' == e.key)
	    {
	    this.InteractionModeTranslate();
            }
	  if ('3' == e.key)
	    {
	    this.InteractionModeZoom();
            }
          if ('r' == e.key)
            {
	    ren.ResetCamera();
	    rw.Render();
            }
          if ('w' == e.key)
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
          if ('s' == e.key)
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
        return true;
        }
}
