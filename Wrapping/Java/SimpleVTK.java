
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import vtk.*;
import vtkPanel;
/**
 * An application that displays a JButton and several JRadioButtons.
 * The JRadioButtons determine the look and feel used by the application.
 */
public class SimpleVTK extends JPanel implements ActionListener {
    static JFrame frame;

    Button exitButton;

    public SimpleVTK() {
	// Create the buttons.
	vtkPanel renWin = new vtkPanel();
        vtkConeSource cone = new vtkConeSource();
        cone.SetResolution(8);
        vtkPolyDataMapper coneMapper = new vtkPolyDataMapper();
        coneMapper.SetInput(cone.GetOutput());
        
        vtkActor coneActor = new vtkActor();
        coneActor.SetMapper(coneMapper);
        
        renWin.GetRenderer().AddActor(coneActor);

        exitButton = new Button("Exit");
        exitButton.addActionListener(this);

	add(renWin);
	add(exitButton);
    }


    /** An ActionListener that listens to the radio buttons. */
    public void actionPerformed(ActionEvent e) 
    {
        if (e.getSource().equals(exitButton)) 
            {
                System.exit(0);
            }
    }

    public static void main(String s[]) 
    {
	SimpleVTK panel = new SimpleVTK();
	
	frame = new JFrame("SimpleVTK");
	frame.addWindowListener(new WindowAdapter() 
            {
                public void windowClosing(WindowEvent e) {System.exit(0);}
            });
	frame.getContentPane().add("Center", panel);
	frame.pack();
	frame.setVisible(true);
    }
}

