package vtk.sample;

import java.awt.BorderLayout;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.util.concurrent.TimeUnit;

import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;

import vtk.vtkActor;
import vtk.vtkConeSource;
import vtk.vtkNativeLibrary;
import vtk.vtkObject;
import vtk.vtkPanel;
import vtk.vtkPolyDataMapper;

/**
 * This test create in a secondary frame a VTK application. When that frame get
 * closed we want to make sure all the VTK memory is released without any crash.
 *
 * @author sebastien jourdain - sebastien.jourdain@kitware.com
 */
public class ReleaseVtkPanel {

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

	// -----------------------------------------------------------------

	private static class VtkApplication extends JPanel {
		private static final long serialVersionUID = -6486953735097088917L;
		private vtkPanel panel3dA;
		private vtkPanel panel3dB;

		public VtkApplication() {
			super(new GridLayout(1, 2));
			panel3dA = new vtkPanel();
			panel3dB = new vtkPanel();

			vtkConeSource cone = new vtkConeSource();
			vtkPolyDataMapper mapper = new vtkPolyDataMapper();
			vtkActor actor = new vtkActor();

			mapper.SetInputConnection(cone.GetOutputPort());
			actor.SetMapper(mapper);

			panel3dA.GetRenderer().AddActor(actor);
			add(panel3dA);

			panel3dB.GetRenderer().AddActor(actor);
			add(panel3dB);
		}

		public void Delete() {
			panel3dA.Delete();
			panel3dB.Delete();
		}

	}

	public static void main(String[] args) {
		SwingUtilities.invokeLater(new Runnable() {

			public void run() {
				// Setup GC to run every 1 second in EDT
				vtkObject.JAVA_OBJECT_MANAGER.getAutoGarbageCollector().SetScheduleTime(5, TimeUnit.SECONDS);
				vtkObject.JAVA_OBJECT_MANAGER.getAutoGarbageCollector().SetDebug(true);
				vtkObject.JAVA_OBJECT_MANAGER.getAutoGarbageCollector().Start();

				JButton startVTKApp = new JButton("Start VTK application");
				startVTKApp.addActionListener(new ActionListener() {

					public void actionPerformed(ActionEvent e) {
						final VtkApplication app = new VtkApplication();
						JFrame f = buildFrame("VtkApp", app, 400, 200);
						f.addWindowListener(new WindowAdapter() {
							public void windowClosing(WindowEvent e) {
								app.Delete();
							}
						});
						f.setVisible(true);
					}
				});
				JFrame mainFrame = buildFrame("Launcher", startVTKApp, 200, 200);
				mainFrame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
				mainFrame.setVisible(true);
			}
		});
	}

	public static JFrame buildFrame(String title, JComponent content, int width, int height) {
		JFrame f = new JFrame(title);
		f.getContentPane().setLayout(new BorderLayout());
		f.getContentPane().add(content, BorderLayout.CENTER);
		f.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		f.setSize(width, height);
		f.setLocationRelativeTo(null);
		return f;
	}
}
