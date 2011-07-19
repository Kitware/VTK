package vtk.sample;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.lang.reflect.InvocationTargetException;
import java.util.concurrent.Callable;
import java.util.concurrent.CompletionService;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JCheckBox;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import javax.swing.Timer;

import vtk.vtkActor;
import vtk.vtkDataSetMapper;
import vtk.vtkNativeLibrary;
import vtk.vtkObject;
import vtk.vtkPanel;
import vtk.vtkReferenceInformation;
import vtk.vtkRenderer;
import vtk.vtkShrinkFilter;
import vtk.vtkSphereSource;

/**
 * This application run concurrently thread that create VTK objects and the EDT
 * that collect those objects and render them.
 *
 * @author sebastien jourdain - sebastien.jourdain@kitware.com
 */
public class Demo extends JPanel {
    private static final long serialVersionUID = 1L;
    private vtkPanel panel3d;
    private JCheckBox runGC;
    private JCheckBox debugMode;
    private JLabel gcStatus;
    private int NUMBER_OF_PIPLINE_TO_BUILD = 120;
    private int nbSeconds;

    private final CompletionService<vtkActor> exec;

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
    public static class AddActorRunnable implements Runnable {
        private vtkActor actorToAdd;
        private vtkRenderer renderer;
        private vtkPanel panel;

        void setRenderer(vtkPanel panel) {
            this.renderer = panel.GetRenderer();
            this.panel = panel;
        }

        void setActor(vtkActor a) {
            this.actorToAdd = a;
        }

        @Override
        public void run() {
            this.renderer.AddActor(this.actorToAdd);
            this.panel.Render();
        }
    }

    // -----------------------------------------------------------------
    public static class PipelineBuilder implements Callable<vtkActor> {
        private vtkActor actor;
        private vtkDataSetMapper mapper;
        private vtkShrinkFilter shrink;
        private vtkSphereSource sphere;

        @Override
        public vtkActor call() throws Exception {
            // New
            actor = new vtkActor();
            mapper = new vtkDataSetMapper();
            shrink = new vtkShrinkFilter();
            sphere = new vtkSphereSource();

            // Settings
            sphere.SetPhiResolution(60);
            sphere.SetThetaResolution(60);
            double[] center = new double[3];
            sphere.SetCenter(GetRandomCenter(center));
            actor.GetProperty().SetColor(Math.random(), Math.random(), Math.random());

            // Binding
            actor.SetMapper(mapper);
            mapper.SetInputConnection(shrink.GetOutputPort());
            shrink.SetInputConnection(sphere.GetOutputPort());

            // Update
            mapper.Update();

            // Wait some time
            Thread.sleep((long) (Math.random() * 500));

            // Return
            return actor;
        }

        public double[] GetRandomCenter(double[] center) {
            for (int i = 0; i < 3; i++) {
                center[i] = Math.random() * 10;
            }
            return center;
        }
    }

    // -----------------------------------------------------------------

    public Demo() {
        super(new BorderLayout());
        panel3d = new vtkPanel();
        gcStatus = new JLabel("");
        runGC = new JCheckBox("Enable GC", false);
        debugMode = new JCheckBox("Debug mode", false);
        exec = new ExecutorCompletionService<vtkActor>(Executors.newFixedThreadPool(Runtime.getRuntime().availableProcessors()));

        // Setup UI
        JPanel statusBar = new JPanel();
        statusBar.setLayout(new BoxLayout(statusBar, BoxLayout.X_AXIS));
        statusBar.add(runGC);
        statusBar.add(debugMode);
        statusBar.add(Box.createHorizontalGlue());
        statusBar.add(gcStatus);
        add(panel3d, BorderLayout.CENTER);
        add(statusBar, BorderLayout.SOUTH);

        // Init app
        // this.setupGC(); // We use a Swing timer that show the result in UI
        // instead..
        this.setupWorkers();

        // Update GC info into the UI every seconds
        // Reset camera each seconds the first 10 ones
        this.nbSeconds = 0;
        new Timer(1000, new ActionListener() {

            @Override
            public void actionPerformed(ActionEvent e) {
                if (nbSeconds++ < 10) {
                    panel3d.resetCamera();
                }
                vtkRenderer renderer = panel3d.GetRenderer();
                if (renderer.GetNumberOfPropsRendered() > 1) {
                    renderer.RemoveActor(renderer.GetActors().GetLastProp());
                }

                // Run GC in local thread (EDT)
                if (runGC.isSelected()) {
                    vtkReferenceInformation info = vtkObject.JAVA_OBJECT_MANAGER.gc(debugMode.isSelected());
                    if (debugMode.isSelected()) {
                        System.out.println(info.listKeptReferenceToString());
                        System.out.println(info.listRemovedReferenceToString());
                    }
                    gcStatus.setText(info.toString());
                } else {
                    gcStatus.setText("");
                }

                panel3d.Render();
            }
        }).start();
    }

    private void setupGC() {
        // Setup GC to run every 1 second in EDT
        vtkObject.JAVA_OBJECT_MANAGER.getAutoGarbageCollector().SetScheduleTime(1, TimeUnit.SECONDS);

        // Start/Stop the GC based on the checkbox
        runGC.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent arg0) {
                vtkObject.JAVA_OBJECT_MANAGER.getAutoGarbageCollector().SetAutoGarbageCollection(runGC.isSelected());
            }
        });

        // Change GC mode based on the checkbox
        debugMode.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent arg0) {
                vtkObject.JAVA_OBJECT_MANAGER.getAutoGarbageCollector().SetDebug(debugMode.isSelected());
            }
        });
    }

    private void setupWorkers() {
        // Add actor thread: Consume the working queue and add the actor into
        // the render inside the EDT thread
        final AddActorRunnable adderRunnable = new AddActorRunnable();
        adderRunnable.setRenderer(panel3d);
        new Thread() {
            public void run() {
                for (int i = 0; i < NUMBER_OF_PIPLINE_TO_BUILD; i++) {
                    try {
                        adderRunnable.setActor(exec.take().get());
                        SwingUtilities.invokeAndWait(adderRunnable);
                        panel3d.repaint();
                    } catch (InterruptedException e) {
                        return;
                    } catch (ExecutionException e) {
                        e.printStackTrace();
                    } catch (InvocationTargetException e) {
                        e.printStackTrace();
                    }
                }
            };
        }.start();
    }

    public void startWorking() {
        for (int i = 0; i < NUMBER_OF_PIPLINE_TO_BUILD; i++) {
            exec.submit(new PipelineBuilder());
        }
    }

    // -----------------------------------------------------------------
    public static void main(String[] args) throws InterruptedException, InvocationTargetException {
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                Demo app = new Demo();

                JFrame f = new JFrame("Concurrency test");
                f.getContentPane().setLayout(new BorderLayout());
                f.getContentPane().add(app, BorderLayout.CENTER);
                f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
                f.setSize(400, 400);
                f.setVisible(true);
                f.validate();

                app.startWorking();
            }
        });
    }
}
