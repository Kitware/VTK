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
   
  public static String ExpandDataFileName(String[] args, String path)
    {
    return vtkTesting2.ExpandDataFileName(args, path, false);
    }

  public static String ExpandDataFileName(String[] args, String path, boolean slash)
    {
    return vtkTesting2.ExpandFileNameWithArgOrDefault(
      "-D", args, 
      "../../../../VTKData",
      path, slash);
    }

  public static String GetArgOrDefault(String arg, String[] args, String def)
    {
    int index = -1;
    int cc;
    for ( cc = 0; cc < args.length; cc ++ )
      {
      if ( args[cc].equals(arg) && cc < args.length - 1 )
        {
        index = cc + 1;
        }
      }
    String value = null;
    if ( index > -1 )
      {
      value = args[index];
      }
    else
      {
      value = def;
      }
    return value;
    }

    private static String ExpandFileNameWithArgOrDefault(String arg, 
                                                         String[] args, 
                                                         String def, 
                                                         String fname,
                                                         boolean slash)
    {
    String fullName = null;
    String value = vtkTesting2.GetArgOrDefault(arg, args, def);
    if (value != null)
      {
      fullName = value;
      fullName = fullName + "/";
      fullName = fullName + fname;
      }
    else
      {
      fullName = fname;
      }

    if (slash)
      {
      fullName = fullName + "/";
      }

    return fullName;
    }

  public static int RegressionTestImage( vtkRenderWindow renWin, String[] args,
    int threshold )
    {
    String image_path = null;
    String data_path = null;
    int cc;
    boolean last = false;
    for ( cc = 0; cc < args.length; cc ++ )
      {
      if( cc == args.length )
        {
        last = true;
        }
      if ( args[cc].equals("-I") )
        {
        System.out.println("Interactive mode");
        return vtkTesting2.DO_INTERACTOR;
        }
      if ( args[cc].equals("-D") )
        {
        if ( !last )
          {
          data_path = args[cc+1];
          System.out.println("Set data path to: " + data_path);
          cc++;
          }
        else
          {
          System.err.println("Data path not specified");
          System.exit(1);
          }
        }
      if ( args[cc].equals("-V") )
        {
        if ( !last )
          {
          image_path = args[cc+1];
          System.out.println("Set image path to: " + image_path);
          cc++;
          }
        else
          {
          System.err.println("Image path not specified");
          return vtkTesting2.NOT_RUN;
          }
        }
      }

    File file = new File(data_path + "/" + image_path);
    if ( !file.exists() )
      {
      System.err.println("File " + file.getName() + " does not exists");
      return vtkTesting2.NOT_RUN;
      }
    vtkTesting tt = new vtkTesting();
    tt.SetDataFileName(data_path + "/" + image_path);
    tt.SetRenderWindow(renWin);

    if (tt.RegressionTest(threshold) == vtkTesting2.PASSED ) 
      {
      return vtkTesting2.PASSED;
      } 
    System.out.println("Image difference: " + tt.GetImageDifference());
    return vtkTesting2.FAILED;
    } 
}
