/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPSimpleBondPerceiver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkPSimpleBondPerceiver.h"
#include "vtkPeriodicTable.h"
#include "vtkUnsignedCharArray.h"

int TestPSimpleBondPerceiver(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  assert("pre: Controller should not be nullptr" && (controller != nullptr));
  vtkMultiProcessController::SetGlobalController(controller);

  const int rank = controller->GetLocalProcessId();
  const float COVALENT_RADIUS = 1.02f;

  vtkNew<vtkMolecule> mol;
  vtkNew<vtkPSimpleBondPerceiver> bonder;

  const vtkIdType atomicNb = 4;
  vtkNew<vtkPeriodicTable> periodicTable;
  // assert table is as expected when writing the test. cf values in vtkBlueObeliskDataInternal.h
  if (periodicTable->GetCovalentRadius(atomicNb) != COVALENT_RADIUS)
  {
    std::cout << "Warning : covalent radius from periodic table has changed since the test has "
                 "been written."
              << std::endl;
  }

  // First create a test molecule:

  // inter atomic distance : create a square per rank
  const float interAtomic = 2 * COVALENT_RADIUS;
  // inter rank distance so we will have inter rank bonds before diagonals inside a rank.
  // 1.25 < sqrt(2)
  const float interRank = 1.25 * interAtomic;

  mol->AppendAtom(atomicNb, interRank * rank, 0, 0);
  mol->AppendAtom(atomicNb, interRank * rank, interAtomic, 0);
  mol->AppendAtom(atomicNb, interRank * rank, 0, interAtomic);
  mol->AppendAtom(atomicNb, interRank * rank, interAtomic, interAtomic);
  bonder->SetInputData(mol);

  // 1.
  // bonds only between the 4 atoms of the rank. (no diagonals)
  // relative tolerance
  //  - greater than 1 to create bonds
  //  - less than sqrt(2) to avoid the diagonals
  //  - less than 1.25 to avoid inter - rank
  bonder->SetIsToleranceAbsolute(false);
  bonder->SetTolerance(1.15);
  bonder->Update();

  vtkIdType numOfBonds = bonder->GetOutput()->GetNumberOfBonds();
  vtkIdType expectedNumOfBonds = 4;
  if (numOfBonds != expectedNumOfBonds)
  {
    std::cout << "Case 1 : Wrong number of bonds for (have " << numOfBonds << " instead of "
              << expectedNumOfBonds << ")" << std::endl;
    controller->Finalize();
    return EXIT_FAILURE;
  }

  // 2.
  // bonds between the 4 atoms of the rank AND between rank.
  bonder->SetTolerance(1.4);
  bonder->Update();
  numOfBonds = bonder->GetOutput()->GetNumberOfBonds();
  expectedNumOfBonds = 8;
  if (numOfBonds != expectedNumOfBonds)
  {
    std::cout << "Case 2 : Wrong number of bonds ! (have " << numOfBonds << " instead of "
              << expectedNumOfBonds << ")" << std::endl;
    controller->Finalize();
    return EXIT_FAILURE;
  }

  // 3.
  // bonds between the 4 atoms of the rank. (no diagonals)
  bonder->SetIsToleranceAbsolute(true);
  bonder->SetTolerance(0.3);
  bonder->Update();
  numOfBonds = bonder->GetOutput()->GetNumberOfBonds();
  expectedNumOfBonds = 4;
  if (numOfBonds != expectedNumOfBonds)
  {
    std::cout << "Case 3 : Wrong number of bonds ! (have " << numOfBonds << " instead of "
              << expectedNumOfBonds << ")" << std::endl;
    controller->Finalize();
    return EXIT_FAILURE;
  }

  // 4.
  // bonds between the 4 atoms of the rank AND between rank.
  bonder->SetTolerance(0.8);
  bonder->Update();
  numOfBonds = bonder->GetOutput()->GetNumberOfBonds();
  expectedNumOfBonds = 8;
  if (numOfBonds != expectedNumOfBonds)
  {
    std::cout << "Case 4 : Wrong number of bonds ! (have " << numOfBonds << " instead of "
              << expectedNumOfBonds << ")" << std::endl;
    controller->Finalize();
    return EXIT_FAILURE;
  }

  controller->Finalize();

  return EXIT_SUCCESS;
}
