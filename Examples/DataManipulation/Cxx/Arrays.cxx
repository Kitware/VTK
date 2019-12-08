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

// First include the required header files for the vtk classes we are using.
#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <array>

int main()
{
  vtkNew<vtkNamedColors> colors;

  // Create a float array which represents the points.
  vtkNew<vtkDoubleArray> pcoords;

  // Note that by default, an array has 1 component.
  // We have to change it to 3 for points.
  pcoords->SetNumberOfComponents(3);
  // We ask pcoords to allocate room for at least 4 tuples
  // and set the number of tuples to 4.
  pcoords->SetNumberOfTuples(4);
  // Assign each tuple. There are 5 specialized versions of SetTuple:
  // SetTuple1 SetTuple2 SetTuple3 SetTuple4 SetTuple9
  // These take 1, 2, 3, 4 and 9 components respectively.
  std::array<std::array<double, 3>, 4> pts = { { { { 0.0, 0.0, 0.0 } }, { { 0.0, 1.0, 0.0 } },
    { { 1.0, 0.0, 0.0 } }, { { 1.0, 1.0, 0.0 } } } };
  for (auto i = 0ul; i < pts.size(); ++i)
  {
    pcoords->SetTuple(i, pts[i].data());
  }

  // Create vtkPoints and assign pcoords as the internal data array.
  vtkNew<vtkPoints> points;
  points->SetData(pcoords);

  // Create the cells. In this case, a triangle strip with 2 triangles
  // (which can be represented by 4 points).
  vtkNew<vtkCellArray> strips;
  strips->InsertNextCell(4);
  strips->InsertCellPoint(0);
  strips->InsertCellPoint(1);
  strips->InsertCellPoint(2);
  strips->InsertCellPoint(3);

  // Create an integer array with 4 tuples. Note that when using
  // InsertNextValue (or InsertNextTuple1 which is equivalent in
  // this situation), the array will expand automatically.
  vtkNew<vtkIntArray> temperature;
  temperature->SetName("Temperature");
  temperature->InsertNextValue(10);
  temperature->InsertNextValue(20);
  temperature->InsertNextValue(30);
  temperature->InsertNextValue(40);

  // Create a double array.
  vtkNew<vtkDoubleArray> vorticity;
  vorticity->SetName("Vorticity");
  vorticity->InsertNextValue(2.7);
  vorticity->InsertNextValue(4.1);
  vorticity->InsertNextValue(5.3);
  vorticity->InsertNextValue(3.4);

  // Create the dataset. In this case, we create a vtkPolyData
  vtkNew<vtkPolyData> polydata;
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
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(polydata);
  mapper->SetScalarRange(0, 40);

  // Create an actor.
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Create the rendering objects.
  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);
  ren->SetBackground(colors->GetColor3d("DarkSlateGray").GetData());

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  renWin->SetSize(600, 600);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
