/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractGridPieces.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
