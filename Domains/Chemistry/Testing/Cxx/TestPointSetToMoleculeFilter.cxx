/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPointSetToMoleculeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDoubleArray.h"
#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPointSetToMoleculeFilter.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnsignedShortArray.h"

int TestPointSetToMoleculeFilter(int, char* [])
{
  vtkNew<vtkPolyData> polyData;
  vtkNew<vtkPoints> points;
  const int numberOfAtoms = 4;
  points->SetNumberOfPoints(numberOfAtoms);
  points->SetPoint(0, 0, 0, 0);
  points->SetPoint(1, 1, 1, 1);
  points->SetPoint(2, 2, 2, 2);
  points->SetPoint(3, 3, 3, 3);
  polyData->SetPoints(points);

  vtkNew<vtkUnsignedShortArray> scalars;
  scalars->SetNumberOfValues(numberOfAtoms);
  const unsigned short firstAtomicNb = 42;
  scalars->SetValue(0, firstAtomicNb);
  scalars->SetValue(1, firstAtomicNb + 1);
  scalars->SetValue(2, firstAtomicNb + 2);
  scalars->SetValue(3, firstAtomicNb + 3);
  scalars->SetName("scalarsData");
  polyData->GetPointData()->SetScalars(scalars);

  vtkNew<vtkDoubleArray> extraData;
  extraData->SetNumberOfValues(numberOfAtoms);
  extraData->SetValue(0, 0.0);
  extraData->SetValue(1, 0.1);
  extraData->SetValue(2, 0.2);
  extraData->SetValue(3, 0.3);
  extraData->SetName("ExtraData");
  polyData->GetPointData()->AddArray(extraData);

  vtkNew<vtkPointSetToMoleculeFilter> filter;
  filter->SetInputData(polyData.Get());
  filter->Update();

  vtkMolecule* molecule = filter->GetOutput();

  if (!molecule)
  {
    std::cout << "Output molecule was not initialized !" << std::endl;
    return EXIT_FAILURE;
  }

  if (molecule->GetNumberOfAtoms() != numberOfAtoms)
  {
    std::cout << "Wrong number of atoms." << std::endl;
    return EXIT_FAILURE;
  }

  // all arrays are copied + atomic number created from scalars
  const int nbExpectedArrays = 3;
  if (molecule->GetVertexData()->GetNumberOfArrays() != nbExpectedArrays)
  {
    std::cout << "Wrong number of arrays (" << molecule->GetVertexData()->GetNumberOfArrays()
              << " instead of " << nbExpectedArrays << ")" << std::endl;
    return EXIT_FAILURE;
  }

  vtkDataArray* atomicNumbers = molecule->GetAtomicNumberArray();
  if (!atomicNumbers)
  {
    std::cout << "No scalars array." << std::endl;
    return EXIT_FAILURE;
  }

  if (atomicNumbers->GetTuple1(0) != firstAtomicNb)
  {
    std::cout << "Wrong atomic number value !" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
