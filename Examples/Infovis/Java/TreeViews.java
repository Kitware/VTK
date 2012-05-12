import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.beans.PropertyChangeListener;
import java.io.File;
import java.util.Random;

import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JPopupMenu;
import javax.swing.KeyStroke;
import javax.swing.filechooser.FileNameExtensionFilter;

import vtk.*;

public class TreeViews extends JFrame {

  private vtkRenderWindowPanel panel1;
  private vtkRenderWindowPanel panel2;
  private vtkRenderWindowPanel panel3;
  private vtkRenderWindowPanel panel4;
  
  private vtkXMLTreeReader reader = new vtkXMLTreeReader();
  private vtkTreeLevelsFilter levels = new vtkTreeLevelsFilter();

  private vtkGraphLayoutView view1;
  private vtkGraphLayoutView view2;
  private vtkGraphLayoutView view3;
  private vtkTreeMapView view4;

  public TreeViews() {
    // Create a random tree
    // The tree has a "name" field to use for labels.
    // It also has a "level" field to use for colors.
    vtkStringArray nameArr = new vtkStringArray();
    nameArr.SetName("name");
    // Start by adding a root to the tree
    vtkMutableDirectedGraph g = new vtkMutableDirectedGraph();
    g.AddVertex();
    nameArr.InsertNextValue("0");
    Random r = new Random();
    for (int i = 1; i < 100; ++i) {
      // Add a child to a random vertex in the tree.
      g.AddChild(r.nextInt(i));
      nameArr.InsertNextValue("" + i);
    }
    g.GetVertexData().AddArray(nameArr);
    
    vtkTree tree = new vtkTree();
    tree.ShallowCopy(g);
    levels.SetInput(tree);

    // Create the selection link.
    // This is shared among all views in order to link the selection.
    vtkAnnotationLink link = new vtkAnnotationLink();

    // Create a graph layout view.
    // This view performs an automatic embedding of the graph in 2D.
    // Label by name, color by level.
    view1 = new vtkGraphLayoutView();
    vtkDataRepresentation rep1 = view1.AddRepresentationFromInputConnection(levels.GetOutputPort());
    panel1 = new vtkRenderWindowPanel(view1.GetRenderWindow());
    view1.SetVertexLabelArrayName("name");
    view1.VertexLabelVisibilityOn();
    view1.SetVertexColorArrayName("level");
    view1.ColorVerticesOn();
    view1.ResetCamera();
    // Link the selection.
    rep1.SetAnnotationLink(link);

    // Create a standard tree layout view.
    // The standard tree layout arranges the vertices of a tree in horizontal
    // levels.
    // Label by name, color by level.
    view2 = new vtkGraphLayoutView();
    vtkDataRepresentation rep2 = view2.AddRepresentationFromInputConnection(levels.GetOutputPort());
    panel2 = new vtkRenderWindowPanel(view2.GetRenderWindow());
    view2.SetVertexLabelArrayName("name");
    view2.VertexLabelVisibilityOn();
    view2.SetVertexColorArrayName("level");
    view2.ColorVerticesOn();
    view2.ResetCamera();
    // Link the selection.
    rep2.SetAnnotationLink(link);

    // Create a radial tree layout view.
    // The radial tree layout arranges the vertices of a tree in concentric
    // circles.
    // Label by name, color by level.
    view3 = new vtkGraphLayoutView();
    vtkDataRepresentation rep3 = view3.AddRepresentationFromInputConnection(levels.GetOutputPort());
    panel3 = new vtkRenderWindowPanel(view3.GetRenderWindow());
    vtkTreeLayoutStrategy layout3 = new vtkTreeLayoutStrategy();
    layout3.RadialOn();
    layout3.SetAngle(360);
    layout3.SetLeafSpacing(0.3);
    view3.SetLayoutStrategy(layout3);
    view3.SetVertexLabelArrayName("name");
    view3.VertexLabelVisibilityOn();
    view3.SetVertexColorArrayName("level");
    view3.ColorVerticesOn();
    view3.ResetCamera();
    // Link the selection.
    rep3.SetAnnotationLink(link);

    // Create a tree map view.
    view4 = new vtkTreeMapView();
    vtkDataRepresentation rep4 = view4.AddRepresentationFromInputConnection(levels.GetOutputPort());
    panel4 = new vtkRenderWindowPanel(view4.GetRenderWindow());
    // Link the selection.
    rep4.SetAnnotationLink(link);
    view4.ResetCamera();

    // Use a simple ViewChangedObserver to render all views
    // when the selection in one view changes.
    ViewChangedObserver obs = new ViewChangedObserver();
    link.AddObserver("SelectionChangedEvent", obs, "SelectionChanged");

    // Arrange the views in a grid.
    GridLayout layout = new GridLayout(2, 2);
    layout.setHgap(5);
    layout.setVgap(5);
    this.setLayout(layout);
    this.add(panel1);
    this.add(panel2);
    this.add(panel3);
    this.add(panel4);

    // Create menu
    JMenuBar menuBar;
    JMenu menu;
    JMenuItem menuItem;

    // Create the menu bar.
    menuBar = new JMenuBar();

    // Build the first menu.
    menu = new JMenu("File");
    menu.setMnemonic(KeyEvent.VK_F);
    menuBar.add(menu);

    // A JMenuItem
    menuItem = new JMenuItem("Open XML...", KeyEvent.VK_X);
    menuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_O,
        ActionEvent.CTRL_MASK));
    menuItem.addActionListener(new ActionListener() {

      public void actionPerformed(ActionEvent e) {
        // Create a file chooser
        final JFileChooser fc = new JFileChooser();
        FileNameExtensionFilter filter = new FileNameExtensionFilter("XML files (*.xml)", "xml");
        fc.setFileFilter(filter);
        // In response to a button click:
        int returnVal = fc.showOpenDialog(null);
        if (returnVal == JFileChooser.APPROVE_OPTION) {
          File f = fc.getSelectedFile();
          reader.SetFileName(f.getAbsolutePath());
          levels.SetInputConnection(reader.GetOutputPort());
          ViewChangedObserver obs = new ViewChangedObserver();
          obs.SelectionChanged();
        }
      }});
    menu.add(menuItem);

    this.setJMenuBar(menuBar);
  }

  private class ViewChangedObserver {
    // When a selection changes, render all views.
    void SelectionChanged() {
      view1.Update();
      view2.Update();
      view3.Update();
      view4.Update();
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
    JPopupMenu.setDefaultLightWeightPopupEnabled(false);

    TreeViews app = new TreeViews();
    app.setTitle("Tree Views");
    app.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    app.pack();
    app.setVisible(true);

    app.addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        // Calling vtkGlobalJavaHash.DeleteAll() will clean up
        // VTK references before the Java program exits.
        vtkGlobalJavaHash.DeleteAll();
      }
    });
  }

}
