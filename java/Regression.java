import vtk.*;

public class Regression {
  static { System.loadLibrary("vtkJava"); }
  public static void main (String []args) {
    vtkRenderWindow renWin = new vtkRenderWindow();
    vtkRenderer ren1 = new vtkRenderer();
    renWin.AddRenderer(ren1);
    vtkConeSource cone = new vtkConeSource();
    cone.SetResolution(8);
    vtkPolyDataMapper coneMapper = new vtkPolyDataMapper();
    coneMapper.SetInput(cone.GetOutput());

    vtkActor coneActor = new vtkActor();
    coneActor.SetMapper(coneMapper);

    ren1.AddActor(coneActor);
    renWin.Render();
    renWin.Render();

    vtkWindowToImageFilter w2if = new vtkWindowToImageFilter();
    w2if.SetInput(renWin);

    vtkImageDifference imgDiff = new vtkImageDifference();
    
    vtkPNMReader rtpnm = new vtkPNMReader();
    rtpnm.SetFileName("Regression.ppm");

    imgDiff.SetInput(w2if.GetOutput());
    imgDiff.SetImage(rtpnm.GetOutput());
    imgDiff.Update();

    if (imgDiff.GetThresholdedError() <= 10) 
	{
        System.out.println("Java smoke test passed.<br>"); 
	} 
      else 
        {
        System.out.println("<FONT COLOR=#DC143C>Java smoke test error!</font><br>"); 
	}	
    } 
}

