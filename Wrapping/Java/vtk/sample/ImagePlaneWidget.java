package vtk.sample;

import java.awt.BorderLayout;
import java.io.File;

import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;

import vtk.vtkActor;
import vtk.vtkCanvas;
import vtk.vtkCellPicker;
import vtk.vtkImageData;
import vtk.vtkImagePlaneWidget;
import vtk.vtkNativeLibrary;
import vtk.vtkOutlineFilter;
import vtk.vtkPolyDataMapper;
import vtk.vtkVolume16Reader;

/**
 * Example of complex 3D widget in use.
 */
public class ImagePlaneWidget extends vtkCanvas {
    private static final long serialVersionUID = 1L;
    private int width = 512;
    private int height = 512;

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

    public ImagePlaneWidget(String path) {
        // Start by loading some data.
        vtkVolume16Reader v16 = new vtkVolume16Reader();
        v16.SetDataDimensions(64, 64);
        v16.SetDataByteOrderToLittleEndian();
        v16.SetFilePrefix(path);
        v16.SetImageRange(1, 93);
        v16.SetDataSpacing(3.2, 3.2, 1.5);
        v16.Update();

        setImageData(v16.GetOutput());

        JPanel p = new JPanel();
        p.setLayout(new BorderLayout());
        p.add(this, BorderLayout.CENTER);

        JFrame frame = new JFrame("ImagePlaneWidget Test");
        frame.setBounds(10, 10, width, height);
        frame.getContentPane().add(p, BorderLayout.CENTER);
        frame.setVisible(true);
        frame.pack();
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    }

    public void setImageData(vtkImageData id) {
        // The shared picker enables us to use 3 planes at one time
        // and gets the picking order right
        vtkCellPicker picker = new vtkCellPicker();
        picker.SetTolerance(0.005);

        // The 3 image plane widgets are used to probe the dataset.
        vtkImagePlaneWidget planeWidgetX = new vtkImagePlaneWidget();
        planeWidgetX.DisplayTextOn();
        planeWidgetX.SetInput(id);
        planeWidgetX.SetInteractor(getRenderWindowInteractor());
        planeWidgetX.SetPlaneOrientationToXAxes();
        planeWidgetX.SetSliceIndex(32);
        planeWidgetX.SetPicker(picker);
        planeWidgetX.SetKeyPressActivationValue('x');
        planeWidgetX.GetPlaneProperty().SetColor(1, 0, 0);
        planeWidgetX.On();

        vtkImagePlaneWidget planeWidgetY = new vtkImagePlaneWidget();
        planeWidgetY.DisplayTextOn();
        planeWidgetY.SetInput(id);
        planeWidgetY.SetInteractor(getRenderWindowInteractor());
        planeWidgetY.SetPlaneOrientationToYAxes();
        planeWidgetY.SetSliceIndex(32);
        planeWidgetY.SetPicker(picker);
        planeWidgetY.SetKeyPressActivationValue('y');
        planeWidgetY.GetPlaneProperty().SetColor(1, 1, 0);
        planeWidgetY.SetLookupTable(planeWidgetX.GetLookupTable());
        planeWidgetY.On();

        // for the z-slice, turn off texture interpolation:
        // interpolation is now nearest neighbour, to demonstrate
        // cross-hair cursor snapping to pixel centers
        vtkImagePlaneWidget planeWidgetZ = new vtkImagePlaneWidget();
        planeWidgetZ.DisplayTextOn();
        planeWidgetZ.SetInput(id);
        planeWidgetZ.TextureInterpolateOff();
        planeWidgetZ.SetInteractor(getRenderWindowInteractor());
        planeWidgetZ.SetPlaneOrientationToZAxes();
        planeWidgetZ.SetSliceIndex(46);
        planeWidgetZ.SetPicker(picker);
        planeWidgetZ.SetKeyPressActivationValue('z');
        planeWidgetZ.GetPlaneProperty().SetColor(0, 0, 1);
        planeWidgetZ.SetLookupTable(planeWidgetX.GetLookupTable());
        planeWidgetZ.On();

        // An outline is shown for context.
        vtkOutlineFilter outline = new vtkOutlineFilter();
        outline.SetInput(id);

        vtkPolyDataMapper outlineMapper = new vtkPolyDataMapper();
        outlineMapper.SetInput(outline.GetOutput());

        vtkActor outlineActor = new vtkActor();
        outlineActor.SetMapper(outlineMapper);

        GetRenderer().AddActor(outlineActor);

        // Add the outline actor to the renderer, set the background and size
        GetRenderer().GetCullers().RemoveAllItems();

        GetRenderer().SetBackground(0.1, 0.1, 0.2);
    }

    static public void printUsage(String err) {
        if (!err.equals("")) {
            System.err.println("Error: " + err);
        }
        System.err.println("Usage: java ImagePlaneWidget [-D path]");
        System.err.println("Where:");
        System.err.println("      path is location of your VTKData directory");
        System.exit(-1);
    }

    public static void main(String[] argv) {
        int argSize = argv.length;
        String pathToVTKData = "";
        int argCurrent = 0;

        try {
            while (argSize > argCurrent) {
                if (argv[argCurrent].equals("-D")) {
                    ++argCurrent;
                    pathToVTKData = argv[argCurrent];
                    ++argCurrent;
                } else {
                    ImagePlaneWidget.printUsage("");
                }
            }
        } catch (Exception e) {
            ImagePlaneWidget.printUsage("");
        }

        if (pathToVTKData.equals(""))
            ImagePlaneWidget.printUsage("");

        final File f = new File(pathToVTKData + "/Data/headsq");
        if (!f.exists() || !f.canRead() || !f.isDirectory())
            ImagePlaneWidget.printUsage(f.getAbsolutePath() + " does not exist or cannot be read.");

        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                new ImagePlaneWidget(f.getAbsolutePath() + "/quarter");
            }
        });
    }
}
