package vtk.sample.rendering;

import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

import vtk.vtkActor;
import vtk.vtkConeSource;
import vtk.vtkNativeLibrary;
import vtk.vtkPolyDataMapper;
import vtk.rendering.swt.vtkSwtComponent;

public class SwtConeRendering {
  // -----------------------------------------------------------------
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

  public static void main(String[] args) {

    Display display = new Display();
    Shell shell = new Shell(display);

    // build VTK Pipeline
    vtkConeSource cone = new vtkConeSource();
    cone.SetResolution(8);

    vtkPolyDataMapper coneMapper = new vtkPolyDataMapper();
    coneMapper.SetInputConnection(cone.GetOutputPort());

    vtkActor coneActor = new vtkActor();
    coneActor.SetMapper(coneMapper);

    // VTK rendering part
    final vtkSwtComponent swtWidget = new vtkSwtComponent(shell);
    swtWidget.getRenderer().AddActor(coneActor);

    shell.addControlListener(new ControlListener() {

      @Override
      public void controlResized(ControlEvent e) {
        if (e.widget instanceof Shell) {
          Shell s = (Shell) e.widget;
          swtWidget.setSize(s.getClientArea().width, s.getClientArea().height);
        }
      }

      @Override
      public void controlMoved(ControlEvent e) {
        // TODO Auto-generated method stub

      }
    });

    shell.pack();
    shell.open();
    while (!shell.isDisposed()) {
      if (!display.readAndDispatch())
        display.sleep();
    }
    display.dispose();
  }
}
