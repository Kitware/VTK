/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyhedron4.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
