// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPlane.h"
#include "vtkPolyhedron.h"
#include "vtkUnstructuredGrid.h"

#include "vtkCutter.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

// Test of contour/clip of vtkPolyhedron. uses input from
// https://gitlab.kitware.com/vtk/vtk/-/issues/14485
int TestPolyhedron2(int argc, char* argv[])
{
  vtkObject::GlobalWarningDisplayOff();

  const char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/polyhedron_mesh.vtu");
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();

  vtkUnstructuredGrid* pGrid = reader->GetOutput();

  vtkNew<vtkCutter> cutter;
  vtkNew<vtkPlane> p;
  p->SetOrigin(pGrid->GetCenter());
  p->SetNormal(1, 0, 0);

  cutter->SetCutFunction(p);
  cutter->SetGenerateTriangles(0);

  cutter->SetInputConnection(0, reader->GetOutputPort());
  cutter->Update();

  vtkPolyData* output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  if (output->GetNumberOfCells() != 2)
  {
    std::cerr << "Expected 2 polygons but found " << output->GetNumberOfCells()
              << " polygons in sliced polyhedron." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
