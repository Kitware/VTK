package vtk;

import java.io.File;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import vtk.vtkObject;
import vtk.vtkRenderWindow;
import vtk.vtkSettings;
import vtk.vtkTesting;

public class vtkJavaTesting {
  public static final int FAILED = 0;
  public static final int PASSED = 1;
  public static final int NOT_RUN = 2;
  public static final int DO_INTERACTOR = 3;

  private static int LoadLib(String lib, boolean verbose) {
    try {
      if (verbose) {
        System.out.println("Try to load: " + lib);
      }

      if(!new File(lib).exists()) {
        if(verbose) {
          System.out.println("File does not exist: " + lib);
          return 0;
        }
      }

      Runtime.getRuntime().load(lib);
    } catch (UnsatisfiedLinkError e) {
      if (verbose) {
        System.out.println("Failed to load: " + lib);
        e.printStackTrace();
      }
      return 0;
    }
    if (verbose) {
      System.out.println("Successfully loaded: " + lib);
    }
    return 1;
  }

  private static void LoadLibrary(String path, String library, boolean verbose) {
    String lname = System.mapLibraryName(library);
    String sep = System.getProperty("file.separator");
    String libname = path + sep + lname;
    String releaselibname = path + sep + "Release" + sep + lname;
    String debuglibname = path + sep + "Debug" + sep + lname;
    if (vtkJavaTesting.LoadLib(library, verbose) != 1 //
        && vtkJavaTesting.LoadLib(libname, verbose) != 1
        && vtkJavaTesting.LoadLib(releaselibname, verbose) != 1
        && vtkJavaTesting.LoadLib(debuglibname, verbose) != 1) {
      System.out.println("Problem loading appropriate library");
    }
  }

  public static void Initialize(String[] args) {
    vtkJavaTesting.Initialize(args, false);
  }

  public static void Initialize(String[] args, boolean verbose) {
    String lpath = vtkSettings.GetVTKLibraryDir();
    if (lpath != null) {
      String path_separator = System.getProperty("path.separator");
      String s = System.getProperty("java.library.path");
      s = s + path_separator + lpath;
      System.setProperty("java.library.path", s);
    }
    // String lname = System.mapLibraryName("vtkCommonJava");
    String[] kits = vtkSettings.GetKits();
    int cc;
    for (cc = 0; cc < kits.length; cc++) {
      vtkJavaTesting.LoadLibrary(lpath, "vtk" + kits[cc] + "Java", verbose);
    }
    vtkJavaTesting.Tester = new vtk.vtkTesting();
    for (cc = 0; cc < args.length; cc++) {
      vtkJavaTesting.Tester.AddArgument(args[cc]);
    }
  }

  public static boolean IsInteractive() {
    if (vtkJavaTesting.Tester.IsInteractiveModeSpecified() == 0) {
      return false;
    }
    return true;
  }

  public static void Exit(int retVal) {
    vtkJavaTesting.Tester = null;
    System.gc();
    vtkObjectBase.JAVA_OBJECT_MANAGER.gc(true);

    if (retVal == vtkJavaTesting.FAILED || retVal == vtkJavaTesting.NOT_RUN) {
      System.out.println("Test failed or was not run");
      System.exit(1);
    }
    System.out.println("Test passed");
    System.exit(0);
  }

  public static int RegressionTest(vtkRenderWindow renWin, int threshold) {
    vtkJavaTesting.Tester.SetRenderWindow(renWin);

    if (vtkJavaTesting.Tester.RegressionTest(threshold) == vtkJavaTesting.PASSED) {
      return vtkJavaTesting.PASSED;
    }
    System.out.println("Image difference: " + vtkJavaTesting.Tester.GetImageDifference());
    return vtkJavaTesting.FAILED;
  }

  public static void StartTimeoutExit(long time, TimeUnit unit) {
    ScheduledExecutorService killerThread = Executors.newSingleThreadScheduledExecutor();
    Runnable killer = new Runnable() {
      public void run() {
        System.exit(0);
      }
    };
    killerThread.schedule(killer, time, unit);
  }

  public static vtkJavaGarbageCollector StartGCInEDT(long time, TimeUnit unit) {
    vtkJavaGarbageCollector gc = vtkObjectBase.JAVA_OBJECT_MANAGER.getAutoGarbageCollector();
    gc.SetScheduleTime(time, unit);
    gc.SetAutoGarbageCollection(true);
    return gc;
  }

  private static vtkTesting Tester = null;
}
