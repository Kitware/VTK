import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import vtk.*;

/**
 * This test should go on forever without crashing.
 */
public class TestJavaGCHashTable {

  static {
    System.out.println(System.getProperty("java.library.path"));
    System.loadLibrary("vtkCommonJava");
    System.loadLibrary("vtkImagingJava");
  }

  public static void main(String[] args) {
    vtkDataArray pseudoStatic = new vtkDoubleArray();
    while (true) {
      vtkAbstractArray arr = pseudoStatic.CreateDataArray(8);
      arr.Delete();
      arr.Delete();

      vtkQuadric quadric = new vtkQuadric();
      vtkSampleFunction sample = new vtkSampleFunction();
      sample.SetSampleDimensions(30, 30, 30);
      sample.SetImplicitFunction(quadric);
      sample.GetImplicitFunction();
      sample.Delete();
      quadric.Delete();
    }
  }

}

