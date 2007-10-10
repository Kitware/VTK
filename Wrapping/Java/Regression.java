/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Regression.java

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
import vtk.*;

public class Regression 
{
  public static void main (String []args) 
    {
    vtkTesting2.Initialize(args, true);


    vtkShortArray array = new vtkShortArray();
    array.InsertNextTuple1(3.0);
    array.InsertNextTuple1(1.0);
    array.InsertNextTuple1(4.0);
    array.InsertNextTuple1(1.0);
    array.InsertNextTuple1(5.0);
    array.InsertNextTuple1(9.0);
    array.InsertNextTuple1(2.0);
    array.InsertNextTuple1(6.0);
    array.InsertNextTuple1(5.0);
    array.InsertNextTuple1(3.0);
    array.InsertNextTuple1(5.0);
    array.InsertNextTuple1(8.0);
    array.InsertNextTuple1(9.0);
    array.InsertNextTuple1(7.0);
    array.InsertNextTuple1(9.0);
    array.InsertNextTuple1(3.0);
    array.InsertNextTuple1(1.0);
    short[] carray = array.GetJavaArray();
    int cc;
    System.out.print("[");
    for ( cc = 0; cc < carray.length; cc ++ )
      {
      short i = carray[cc];
      System.out.print(i);
      }
    System.out.println("]");

    vtkUnsignedShortArray narray = new vtkUnsignedShortArray();
    narray.SetJavaArray(carray);
    System.out.print("[");
    for ( cc = 0; cc <= narray.GetMaxId(); cc ++ )
      {
      int i = narray.GetValue(cc);
      System.out.print(i);
      }
    System.out.println("]");

    vtkRenderWindow renWin = new vtkRenderWindow();
    vtkRenderer ren1 = new vtkRenderer();
    renWin.AddRenderer(ren1);
    vtkRenderWindowInteractor iren = new vtkRenderWindowInteractor();
    iren.SetRenderWindow(renWin);
    vtkConeSource cone = new vtkConeSource();
    cone.SetResolution(8);
    vtkPolyDataMapper coneMapper = new vtkPolyDataMapper();
    coneMapper.SetInput(cone.GetOutput());

    vtkActor coneActor = new vtkActor();
    coneActor.SetMapper(coneMapper);

    ren1.AddActor(coneActor);
    renWin.Render();
    vtkWindowToImageFilter w2i = new vtkWindowToImageFilter();
    w2i.SetInput(renWin);
    w2i.Modified();
    renWin.Render();
    w2i.Update();
    vtkImageData image = w2i.GetOutput();

    vtkUnsignedCharArray da = (vtkUnsignedCharArray)image.GetPointData().GetScalars();
    byte[] barray = da.GetJavaArray();

    System.out.println("Length of array: " + barray.length);

    vtkUnsignedCharArray nda = new vtkUnsignedCharArray();
    nda.SetJavaArray(barray);

    vtkImageData nimage = new vtkImageData();
    nimage.SetDimensions(image.GetDimensions());
    nimage.SetSpacing(image.GetSpacing());
    nimage.SetOrigin(image.GetOrigin());
    nimage.SetScalarType(image.GetScalarType());
    nimage.SetNumberOfScalarComponents(image.GetNumberOfScalarComponents());
    nimage.AllocateScalars();
    vtkUnsignedCharArray nida = (vtkUnsignedCharArray)nimage.GetPointData().GetScalars();
    nida.SetJavaArray(barray);

    int retVal0 = vtkTesting2.PASSED;

    for ( cc = 0; cc <= da.GetMaxId(); cc ++ )
      {
      int v1=0, v2=0, v3=0;
      v1 = da.GetValue(cc);
      if ( cc <= nda.GetMaxId() )
        {
        v2 = nda.GetValue(cc);
        }
      else
        {
        System.out.println("Cannot find point " + cc + " in nda");
        retVal0 = vtkTesting2.FAILED;
        }
      if ( cc <= nida.GetMaxId() )
        {
        v3 = nida.GetValue(cc);
        }
      else
        {
        System.out.println("Cannot find point " + cc + " in nida");
        retVal0 = vtkTesting2.FAILED;
        }
      if ( v1 != v2 || v1 != v3 )
        {
        System.out.println("Wrong point: " + v1 + " <> " + v2 + " <> " + v3);
        retVal0 = vtkTesting2.FAILED;
        }
      }

    vtkImageDifference imgDiff = new vtkImageDifference();
    imgDiff.SetInput(nimage);
    imgDiff.SetImage(image);
    imgDiff.Update();

    int retVal1 = vtkTesting2.PASSED;
    if ( imgDiff.GetThresholdedError() != 0 )
      {
      System.out.println("Problem with array conversion. Image difference is: " + imgDiff.GetThresholdedError());
      vtkPNGWriter wr = new vtkPNGWriter();
      wr.SetInput(image);
      wr.SetFileName("im1.png");
      wr.Write();
      wr.SetInput(nimage);
      wr.SetFileName("im2.png");
      wr.Write();
      wr.SetInput(imgDiff.GetOutput());
      wr.SetFileName("diff.png");
      wr.Write();
      retVal1 = vtkTesting2.FAILED;
      }

    int retVal2 = vtkTesting2.PASSED;
    if ( vtkTesting2.IsInteractive() )
      {
      iren.Start();
      }
    else
      {
      retVal2 = vtkTesting2.RegressionTest(renWin, 10);
      }

    vtkGlobalJavaHash.DeleteAll();
    if ( retVal0 != vtkTesting2.PASSED )
      {
      vtkTesting2.Exit(retVal0);
      }
    if ( retVal1 != vtkTesting2.PASSED )
      {
      vtkTesting2.Exit(retVal1);
      }
    vtkTesting2.Exit(retVal2);
    }
}

