import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import vtk.*;

public class TestVTKCanvas extends JPanel {

  public TestVTKCanvas() {
    setLayout(new BorderLayout());
    // Create the buttons.
    vtkCanvas renWin = new vtkCanvas();
    add(renWin, BorderLayout.CENTER);
    vtkConeSource cone = new vtkConeSource();
    cone.SetResolution(8);
    vtkPolyDataMapper coneMapper = new vtkPolyDataMapper();
    coneMapper.SetInput(cone.GetOutput());
        
    vtkActor coneActor = new vtkActor();
    coneActor.SetMapper(coneMapper);
        
    renWin.GetRenderer().AddActor(coneActor);
    AxesActor aa = new AxesActor(renWin.GetRenderer());
    renWin.GetRenderer().AddActor(aa);
  }


  public static void main(String s[]) 
  {
    TestVTKCanvas panel = new TestVTKCanvas();
    TestVTKCanvas panel2 = new TestVTKCanvas();
	
    JFrame frame = new JFrame("VTK Canvas Test");
    frame.getContentPane().setLayout(new GridLayout(2,1));
    frame.addWindowListener(new WindowAdapter() 
      {
        public void windowClosing(WindowEvent e) {System.exit(0);}
      });
    frame.getContentPane().add(panel);
    frame.getContentPane().add(panel2);
    frame.pack();
    frame.setVisible(true);
  }
}

