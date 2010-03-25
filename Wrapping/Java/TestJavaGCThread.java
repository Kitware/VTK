
import java.awt.BorderLayout;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JFrame;
import javax.swing.SwingUtilities;
import vtk.vtkActor;
import vtk.vtkConeSource;
import vtk.vtkGlobalJavaHash;
import vtk.vtkJavaGarbageCollector;
import vtk.vtkNativeLibrary;
import vtk.vtkPanel;
import vtk.vtkPolyDataMapper;

/**
 * This test should print out garbage collection messages at regular
 * intervals, then eventually stop (after ~30s) with no leaks.
 *
 * @author sebastien jourdain - sebastien.jourdain@kitware.com
 */
public class TestJavaGCThread {

  public static void main(String[] args) {
    if (!vtkNativeLibrary.LoadAllNativeLibraries()) {
      for (vtkNativeLibrary lib : vtkNativeLibrary.values()) {
        if (!lib.IsLoaded()) {
          System.out.println(lib.GetLibraryName() + " not loaded");
        }
      }
    }

    vtkConeSource cone = new vtkConeSource();
    vtkPolyDataMapper mapper = new vtkPolyDataMapper();
    vtkActor actor = new vtkActor();
    
    mapper.SetInputConnection(cone.GetOutputPort());
    actor.SetMapper(mapper);
    
    vtkPanel panel = new vtkPanel();
    panel.GetRenderer().AddActor(actor);
    
    JFrame f = new JFrame();
    f.getContentPane().setLayout(new BorderLayout());
    f.getContentPane().add(panel, BorderLayout.CENTER);
    f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    f.setSize(400, 400);
    f.setVisible(true);

    // Garbage collection test
    vtkJavaGarbageCollector gc = vtkGlobalJavaHash.GarbageCollector;
    gc.SetDebug(true);
    gc.SetScheduleTime(2, TimeUnit.SECONDS);
    System.out.println("Start 2s");
    gc.SetAutoGarbageCollection(true);
    try {
      Thread.sleep(5000);
    } catch (InterruptedException ex) {
      Logger.getLogger(TestJavaGCThread.class.getName()).log(Level.SEVERE, null, ex);
    }
    System.out.println("Start .5 s");
    gc.SetScheduleTime(500, TimeUnit.MILLISECONDS);


    try {
      Thread.sleep(5000);
    } catch (InterruptedException ex) {
      Logger.getLogger(TestJavaGCThread.class.getName()).log(Level.SEVERE, null, ex);
    }
    System.out.println("Stop");
    gc.SetAutoGarbageCollection(false);
    System.out.println("setTime");
    try {
      Thread.sleep(5000);
    } catch (InterruptedException ex) {
      Logger.getLogger(TestJavaGCThread.class.getName()).log(Level.SEVERE, null, ex);
    }
    gc.SetScheduleTime(500, TimeUnit.MILLISECONDS);
    try {
      Thread.sleep(1000);
    } catch (InterruptedException ex) {
      Logger.getLogger(TestJavaGCThread.class.getName()).log(Level.SEVERE, null, ex);
    }
    System.out.println("Restart");
    gc.SetAutoGarbageCollection(true);
    try {
      Thread.sleep(1000);
    } catch (InterruptedException ex) {
      Logger.getLogger(TestJavaGCThread.class.getName()).log(Level.SEVERE, null, ex);
    }

    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        int num = vtkGlobalJavaHash.DeleteAll();
        System.out.println("vtkGlobalJavaHash deleted " + num + " references on shutdown.");
        System.exit(0);
      }
    });

  }
}
