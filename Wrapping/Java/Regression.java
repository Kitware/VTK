import vtk.*;

public class Regression {
  static { 
    System.loadLibrary("vtkCommonJava"); 
    System.loadLibrary("vtkFilteringJava"); 
    System.loadLibrary("vtkIOJava"); 
    System.loadLibrary("vtkImagingJava"); 
    System.loadLibrary("vtkGraphicsJava"); 
    System.loadLibrary("vtkRenderingJava"); 
  }
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
    
    vtkPNGReader rtpnm = new vtkPNGReader();
    rtpnm.SetFileName("Regression.png");

    imgDiff.SetInput(w2if.GetOutput());
    imgDiff.SetImage(rtpnm.GetOutput());
    imgDiff.Update();

    if (imgDiff.GetThresholdedError() <= 10) 
	{
        System.out.println("Java smoke test passed."); 
	} 
      else 
        {
        System.out.println("Java smoke test error!"); 
	}	
    } 
}

