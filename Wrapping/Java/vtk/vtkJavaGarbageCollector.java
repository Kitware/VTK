package vtk;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import javax.swing.SwingUtilities;

/**
 * This class helps to provide a solution in concurrent garbage collection issue
 * with VTK. This class allow automatic garbage collection done in a specific
 * thread such as the EDT.
 *
 * @author sebastien jourdain - sebastien.jourdain@kitware.com
 */
public class vtkJavaGarbageCollector {

    private ScheduledExecutorService executor;
    private Runnable deleteRunnable;
    private Runnable deferredEdtRunnable;
    private long periodTime;
    private TimeUnit timeUnit;
    private boolean autoCollectionRunning;
    private boolean debug;

    /**
     * Build a garbage collector which is configured to garbage collect every
     * seconds but has not been started yet. The user has to call
     * SetAutoGarbageCollection(true) to make it start.
     */
    public vtkJavaGarbageCollector() {
        // Default settings
        debug = false;
        periodTime = 1;
        timeUnit = TimeUnit.SECONDS;
        autoCollectionRunning = false;
        //
        executor = Executors.newSingleThreadScheduledExecutor();
        deleteRunnable = new Runnable() {

            public void run() {
                // Do the delete here
                vtkReferenceInformation info = vtkObjectBase.JAVA_OBJECT_MANAGER.gc(debug);
                if (debug) {
                    System.out.println(info);
                    System.out.println(info.listKeptReferenceToString());
                    System.out.println(info.listRemovedReferenceToString());
                }
            }
        };
        deferredEdtRunnable = new Runnable() {

            public void run() {
                SwingUtilities.invokeLater(deleteRunnable);
            }
        };
    }

    /**
     * Set the schedule time that should be used to send a garbage collection
     * request to the EDT.
     *
     * @param period
     * @param timeUnit
     */
    public void SetScheduleTime(long period, TimeUnit timeUnit) {
        this.periodTime = period;
        this.timeUnit = timeUnit;
        SetAutoGarbageCollection(autoCollectionRunning);
    }

    /**
     * Whether to print out when garbage collection is run.
     *
     * @param debug
     */
    public void SetDebug(boolean debug) {
        this.debug = debug;
    }

    /**
     * Start or stop the automatic garbage collection in the EDT.
     *
     * @param doGarbageCollectionInEDT
     */
    public void SetAutoGarbageCollection(boolean doGarbageCollectionInEDT) {
        autoCollectionRunning = doGarbageCollectionInEDT;
        executor.shutdown();
        if (doGarbageCollectionInEDT) {
            executor = Executors.newSingleThreadScheduledExecutor();
            executor.scheduleAtFixedRate(deferredEdtRunnable, periodTime, periodTime, timeUnit);
        }
    }

    /**
     * Shortcut for SetAutoGarbageCollection(true)
     * @see SetAutoGarbageCollection
     */
    public void Start() {
        this.SetAutoGarbageCollection(true);
    }

    /**
     * Shortcut for SetAutoGarbageCollection(false)
     * @see SetAutoGarbageCollection
     */
    public void Stop() {
        this.SetAutoGarbageCollection(false);
    }

    /**
     * @return the runnable that do the garbage collection. This could be used
     *         if you want to execute the garbage collection in another thread
     *         than the EDT.
     */
    public Runnable GetDeleteRunnable() {
        return deleteRunnable;
    }
}
