package vtk.test;

import java.util.concurrent.TimeUnit;

import vtk.vtkIdTypeArray;
import vtk.vtkJavaTesting;
import vtk.vtkObject;
import vtk.vtkObjectBase;
import vtk.vtkReferenceInformation;
import vtk.vtkSelection;
import vtk.vtkSelectionNode;

/**
 * This test should run indefinitely, occasionally outputting
 * "N references deleted".
 *
 * It is an example of how to execute a non-interactive intense processing Java
 * script using VTK with manual garbage collection.
 */
public class ManualGC {

    private static vtkIdTypeArray createSelection() {
        vtkSelection sel = new vtkSelection();
        vtkSelectionNode node = new vtkSelectionNode();
        vtkIdTypeArray arr = new vtkIdTypeArray();
        node.SetSelectionList(arr);
        sel.AddNode(node);
        return arr;
    }

    public static void main(String[] args) {
        try {
            vtkJavaTesting.Initialize(args, true);
            int count = 0;
            vtkJavaTesting.StartTimeoutExit(1, TimeUnit.MINUTES);
            //vtkJavaTesting.StartGCInEDT(100, TimeUnit.MILLISECONDS);
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
                // We must manually call
                // vtkObject.JAVA_OBJECT_MANAGER.gc(true/false) when we
                // want to collect unused VTK objects.
                vtkIdTypeArray arr = createSelection();
                for (int i = 0; i < 10000; ++i) {
                    arr.Register(null);
                    vtkObjectBase.VTKDeleteReference(arr.GetVTKId());
                }
                ++count;
                if (count % 100 == 0) {
                    vtkReferenceInformation infos = vtkObject.JAVA_OBJECT_MANAGER.gc(false);
                    System.out.println(infos.toString());
                }
            }
        } catch (Throwable e) {
            e.printStackTrace();
            vtkJavaTesting.Exit(vtkJavaTesting.FAILED);
        }
    }
}
