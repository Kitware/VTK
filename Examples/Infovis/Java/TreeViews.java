
import java.awt.GridLayout;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.util.Random;

import javax.swing.JComponent;
import javax.swing.JFrame;

import vtk.*;

public class TreeViews extends JComponent {

  private vtkRenderWindowPanel panel1 = new vtkRenderWindowPanel();
  private vtkRenderWindowPanel panel2 = new vtkRenderWindowPanel();
  private vtkRenderWindowPanel panel3 = new vtkRenderWindowPanel();
  private vtkRenderWindowPanel panel4 = new vtkRenderWindowPanel();

  private vtkGraphLayoutView view1;
  private vtkTreeLayoutView view2;
  private vtkTreeLayoutView view3;
  private vtkTreeMapView view4;

  public TreeViews() {
    // Create a random tree
    // The tree has a "name" field to use for labels.
    // It also has a "level" field to use for colors.
    vtkStringArray nameArr = new vtkStringArray();
    nameArr.SetName("name");    
    vtkIntArray levelArr = new vtkIntArray();
    levelArr.SetName("level");
    vtkTree tree = new vtkTree();
    // Start by adding a root to the tree
    tree.AddRoot();
    nameArr.InsertNextValue("0");
    levelArr.InsertNextValue(0);
    Random r = new Random();
    for (int i = 1; i < 100; ++i) {
      // Add a child to a random vertex in the tree.
      tree.AddChild(r.nextInt(i));
      nameArr.InsertNextValue("" + i);
      levelArr.InsertNextValue(tree.GetLevel(i));
    }
    tree.GetVertexData().AddArray(nameArr);
    tree.GetVertexData().AddArray(levelArr);

    // Create the selection link.
    // This is shared among all views in order to link the selection.
    vtkSelectionLink link = new vtkSelectionLink();

    // Create a graph layout view.
    // This view performs an automatic embedding of the graph in 2D.
    // Label by name, color by level.
    view1 = new vtkGraphLayoutView();
    vtkDataRepresentation rep1 = view1.AddRepresentationFromInput(tree);
    view1.SetupRenderWindow(panel1.GetRenderWindow());
    view1.SetVertexLabelArrayName("name");
    view1.VertexLabelVisibilityOn();
    view1.SetVertexColorArrayName("level");
    view1.ColorVerticesOn();
    // Link the selection.
    rep1.SetSelectionLink(link);

    // Create a standard tree layout view.
    // The standard tree layout arranges the vertices of a tree in horizontal levels.
    // Label by name, color by level.
    view2 = new vtkTreeLayoutView();
    vtkDataRepresentation rep2 = view2.AddRepresentationFromInput(tree);
    view2.SetupRenderWindow(panel2.GetRenderWindow());
    view2.SetLabelArrayName("name");
    view2.LabelVisibilityOn();
    view2.SetVertexColorArrayName("level");
    view2.ColorVerticesOn();
    // Link the selection.
    rep2.SetSelectionLink(link);

    // Create a radial tree layout view.
    // The radial tree layout arranges the vertices of a tree in concentric circles.
    // Label by name, color by level.
    view3 = new vtkTreeLayoutView();
    vtkDataRepresentation rep3 = view3.AddRepresentationFromInput(tree);
    view3.SetupRenderWindow(panel3.GetRenderWindow());
    view3.RadialOn();
    view3.SetAngle(360);
    view3.SetLeafSpacing(0.3);
    view3.SetLabelArrayName("name");
    view3.LabelVisibilityOn();
    view3.SetVertexColorArrayName("level");
    view3.ColorVerticesOn();
    view3.GetRenderer().ResetCamera();
    // Link the selection.
    rep3.SetSelectionLink(link);

    // Create a tree map view.
    view4 = new vtkTreeMapView();
    vtkDataRepresentation rep4 = view4.AddRepresentationFromInput(tree);
    view4.SetupRenderWindow(panel4.GetRenderWindow());
    // Link the selection.
    rep4.SetSelectionLink(link);

    // Use a simple ViewChangedObserver to render all views
    // when the selection in one view changes.
    ViewChangedObserver obs = new ViewChangedObserver();
    rep1.AddObserver("SelectionChangedEvent", obs, "SelectionChanged");
    rep2.AddObserver("SelectionChangedEvent", obs, "SelectionChanged");
    rep3.AddObserver("SelectionChangedEvent", obs, "SelectionChanged");
    rep4.AddObserver("SelectionChangedEvent", obs, "SelectionChanged");

    // Arrange the views in a grid.
    GridLayout layout = new GridLayout(2, 2);
    layout.setHgap(5);
    layout.setVgap(5);
    this.setLayout(layout);
    this.add(panel1);
    this.add(panel2);
    this.add(panel3);
    this.add(panel4);
  }

  private class ViewChangedObserver {
    // When a selection changes, render all views.
    void SelectionChanged() {
      panel1.Render();
      panel2.Render();
      panel3.Render();
      panel4.Render();
    }
  }

  // In the static constructor we load in the native code.
  // The libraries must be in your path to work.
  static {
    System.loadLibrary("vtkCommonJava");
    System.loadLibrary("vtkFilteringJava");
    System.loadLibrary("vtkIOJava");
    System.loadLibrary("vtkImagingJava");
    System.loadLibrary("vtkGraphicsJava");
    System.loadLibrary("vtkRenderingJava");
    System.loadLibrary("vtkInfovisJava");
    System.loadLibrary("vtkViewsJava");
  }

  public static void main(String args[]) {
    TreeViews panel = new TreeViews();
    JFrame frame = new JFrame("Titan Demo");
    frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    frame.getContentPane().add("Center", panel);
    frame.pack();
    frame.setVisible(true);

    frame.addWindowListener(new WindowAdapter() {
      @Override
      public void windowClosing(WindowEvent e) {
        // Calling vtkGlobalJavaHash.DeleteAll() will clean up
        // VTK references before the Java program exits.
        vtkGlobalJavaHash.DeleteAll();
      }
    });
  }

}
