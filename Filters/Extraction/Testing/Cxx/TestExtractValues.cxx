/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractValues.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests value selection of a vtkPolyData

#include "vtkExtractSelection.h"
#include "vtkNew.h"
#include "vtkSelectionSource.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLPolyDataReader.h"

int TestExtractValues(int vtkNotUsed(argc), char* argv[])
{
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(argv[1]);

  vtkNew<vtkSelectionSource> selection;
  selection->SetArrayName("Solid id");
  selection->SetContentType(vtkSelectionNode::VALUES);
  selection->SetFieldType(vtkSelectionNode::CELL);
  selection->AddID(-1, 1);
  selection->AddID(-1, 2);

  vtkNew<vtkExtractSelection> extract;
  extract->SetInputConnection(0, reader->GetOutputPort());
  extract->SetInputConnection(1, selection->GetOutputPort());
  extract->Update();

  vtkUnstructuredGrid* result = vtkUnstructuredGrid::SafeDownCast(extract->GetOutput());
  vtkIdType nbCells = result->GetNumberOfCells();

  // We are extracting 2 cubes. Each cube has 6 faces of 4 faces, 12 polylines and 8 vertices.
  // We are expecting 2*(6*4+12+8) = 88 cells
  if (nbCells == 88)
  {
    return EXIT_SUCCESS;
  }

  cerr << "There is " << nbCells << " cells instead of 88 cells." << endl;

  return EXIT_FAILURE;
}
