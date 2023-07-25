// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCutter.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtkXMLUnstructuredGridReader.h"

int TestPolyhedron4(int argc, char* argv[])
{
  // Test that a nonwatertight polyhedron does no make vtkPolyhedron segfault
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv,
    "Data/nonWatertightPolyhedron.vtu"); // this is in fact a bad name; the grid *is* watertight

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(filename);
  delete[] filename;

  vtkNew<vtkCutter> cutter;
  vtkNew<vtkPlane> p;
  p->SetOrigin(0, 0, 0);
  p->SetNormal(0, 1, 0);

  cutter->SetCutFunction(p);
  cutter->GenerateTrianglesOn();
  cutter->SetInputConnection(0, reader->GetOutputPort());

  // We want to check this does not segfault. We cannot check the error message
  vtkObject::GlobalWarningDisplayOff();
  cutter->Update();
  return EXIT_SUCCESS; // success
}
