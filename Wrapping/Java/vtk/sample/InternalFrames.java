package vtk.sample;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.event.WindowListener;

import javax.swing.AbstractAction;
import javax.swing.JDesktopPane;
import javax.swing.JFrame;
import javax.swing.JInternalFrame;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JPopupMenu;
import javax.swing.JSplitPane;
import javax.swing.JTabbedPane;
import javax.swing.SwingUtilities;

import vtk.vtkNativeLibrary;

public class InternalFrames extends JFrame {
  private static final long serialVersionUID = 1L;
  private Desktop theDesktop;
  private Dimension screenSize;

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

  public InternalFrames() {
    super("VTK Internal Frame Demo");
    screenSize = Toolkit.getDefaultToolkit().getScreenSize();
    this.setSize(900, 900);

    WindowListener l = new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        System.exit(0);
      }
    };
    this.addWindowListener(l);

    this.getContentPane().add(new SplitFrame());

    new MenuMgr();

    this.setVisible(true);
  }

  public void addMenuBar(JMenuBar m) {
    setJMenuBar(m);
  }

  private class SplitFrame extends JSplitPane {
    private static final long serialVersionUID = 1L;

    public SplitFrame() {
      super(JSplitPane.VERTICAL_SPLIT);
      this.setDividerLocation(screenSize.height / 2);
      setContinuousLayout(true);
      setOneTouchExpandable(true);
      add(theDesktop = new Desktop());
      add(new Tabbed());
    }
  }

  private class Desktop extends JDesktopPane {
    private static final long serialVersionUID = 1L;

    public Desktop() {
      super();
      this.setPreferredSize(screenSize);
      this.setDragMode(JDesktopPane.OUTLINE_DRAG_MODE);
      this.add(new VTKFrame(10, 10));
      this.add(new VTKFrame(500, 10));
    }
  }

  private class Tabbed extends JTabbedPane {
    private static final long serialVersionUID = 1L;

    public Tabbed() {
      this.addTab("vtk1", new VTKCanvas());
      this.addTab("vtk2", new VTKCanvas());

      setMinimumSize(new Dimension(300, 300));
      // another swing bug work around
      // for some reason the first time the tabbed item is drawn,
      // tab 0 is selected but the sphere (tab 1) is drawn
      // manually selecting the tabs synchs the tab and the draw order
      // it is harmless but it is still annoying
      // so we force tab 1 to be selected and hide the bug from the user
      // note that selecting 0 does not work....
      this.setSelectedIndex(1);
    }
  }

  private class VTKFrame extends JInternalFrame {
    private static final long serialVersionUID = 1L;

    public VTKFrame(int x, int y) {
      super("VTK Window", true, true, true, true);
      Dimension mySize = new Dimension();
      mySize.height = 300;
      mySize.width = 300;
      this.setSize(mySize);
      this.getContentPane().setLayout(new BorderLayout());
      this.setLocation(x, y);

      this.getContentPane().add(new VTKCanvas(), BorderLayout.CENTER);
      this.pack();
      this.setVisible(true);
    }
  }

  private class MenuMgr extends JMenuBar {
    private static final long serialVersionUID = 1L;
    private JMenu menu;

    public MenuMgr() {
      super();
      JPopupMenu.setDefaultLightWeightPopupEnabled(false);

      menu = new JMenu("File");
      menu.add(new CreateWindowAction("Create New VTK Window"));
      menu.add(new KillAction("Exit"));
      add(menu);

      addMenuBar(this);
    }
  }

  private class CreateWindowAction extends AbstractAction {
    private static final long serialVersionUID = 1L;
    private int layer = 0;

    public CreateWindowAction(String label) {
      super(label);
    }

    public void actionPerformed(ActionEvent ev) {
      theDesktop.add(new VTKFrame(340, 200), new Integer(layer));
    }
  }

  private class KillAction extends AbstractAction {
    private static final long serialVersionUID = 1L;

    public KillAction(String label) {
      super(label);
    }

    public void actionPerformed(ActionEvent ev) {
      System.exit(0);
    }
  }

  public static void main(String[] args) {
    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        new InternalFrames();
      }
    });
  }
}
