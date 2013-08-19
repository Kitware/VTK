package vtk.sample.rendering;

import java.awt.BorderLayout;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;

import javax.swing.JFrame;
import javax.swing.SwingUtilities;

import vtk.vtkActor;
import vtk.vtkConeSource;
import vtk.vtkNativeLibrary;
import vtk.vtkPolyDataMapper;
import vtk.rendering.jogl.vtkJoglComponent;

public class JoglConeRendering {
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
        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                // build VTK Pipeline
                vtkConeSource cone = new vtkConeSource();
                cone.SetResolution(8);

                vtkPolyDataMapper coneMapper = new vtkPolyDataMapper();
                coneMapper.SetInputConnection(cone.GetOutputPort());

                vtkActor coneActor = new vtkActor();
                coneActor.SetMapper(coneMapper);

                // VTK rendering part
                final vtkJoglComponent joglWidget = new vtkJoglComponent();
                joglWidget.getRenderer().AddActor(coneActor);

                // UI part
                JFrame frame = new JFrame("SimpleVTK");
                frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
                frame.getContentPane().setLayout(new BorderLayout());
                frame.getContentPane().add(joglWidget.getComponent(),
                        BorderLayout.CENTER);
                frame.setSize(400, 400);
                frame.setLocationRelativeTo(null);
                frame.setVisible(true);

                // Add r:ResetCamera and q:Quit key binding
                joglWidget.getComponent().addKeyListener(new KeyListener() {

					@Override
					public void keyTyped(KeyEvent e) {
						if(e.getKeyChar() == 'r') {
							joglWidget.resetCamera();
						} else if (e.getKeyChar() == 'q') {
							System.exit(0);
						}
					}

					@Override
					public void keyReleased(KeyEvent e) {
					}

					@Override
					public void keyPressed(KeyEvent e) {
					}
				});
            }
        });
    }
}
