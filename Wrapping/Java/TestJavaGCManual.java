import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import vtk.*;

/**
 * This test should run indefinitely, occasionally outputting
 * "N references deleted".
 *
 * It is an example of how to execute a non-interactive intense
 * processing Java script using VTK with manual garbage collection.
 */
public class TestJavaGCManual {

  static {
    System.out.println(System.getProperty("java.library.path"));
    System.loadLibrary("vtkCommonJava");
    System.loadLibrary("vtkImagingJava");
  }

  private static vtkIdTypeArray createSelection() {
    vtkSelection sel = new vtkSelection();
    vtkSelectionNode node = new vtkSelectionNode();
    vtkIdTypeArray arr = new vtkIdTypeArray();
    node.SetSelectionList(arr);
    sel.AddNode(node);
    return arr;
  }

  public static void main(String[] args) {
    int count = 0;
    while (true) {
      // When the selection is deleted,
      // it will decrement the array's reference count.
      // If GC is done on a different thread, this will
      // interfere with the Register/Delete calls on
      // this thread and cause a crash. In general, the code
      // executed in a C++ destructor can do anything, so it
      // is never safe to delete objects on one thread while
      // using them on another.
      //
      // Thus we no longer implement finalize() for VTK objects.
      // We must manually call vtkGlobalJavaHash.GC() when we
      // want to collect unused VTK objects.
      vtkIdTypeArray arr = createSelection();
      for (int i = 0; i < 10000; ++i) {
        arr.Register(null);
        vtkObjectBase.VTKDeleteReference(arr.GetVTKId());
      }
      ++count;
      if (count % 100 == 0) {
        int num = vtkGlobalJavaHash.GC();
        System.out.println(num + " references deleted.");
      }
    }
  }

}
