import vtk.*;

public class HelloWorld {

  // Load VTK library and print which library was not properly loaded
  static {
    if (!vtkNativeLibrary.LoadAllNativeLibraries()) {
      for (vtkNativeLibrary lib : vtkNativeLibrary.values()) {
        if (!lib.IsLoaded()) {
          System.out.println(lib.GetLibraryName() + " not loaded");
        }
      }
    }
    vtkNativeLibrary.DisableOutputWindow(null);
  }

  public static void main(String args[]) {
    vtkRandomGraphSource source = new vtkRandomGraphSource();

    vtkGraphLayoutView view = new vtkGraphLayoutView();
    view.AddRepresentationFromInputConnection(source.GetOutputPort());

    view.ResetCamera();
    view.Render();
    view.GetInteractor().Start();
  }
}
