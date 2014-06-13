package vtk.test;

import vtk.vtkDoubleArray;
import vtk.vtkJavaTesting;
import vtk.vtkObject;
import vtk.vtkQuadric;
import vtk.vtkSampleFunction;

/**
 * This test should go on forever without crashing.
 */
public class JavaDelete {

  public static void main(String[] args) {
    try {
      vtkJavaTesting.Initialize(args, true);

      // Start working code
      long timeout = System.currentTimeMillis() + 60000; // +1 minute
      while (System.currentTimeMillis() < timeout) {
        vtkDoubleArray arr = new vtkDoubleArray();
        arr.Delete();

        vtkQuadric quadric = new vtkQuadric();
        vtkSampleFunction sample = new vtkSampleFunction();
        sample.SetSampleDimensions(30, 30, 30);
        sample.SetImplicitFunction(quadric);
        sample.GetImplicitFunction();
        sample.Delete();
        quadric.Delete();

        // Make sure the Java VTK object map is empty
        if (vtkObject.JAVA_OBJECT_MANAGER.getSize() > 1) { // vtkTesting
          System.out.println(vtkObject.JAVA_OBJECT_MANAGER.gc(true).listKeptReferenceToString());
          throw new RuntimeException("There shouldn't have any VTK object inside the map as we are using Delete().");
        }
      }

      vtkJavaTesting.Exit(vtkJavaTesting.PASSED);
    } catch (Throwable e) {
      e.printStackTrace();
      vtkJavaTesting.Exit(vtkJavaTesting.FAILED);
    }
  }
}
