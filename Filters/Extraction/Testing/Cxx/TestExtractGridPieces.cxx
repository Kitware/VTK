// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkExtractGrid.h>
#include <vtkNew.h>
#include <vtkStructuredGrid.h>
#include <vtkTestUtilities.h>
#include <vtkXMLStructuredGridReader.h>

int TestExtractGridPieces(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/multicomb_0.vts");

  vtkNew<vtkXMLStructuredGridReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  vtkNew<vtkExtractGrid> extractor;
  extractor->SetVOI(0, 5, 0, 11, 1, 1);
  extractor->SetInputConnection(reader->GetOutputPort());

  // extractor->Update();
  extractor->UpdatePiece(0, 2, 0);
  extractor->UpdatePiece(1, 2, 0);

  return EXIT_SUCCESS;
}
