/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMoleculeToLines.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"

#include "vtkCMLMoleculeReader.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkMolecule.h"
#include "vtkMoleculeToLinesFilter.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

#define CheckNumbers(name, first, second)                                                          \
  if (first != second)                                                                             \
  {                                                                                                \
    cerr << "Error : wrong number of " << #name << ". Got " << first << " but expects " << second  \
         << endl;                                                                                  \
    return EXIT_FAILURE;                                                                           \
  }

int TestMoleculeToLines(int argc, char* argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/porphyrin.cml");

  // read molecule from cml file
  vtkNew<vtkCMLMoleculeReader> reader;
  reader->SetFileName(fileName);
  reader->Update();
  delete[] fileName;
  vtkMolecule* molecule = reader->GetOutput();

  // convert
  vtkNew<vtkMoleculeToLinesFilter> converter;
  converter->SetInputConnection(reader->GetOutputPort());
  converter->Update();
  vtkPolyData* poly = converter->GetOutput();

  // check number of points, lines and associated data
  CheckNumbers("points", poly->GetNumberOfPoints(), molecule->GetNumberOfAtoms());
  CheckNumbers("lines", poly->GetNumberOfLines(), molecule->GetNumberOfBonds());
  CheckNumbers("pointData", poly->GetPointData()->GetNumberOfArrays(),
    molecule->GetAtomData()->GetNumberOfArrays());
  CheckNumbers("cellData", poly->GetCellData()->GetNumberOfArrays(),
    molecule->GetBondData()->GetNumberOfArrays());
  return EXIT_SUCCESS;
}
