/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSlicePlanePrecision.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtk3DLinearGridPlaneCutter.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkTestUtilities.h>
#include <vtkXMLUnstructuredGridReader.h>

int TestSlicePlanePrecision(int argc, char* argv[])
{
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/slightlyRotated.vtu");
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0.0, 0.0, 0.0);
  plane->SetNormal(0.0, 0.0, 1.0);

  vtkNew<vtk3DLinearGridPlaneCutter> slicer;
  slicer->SetInputConnection(reader->GetOutputPort());
  slicer->SetPlane(plane);
  slicer->Update();

  vtkPolyData* result = vtkPolyData::SafeDownCast(slicer->GetOutput());
  vtkPoints* points = result->GetPoints();

  for (vtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
  {
    double* p = points->GetPoint(i);

    // Z must be exactly 0
    if (p[2] != 0.0)
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
