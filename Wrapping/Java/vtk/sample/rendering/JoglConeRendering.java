package vtk.sample.rendering;

import java.awt.BorderLayout;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.util.Arrays;

import javax.swing.JFrame;
import javax.swing.SwingUtilities;

import vtk.vtkActor;
import vtk.vtkBoxRepresentation;
import vtk.vtkBoxWidget2;
import vtk.vtkCell;
import vtk.vtkCellPicker;
import vtk.vtkConeSource;
import vtk.vtkLookupTable;
import vtk.vtkNativeLibrary;
import vtk.vtkPolyDataMapper;
import vtk.vtkScalarBarRepresentation;
import vtk.vtkScalarBarWidget;
import vtk.vtkTransform;
import vtk.rendering.vtkAbstractEventInterceptor;
import vtk.rendering.vtkEventInterceptor;
import vtk.rendering.jogl.vtkAbstractJoglComponent;
import vtk.rendering.jogl.vtkJoglCanvasComponent;
import vtk.rendering.jogl.vtkJoglPanelComponent;

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
    final boolean usePanel = Boolean.getBoolean("usePanel");

    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        // build VTK Pipeline
        vtkConeSource cone = new vtkConeSource();
        cone.SetResolution(8);
        cone.Update();

        vtkPolyDataMapper coneMapper = new vtkPolyDataMapper();
        coneMapper.SetInputConnection(cone.GetOutputPort());

        final vtkActor coneActor = new vtkActor();
        coneActor.SetMapper(coneMapper);

        // VTK rendering part
        final vtkAbstractJoglComponent<?> joglWidget = usePanel ? new vtkJoglPanelComponent() : new vtkJoglCanvasComponent();
        System.out.println("We are using " + joglWidget.getComponent().getClass().getName() + " for the rendering.");

        joglWidget.getRenderer().AddActor(coneActor);

        // Add orientation axes
        vtkAbstractJoglComponent.attachOrientationAxes(joglWidget);

        // Add Scalar bar widget
        vtkLookupTable lut = new vtkLookupTable();
        lut.SetHueRange(.66, 0);
        lut.Build();
        vtkScalarBarWidget scalarBar = new vtkScalarBarWidget();
        scalarBar.SetInteractor(joglWidget.getRenderWindowInteractor());

        scalarBar.GetScalarBarActor().SetTitle("Example");
        scalarBar.GetScalarBarActor().SetLookupTable(lut);
        scalarBar.GetScalarBarActor().SetOrientationToHorizontal();
        scalarBar.GetScalarBarActor().SetTextPositionToPrecedeScalarBar();
        vtkScalarBarRepresentation srep = (vtkScalarBarRepresentation) scalarBar.GetRepresentation();
        srep.SetPosition(0.5, 0.053796);
        srep.SetPosition2(0.33, 0.106455);
        //scalarBar.ProcessEventsOff();
        scalarBar.EnabledOn();
        scalarBar.RepositionableOn();

        // Add interactive 3D Widget
        final vtkBoxRepresentation representation = new vtkBoxRepresentation();
        representation.SetPlaceFactor(1.25);
        representation.PlaceWidget(cone.GetOutput().GetBounds());

        final vtkBoxWidget2 boxWidget = new vtkBoxWidget2();
        boxWidget.SetRepresentation(representation);
        boxWidget.SetInteractor(joglWidget.getRenderWindowInteractor());
        boxWidget.SetPriority(1);

        final Runnable callback = new Runnable() {
          vtkTransform trasform = new vtkTransform();

          public void run() {
            vtkBoxRepresentation rep = (vtkBoxRepresentation) boxWidget.GetRepresentation();
            rep.GetTransform(trasform);
            coneActor.SetUserTransform(trasform);
          }
        };

        // Bind widget
        boxWidget.AddObserver("InteractionEvent", callback, "run");
        representation.VisibilityOn();
        representation.HandlesOn();
        boxWidget.SetEnabled(1);
        boxWidget.SetMoveFacesEnabled(1);

        // Add cell picker
        final vtkCellPicker picker = new vtkCellPicker();
        Runnable pickerCallback = new Runnable() {
          public void run() {
            if(picker.GetCellId() != -1) {
              vtkCell cell = picker.GetDataSet().GetCell(picker.GetCellId());
              System.out.println("Pick cell: " +  picker.GetCellId() + " - Bounds: " + Arrays.toString(cell.GetBounds()));
            }
          }
        };
        joglWidget.getRenderWindowInteractor().SetPicker(picker);
        picker.AddObserver("EndPickEvent", pickerCallback, "run");

        // Bind pick action to double-click
        joglWidget.getInteractorForwarder().setEventInterceptor(new vtkAbstractEventInterceptor() {

          public boolean mouseClicked(MouseEvent e) {
            // Request picking action on double-click
            final double[] position = {e.getX(), joglWidget.getComponent().getHeight() - e.getY(), 0};
            if(e.getClickCount() == 2) {
              System.out.println("Click trigger the picking (" + position[0] + ", " +position[1] + ")");
              picker.Pick(position, joglWidget.getRenderer());
            }

            // We let the InteractionStyle process the event anyway
            return false;
          }
        });

        // UI part
        JFrame frame = new JFrame("SimpleVTK");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.getContentPane().setLayout(new BorderLayout());
        frame.getContentPane().add(joglWidget.getComponent(),
            BorderLayout.CENTER);
        frame.setSize(400, 400);
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
        joglWidget.resetCamera();
        joglWidget.getComponent().requestFocus();

        // Add r:ResetCamera and q:Quit key binding
        joglWidget.getComponent().addKeyListener(new KeyListener() {
          @Override
          public void keyTyped(KeyEvent e) {
            if (e.getKeyChar() == 'r') {
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
