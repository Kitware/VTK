/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Regression.java
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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
    vtkTesting.Initialize(args);

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

    int retVal = vtkTesting.RegressionTestImage(renWin, args, 10);
    if ( retVal == vtkTesting.DO_INTERACTOR )
      {
      iren.Start();
      }
    System.exit(retVal);
    }
}

