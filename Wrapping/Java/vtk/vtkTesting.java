package vtk;
import vtk.*;
import java.io.File;

import java.util.Properties;

public class vtkTesting
{
  public static final int FAILED        = 0;
  public static final int PASSED        = 1;
  public static final int NOT_RUN       = 2;
  public static final int DO_INTERACTOR = 3;

  private static void LoadLibrary(String path, String library)
    {
    String lname = System.mapLibraryName(library);
    String libname = path + System.getProperty("file.separator") +
      lname;
    try
      {
      Runtime.getRuntime().load(libname);
      }
    catch (UnsatisfiedLinkError e)
      {
      System.loadLibrary(library);
      }
    }

  public static void Initialize(String[] args)
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
      vtkTesting.LoadLibrary(lpath, "vtk" + kits[cc] + "Java"); 
      }
    }

  public static void Exit(int retVal)
    {
    if ( retVal == vtkTesting.FAILED || retVal == vtkTesting.NOT_RUN )
      {
      System.out.println("Test failed or was not run");
      System.exit(1);
      }
    System.out.println("Test passed");
    System.exit(0);
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
        return vtkTesting.DO_INTERACTOR;
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
          return vtkTesting.NOT_RUN;
          }
        }
      }

    File file = new File(data_path + "/" + image_path);
    if ( !file.exists() )
      {
      System.err.println("File " + file.getName() + " does not exists");
      return vtkTesting.NOT_RUN;
      }

    renWin.Render();

    vtkWindowToImageFilter w2if = new vtkWindowToImageFilter();
    w2if.SetInput(renWin);

    vtkImageDifference imgDiff = new vtkImageDifference();

    vtkPNGReader rtpnm = new vtkPNGReader();
    rtpnm.SetFileName(data_path + "/" + image_path);

    imgDiff.SetInput(w2if.GetOutput());
    imgDiff.SetImage(rtpnm.GetOutput());
    imgDiff.Update();

    if (imgDiff.GetThresholdedError() <= threshold) 
      {
      System.out.println("Java smoke test passed."); 
      return vtkTesting.PASSED;
      } 
    System.out.println("Java smoke test error!"); 
    System.out.println("Image difference: " + imgDiff.GetThresholdedError());
    vtkJPEGWriter wr = new vtkJPEGWriter();
    wr.SetFileName(data_path + "/" + image_path + ".error.jpg");
    wr.SetInput(w2if.GetOutput());
    wr.Write();
    wr.SetFileName(data_path + "/" + image_path + ".diff.jpg");
    wr.SetInput(imgDiff.GetOutput());
    return vtkTesting.FAILED;
    } 
}
