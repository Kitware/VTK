package vtk.test;

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
import javax.swing.SwingUtilities;

import vtk.vtkActor;
import vtk.vtkConeSource;
import vtk.vtkImageData;
import vtk.vtkImageDifference;
import vtk.vtkJavaTesting;
import vtk.vtkObject;
import vtk.vtkPNGWriter;
import vtk.vtkPolyDataMapper;
import vtk.vtkRenderWindow;
import vtk.vtkRenderWindowInteractor;
import vtk.vtkRenderer;
import vtk.vtkShortArray;
import vtk.vtkUnsignedCharArray;
import vtk.vtkUnsignedShortArray;
import vtk.vtkWindowToImageFilter;

public class Regression {

    public static void main(final String[] args) {
        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                vtkJavaTesting.Initialize(args, true);
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
                for (cc = 0; cc < carray.length; cc++) {
                    short i = carray[cc];
                    System.out.print(i);
                }
                System.out.println("]");

                vtkUnsignedShortArray narray = new vtkUnsignedShortArray();
                narray.SetJavaArray(carray);
                System.out.print("[");
                for (cc = 0; cc <= narray.GetMaxId(); cc++) {
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
                coneMapper.SetInputConnection(cone.GetOutputPort());

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

                vtkUnsignedCharArray da = (vtkUnsignedCharArray) image.GetPointData().GetScalars();
                byte[] barray = da.GetJavaArray();

                System.out.println("Length of array: " + barray.length);

                vtkUnsignedCharArray nda = new vtkUnsignedCharArray();
                nda.SetJavaArray(barray);

                vtkImageData nimage = new vtkImageData();
                nimage.SetDimensions(image.GetDimensions());
                nimage.SetSpacing(image.GetSpacing());
                nimage.SetOrigin(image.GetOrigin());
                nimage.AllocateScalars(image.GetScalarType(), image.GetNumberOfScalarComponents());
                vtkUnsignedCharArray nida = (vtkUnsignedCharArray) nimage.GetPointData().GetScalars();
                nida.SetJavaArray(barray);

                int retVal0 = vtkJavaTesting.PASSED;

                for (cc = 0; cc <= da.GetMaxId(); cc++) {
                    int v1 = 0, v2 = 0, v3 = 0;
                    v1 = da.GetValue(cc);
                    if (cc <= nda.GetMaxId()) {
                        v2 = nda.GetValue(cc);
                    } else {
                        System.out.println("Cannot find point " + cc + " in nda");
                        retVal0 = vtkJavaTesting.FAILED;
                    }
                    if (cc <= nida.GetMaxId()) {
                        v3 = nida.GetValue(cc);
                    } else {
                        System.out.println("Cannot find point " + cc + " in nida");
                        retVal0 = vtkJavaTesting.FAILED;
                    }
                    if (v1 != v2 || v1 != v3) {
                        System.out.println("Wrong point: " + v1 + " <> " + v2 + " <> " + v3);
                        retVal0 = vtkJavaTesting.FAILED;
                    }
                }

                vtkImageDifference imgDiff = new vtkImageDifference();
                imgDiff.SetInputData(nimage);
                imgDiff.SetImageConnection(w2i.GetOutputPort());
                imgDiff.Update();

                int retVal1 = vtkJavaTesting.PASSED;
                if (imgDiff.GetThresholdedError() != 0) {
                    System.out.println("Problem with array conversion. Image difference is: " + imgDiff.GetThresholdedError());
                    vtkPNGWriter wr = new vtkPNGWriter();
                    wr.SetInputConnection(w2i.GetOutputPort());
                    wr.SetFileName("im1.png");
                    wr.Write();
                    wr.SetInputData(nimage);
                    wr.SetFileName("im2.png");
                    wr.Write();
                    wr.SetInputConnection(imgDiff.GetOutputPort());
                    wr.SetFileName("diff.png");
                    wr.Write();
                    retVal1 = vtkJavaTesting.FAILED;
                }

                int retVal2 = vtkJavaTesting.PASSED;
                if (vtkJavaTesting.IsInteractive()) {
                    iren.Start();
                } else {
                    retVal2 = vtkJavaTesting.RegressionTest(renWin, 10);
                }

                vtkObject.JAVA_OBJECT_MANAGER.deleteAll();

                if (retVal0 != vtkJavaTesting.PASSED) {
                    vtkJavaTesting.Exit(retVal0);
                }
                if (retVal1 != vtkJavaTesting.PASSED) {
                    vtkJavaTesting.Exit(retVal1);
                }
                vtkJavaTesting.Exit(retVal2);
            }
        });
    }
}
