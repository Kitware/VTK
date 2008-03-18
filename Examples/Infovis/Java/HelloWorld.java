import vtk.*;

public class HelloWorld {

  // In the static constructor we load in the native code.
  // The libraries must be in your path to work.
  static {
    System.loadLibrary("vtkCommonJava");
    System.loadLibrary("vtkFilteringJava");
    System.loadLibrary("vtkIOJava");
    System.loadLibrary("vtkImagingJava");
    System.loadLibrary("vtkGraphicsJava");
    System.loadLibrary("vtkRenderingJava");
    System.loadLibrary("vtkInfovisJava");
    System.loadLibrary("vtkViewsJava");
  }

  public static void main(String args[]) {
    vtkRandomGraphSource source = new vtkRandomGraphSource();

    vtkGraphLayoutView view = new vtkGraphLayoutView();
    view.AddRepresentationFromInputConnection(source.GetOutputPort());

    vtkRenderWindow window = new vtkRenderWindow();
    view.SetupRenderWindow(window);
    window.GetInteractor().Start();
  }
}
