/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Arrays.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrate the use of VTK data arrays as attribute
// data as well as field data. It creates geometry (vtkPolyData) as
// well as attribute data explicitly.

// first include the required header files for the vtk classes we are using
#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int main()
{
  int i;

  // Create a float array which represents the points.
  vtkFloatArray* pcoords = vtkFloatArray::New();

  // Note that by default, an array has 1 component.
  // We have to change it to 3 for points
  pcoords->SetNumberOfComponents(3);
  // We ask pcoords to allocate room for at least 4 tuples
  // and set the number of tuples to 4.
  pcoords->SetNumberOfTuples(4);
  // Assign each tuple. There are 5 specialized versions of SetTuple:
  // SetTuple1 SetTuple2 SetTuple3 SetTuple4 SetTuple9
  // These take 1, 2, 3, 4 and 9 components respectively.
  float pts[4][3] = { {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
                      {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0} };
  for (i=0; i<4; i++)
    {
    pcoords->SetTuple(i, pts[i]);
    }

  // Create vtkPoints and assign pcoords as the internal data array.
  vtkPoints* points = vtkPoints::New();
  points->SetData(pcoords);

  // Create the cells. In this case, a triangle strip with 2 triangles
  // (which can be represented by 4 points)
  vtkCellArray* strips = vtkCellArray::New();
  strips->InsertNextCell(4);
  strips->InsertCellPoint(0);
  strips->InsertCellPoint(1);
  strips->InsertCellPoint(2);
  strips->InsertCellPoint(3);

  // Create an integer array with 4 tuples. Note that when using
  // InsertNextValue (or InsertNextTuple1 which is equivalent in
  // this situation), the array will expand automatically
  vtkIntArray* temperature = vtkIntArray::New();
  temperature->SetName("Temperature");
  temperature->InsertNextValue(10);
  temperature->InsertNextValue(20);
  temperature->InsertNextValue(30);
  temperature->InsertNextValue(40);

  // Create a double array.
  vtkDoubleArray* vorticity = vtkDoubleArray::New();
  vorticity->SetName("Vorticity");
  vorticity->InsertNextValue(2.7);
  vorticity->InsertNextValue(4.1);
  vorticity->InsertNextValue(5.3);
  vorticity->InsertNextValue(3.4);

  // Create the dataset. In this case, we create a vtkPolyData
  vtkPolyData* polydata = vtkPolyData::New();
  // Assign points and cells
  polydata->SetPoints(points);
  polydata->SetStrips(strips);
  // Assign scalars
  polydata->GetPointData()->SetScalars(temperature);
  // Add the vorticity array. In this example, this field
  // is not used.
  polydata->GetPointData()->AddArray(vorticity);

  // Create the mapper and set the appropriate scalar range
  // (default is (0,1)
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputData(polydata);
  mapper->SetScalarRange(0, 40);

  // Create an actor.
  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  // Create the rendering objects.
  vtkRenderer* ren = vtkRenderer::New();
  ren->AddActor(actor);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  iren->Initialize();
  iren->Start();

  pcoords->Delete();
  points->Delete();
  strips->Delete();
  temperature->Delete();
  vorticity->Delete();
  polydata->Delete();
  mapper->Delete();
  actor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();
  return 0;
}


