/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyhedron1.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPolyhedron.h"
#include "vtkPlane.h"

#include "vtkTestUtilities.h"
#include "vtkNew.h"
#include "vtkClipDataSet.h"
#include "vtkPlane.h"

#include "vtkUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

const char inputDataStream[] =
  "# vtk DataFile Version 3.0\n"
  "vtk output\n"
  "ASCII\n"
  "DATASET UNSTRUCTURED_GRID\n"
  "POINTS 8 float\n"
  "1337.72 1586.34 914.4 1337.72 1586.34 1371.6 1261.68 1606.71 914.4 \n"
  "1261.68 1606.71 1371.6 1337.72 1484.47 914.4 1337.72 1484.47 1371.6 \n"
  "1261.68 1464.1 914.4 1261.68 1464.1 1371.6 \n"
  "CELLS 1 32\n"
  "31 6 4 4 6 2 0 4 1 3 7 5 4 0 2 3 1 4 2 6 7 3 4 6 4 5 7 4 4 0 1 5 \n"
  "CELL_TYPES 1\n"
  "42\n";

// Test of contour/clip of vtkPolyhedron. uses input from https://gitlab.kitware.com/vtk/vtk/issues/15026
int TestPolyhedron3(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  vtkNew<vtkUnstructuredGridReader> reader;
  reader->SetInputString(inputDataStream);
  reader->ReadFromInputStringOn();

  vtkNew<vtkPlane> plane;
  plane->SetNormal(0.847934330264784, 0.530022019598814, -0.00916680417631942);
  plane->SetOrigin(1254.0760499239, 1489.93486006017, 1143.9780493697);

  vtkNew<vtkClipDataSet> clip;
  clip->SetInputConnection(reader->GetOutputPort());
  clip->SetClipFunction(plane.Get());
  clip->Update();

  vtkUnstructuredGrid* result = clip->GetOutput(0);
  if (!result) return 1;
  if (result->GetNumberOfCells() != 1)
  {
    std::cout << "Expected 1 but found " << result->GetNumberOfCells() << " cells in intersected polyhedron" << std::endl;
    return 1;
  }
  vtkCell* clipped = result->GetCell(0);
  if (!clipped) return 1;
  if (clipped->GetNumberOfFaces() != 7)
  {
    std::cout << "Expected 7 but found " << result->GetNumberOfCells() << " cells in intersected polyhedron" << std::endl;
    return 1;
  }

  return 0; // success
}
