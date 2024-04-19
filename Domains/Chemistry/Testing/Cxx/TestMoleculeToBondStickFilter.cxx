// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTestUtilities.h"

#include "vtkCMLMoleculeReader.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkMolecule.h"
#include "vtkMoleculeToBondStickFilter.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

bool CheckNumbers(std::string name, int first, int second)
{
  if (first != second)
  {
    std::cerr << "Error: wrong number of " << name << ". Got " << first << " but expects " << second
              << std::endl;
    return false;
  }

  return true;
}

int TestMoleculeToBondStickFilter(int argc, char* argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/porphyrin.cml");

  // Read molecule from .cml file
  vtkNew<vtkCMLMoleculeReader> reader;
  reader->SetFileName(fileName);
  reader->Update();
  delete[] fileName;
  vtkMolecule* molecule = reader->GetOutput();

  if (!molecule)
  {
    std::cerr << "Molecule reader produced incorrect output!" << std::endl;
    return EXIT_FAILURE;
  }

  // Apply MoleculeToBondStickFilter
  vtkNew<vtkMoleculeToBondStickFilter> bondFilter;
  bondFilter->SetInputConnection(reader->GetOutputPort());
  bondFilter->Update();
  vtkPolyData* poly = bondFilter->GetOutput();

  if (!poly)
  {
    std::cerr << "Filter produced incorrect output!" << std::endl;
    return EXIT_FAILURE;
  }

  // Retrieve number of links (i.e. taking into account bond orders)
  int numLinks = 0;
  for (vtkIdType bondIdx = 0; bondIdx < molecule->GetNumberOfBonds(); ++bondIdx)
  {
    numLinks += molecule->GetBond(bondIdx).GetOrder();
  }

  // Check that data array has been created
  vtkDataArray* bondOrderArray =
    poly->GetPointData()->GetScalars(molecule->GetBondOrdersArrayName());
  if (!bondOrderArray)
  {
    std::cerr << "Array named " << molecule->GetBondOrdersArrayName()
              << " should have been created!" << std::endl;
    return EXIT_FAILURE;
  }

  // Check number of points, cells and point scalar data
  // 80 (resp. 22) corresponds to the number of points (resp. cells)
  // of the cylinder used to model the bonds
  if (!CheckNumbers("points", poly->GetNumberOfPoints(), 80 * numLinks) ||
    !CheckNumbers("cells", poly->GetNumberOfPolys(), 22 * numLinks) ||
    !CheckNumbers("point data", bondOrderArray->GetNumberOfTuples(), 80 * numLinks))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
