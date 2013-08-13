package vtk.sample.rendering.annotation;

import javax.swing.JFrame;

import vtk.vtkActor;
import vtk.vtkCubeAxesActor;
import vtk.vtkDelaunay2D;
import vtk.vtkNativeLibrary;
import vtk.vtkPanel;
import vtk.vtkPoints;
import vtk.vtkPolyData;
import vtk.vtkPolyDataMapper;
import vtk.vtkRenderer;
import vtk.vtkStringArray;

public class LabeledCubeAxesActor {
    // Load VTK library and print which library was not properly loaded
    static {
        if(!vtkNativeLibrary.LoadAllNativeLibraries()) {
            for(vtkNativeLibrary lib : vtkNativeLibrary.values()) {
                if(!lib.IsLoaded()) {
                    System.out.println(lib.GetLibraryName() + " not loaded");
                }
            }
        }
        vtkNativeLibrary.DisableOutputWindow(null);
    }

    public static void main(String args[]) {
        try {
            javax.swing.SwingUtilities.invokeLater(new Runnable() {
                @Override
                public void run() {
                    createVtkPanel();
                }
            });
        } catch(Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * @param panel
     */
    static void createVtkPanel() {
        JFrame mainFrame = new JFrame();
        vtkPanel panel = new vtkPanel();
        vtkRenderer renderer = panel.GetRenderer();

        vtkPoints points = getPoints();

        vtkPolyData polyData = new vtkPolyData();
        polyData.SetPoints(points);

        vtkDelaunay2D delaunay = new vtkDelaunay2D();
        delaunay.SetInputData(polyData);

        vtkPolyDataMapper mapper = new vtkPolyDataMapper();
        mapper.SetInputConnection(delaunay.GetOutputPort());

        vtkActor surfaceActor = new vtkActor();
        surfaceActor.SetMapper(mapper);
        renderer.AddActor(surfaceActor);

        vtkCubeAxesActor cubeAxesActor = new vtkCubeAxesActor();
        cubeAxesActor.SetCamera(renderer.GetActiveCamera());
        cubeAxesActor.SetBounds(points.GetBounds());
        cubeAxesActor.SetXTitle("Date");
        cubeAxesActor.SetXAxisMinorTickVisibility(0);
        cubeAxesActor.SetAxisLabels(0, getXLabels());
        cubeAxesActor.SetYTitle("Place");
        cubeAxesActor.SetYAxisMinorTickVisibility(0);
        cubeAxesActor.SetAxisLabels(1, getYLabels());
        cubeAxesActor.SetZTitle("Value");
        renderer.AddActor(cubeAxesActor);

        renderer.ResetCamera();
        mainFrame.add(panel);
        mainFrame.setSize(600, 600);
        panel.Render();
        mainFrame.setVisible(true);
        panel.Render();
    }

    /**
     * @return a list of places ordered the same as the y point values.
     */
    static vtkStringArray getYLabels() {
        vtkStringArray yLabel = new vtkStringArray();
        yLabel.InsertNextValue("A");
        yLabel.InsertNextValue("B");
        yLabel.InsertNextValue("C");
        return yLabel;
    }

    /**
     * @return a list of dates ordered the same as the x point values.
     * We have 4 X-values, from 0.5 to 3, that we will label with 6 dates
     * that will be linearly distributed along the axis.
     */
    static vtkStringArray getXLabels() {
        vtkStringArray xLabel = new vtkStringArray();
        xLabel.InsertNextValue("Jan");
        xLabel.InsertNextValue("Feb");
        xLabel.InsertNextValue("Mar");
        xLabel.InsertNextValue("Apr");
        xLabel.InsertNextValue("May");
        xLabel.InsertNextValue("June");
        return xLabel;
    }

    /**
     * @return data to plot.
     */
    static vtkPoints getPoints() {
        vtkPoints points = new vtkPoints();
        points.InsertNextPoint(0.5, 0, 0.);
        points.InsertNextPoint(1, 0, 1.);
        points.InsertNextPoint(2, 0, 0.4);
        points.InsertNextPoint(3, 0, 0.5);
        points.InsertNextPoint(0.5, 1, 0.3);
        points.InsertNextPoint(1, 1, 0.3);
        points.InsertNextPoint(2, 1, 0.8);
        points.InsertNextPoint(3, 1, 0.6);
        points.InsertNextPoint(0.5, 2, 0.5);
        points.InsertNextPoint(1, 2, 0.8);
        points.InsertNextPoint(2, 2, 0.3);
        points.InsertNextPoint(3, 2, 0.4);
        return points;
    }
}
