package vtk;
import java.io.File;

import java.util.Properties;

public class vtkTesting2
{
  public static final int FAILED        = 0;
  public static final int PASSED        = 1;
  public static final int NOT_RUN       = 2;
  public static final int DO_INTERACTOR = 3;

  private static int LoadLib(String lib, boolean verbose)
    {
    try
      {
      if ( verbose )
        {
        System.out.println("Try to load: " + lib);
        }
      Runtime.getRuntime().load(lib);
      }
    catch (UnsatisfiedLinkError e)
      {
      if ( verbose )
        {
        System.out.println("Failed to load: " + lib);
        }
      return 0;
      }
    if ( verbose )
      {
      System.out.println("Successfully loaded: " + lib);
      }
    return 1;
    }

  private static void LoadLibrary(String path, String library, boolean verbose)
    {
    String lname = System.mapLibraryName(library);
    String sep = System.getProperty("file.separator");
    String libname = path + sep + lname;
    String releaselibname = path + sep + "Release" + sep + lname;
    String debuglibname = path + sep + "Debug" + sep + lname;
    if ( vtkTesting2.LoadLib(library, verbose) != 1 &&
      vtkTesting2.LoadLib(libname, verbose) != 1 &&
      vtkTesting2.LoadLib(releaselibname, verbose) != 1 &&
      vtkTesting2.LoadLib(debuglibname, verbose) != 1 )
      {
      System.out.println("Problem loading apropriate library");
      }
    }

  public static void Initialize(String[] args)
    {
    vtkTesting2.Initialize(args, false);
    }

  public static void Initialize(String[] args, boolean verbose)
    {
    String lpath = vtkSettings.GetVTKLibraryDir();
    if ( lpath != null )
      {
      String path_separator = System.getProperty("path.separator");
      String s = System.getProperty("java.library.path");
      s = s + path_separator + lpath;
      System.setProperty("java.library.path", s);
      }
    String lname = System.mapLibraryName("vtkCommonJava");
    String[] kits = vtkSettings.GetKits();
    int cc;
    for ( cc = 0; cc < kits.length; cc ++ )
      {
      vtkTesting2.LoadLibrary(lpath, "vtk" + kits[cc] + "Java", verbose); 
      }
    vtkTesting2.Tester = new vtk.vtkTesting();
    for ( cc = 0; cc < args.length; cc ++ )
      {
      vtkTesting2.Tester.AddArgument(args[cc]);
      }
    }

  public static boolean IsInteractive()
    {
    if ( vtkTesting2.Tester.IsInteractiveModeSpecified() == 0 )
      {
      return false;
      }
    return true;
    }

  public static void Exit(int retVal)
    {
    if ( retVal == vtkTesting2.FAILED || retVal == vtkTesting2.NOT_RUN )
      {
      System.out.println("Test failed or was not run");
      System.exit(1);
      }
    System.out.println("Test passed");
    System.exit(0);
    }

  public static int RegressionTest( vtkRenderWindow renWin, int threshold )
    {
    vtkTesting2.Tester.SetRenderWindow(renWin);

    if (vtkTesting2.Tester.RegressionTest(threshold) == vtkTesting2.PASSED ) 
      {
      return vtkTesting2.PASSED;
      } 
    System.out.println("Image difference: " + vtkTesting2.Tester.GetImageDifference());
    return vtkTesting2.FAILED;
    }


  private static vtkTesting Tester = null;
}
