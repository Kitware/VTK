import vtk.*;

public class Regression 
{
  public static void main (String []args) 
    {
    vtkTesting.Initialize(args);

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

    vtkTesting.RegressionTestImage(renWin, args);
    }
}

