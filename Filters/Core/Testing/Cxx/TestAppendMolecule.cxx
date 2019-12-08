/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAppendMolecule.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkMolecule.h"
#include "vtkMoleculeAppend.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedShortArray.h"

#define CheckNumbers(name, first, second)                                                          \
  if (first != second)                                                                             \
  {                                                                                                \
    cerr << "Error : wrong number of " << #name << ". Got " << first << " but expects " << second  \
         << endl;                                                                                  \
    return EXIT_FAILURE;                                                                           \
  }

// Used to creates different atoms and data for each molecule
static int NB_OF_MOL = 0;

// Add two atoms with a bond between them.
void InitSimpleMolecule(vtkMolecule* molecule)
{
  NB_OF_MOL++;
  vtkAtom h1 = molecule->AppendAtom(1, 0.5, 1.5, -NB_OF_MOL);
  vtkAtom h2 = molecule->AppendAtom(1, 0.5, 1.5, NB_OF_MOL);
  molecule->AppendBond(h1, h2, 1);
}

void AddAtomData(vtkMolecule* molecule)
{
  vtkNew<vtkDoubleArray> data;
  data->SetName("Data");
  data->SetNumberOfComponents(1);
  vtkIdType size = molecule->GetNumberOfAtoms();
  for (vtkIdType i = 0; i < size; i++)
  {
    data->InsertNextValue(NB_OF_MOL * 1.01);
  }
  molecule->GetAtomData()->AddArray(data);

  vtkNew<vtkStringArray> stringData;
  stringData->SetName("StringData");
  size = molecule->GetNumberOfBonds();
  for (vtkIdType i = 0; i < size; i++)
  {
    stringData->InsertNextValue("string");
  }
  molecule->GetBondData()->AddArray(stringData);
}

int CheckMolecule(vtkMolecule* molecule, int nbAtoms, int nbBonds, int nbOfAtomArrays,
  int nbOfBondArrays, vtkDoubleArray* values, int nbGhostAtoms, int nbGhostBonds)
{
  CheckNumbers("atoms", molecule->GetNumberOfAtoms(), nbAtoms);
  CheckNumbers("bonds", molecule->GetNumberOfBonds(), nbBonds);
  CheckNumbers("atom data arrays", molecule->GetAtomData()->GetNumberOfArrays(), nbOfAtomArrays);
  CheckNumbers("bond data arrays", molecule->GetBondData()->GetNumberOfArrays(), nbOfBondArrays);

  vtkDataArray* resultData = molecule->GetAtomData()->GetArray("Data");
  if (!resultData)
  {
    std::cerr << "Error : atoms data array not found in result" << std::endl;
    return EXIT_FAILURE;
  }
  CheckNumbers("atom data array values", resultData->GetNumberOfTuples(), nbAtoms);

  for (vtkIdType i = 0; i < nbAtoms; i++)
  {
    CheckNumbers("data value", resultData->GetTuple1(i), values->GetValue(i));
  }

  vtkUnsignedShortArray* bondOrderArray = molecule->GetBondOrdersArray();
  if (!bondOrderArray)
  {
    std::cerr << "Error : bonds data array not found in result" << std::endl;
    return EXIT_FAILURE;
  }
  CheckNumbers("bond data array values", bondOrderArray->GetNumberOfTuples(), nbBonds);

  vtkUnsignedCharArray* ghostAtoms = molecule->GetAtomGhostArray();
  int nbOfGhosts = 0;
  for (vtkIdType id = 0; id < nbAtoms; id++)
  {
    if (ghostAtoms->GetValue(id) == 1)
    {
      nbOfGhosts++;
    }
  }
  // ghost atom from molecule2 is still ghost in result
  CheckNumbers("ghost atoms", nbOfGhosts, nbGhostAtoms);

  vtkUnsignedCharArray* ghostBonds = molecule->GetBondGhostArray();
  nbOfGhosts = 0;
  for (vtkIdType id = 0; id < nbBonds; id++)
  {
    if (ghostBonds->GetValue(id) == 1)
    {
      nbOfGhosts++;
    }
  }
  // ghost bond from molecule2 is still ghost in result
  CheckNumbers("ghost bonds", nbOfGhosts, nbGhostBonds);

  return EXIT_SUCCESS;
}

int TestAppendMolecule(int, char*[])
{
  // --------------------------------------------------------------------------
  // Simple test : 2 molecules, no data
  vtkNew<vtkMolecule> simpleMolecule1;
  InitSimpleMolecule(simpleMolecule1);

  vtkNew<vtkMolecule> simpleMolecule2;
  InitSimpleMolecule(simpleMolecule2);

  vtkNew<vtkMoleculeAppend> appender;
  appender->AddInputData(simpleMolecule1);
  appender->AddInputData(simpleMolecule2);
  appender->Update();
  vtkMolecule* resultMolecule = appender->GetOutput();

  int expectedResult = simpleMolecule1->GetNumberOfAtoms() + simpleMolecule2->GetNumberOfAtoms();
  CheckNumbers("atoms", resultMolecule->GetNumberOfAtoms(), expectedResult);

  expectedResult = simpleMolecule1->GetNumberOfBonds() + simpleMolecule2->GetNumberOfBonds();
  CheckNumbers("bonds", resultMolecule->GetNumberOfBonds(), expectedResult);

  // --------------------------------------------------------------------------
  // Full test : ghosts and data

  /**
   * Use 3 molecules:
   *  - fullMolecule1 : 2 atoms and one bond, no ghost
   *  - fullMolecule2 : 3 atoms and 2 bonds, one ghost atom and one ghost bond
   *  - fullMolecule3 : 3 atoms and 2 bonds, one ghost atom and one ghost bond
   */

  // INIT
  vtkNew<vtkMolecule> fullMolecule1;
  InitSimpleMolecule(fullMolecule1);
  AddAtomData(fullMolecule1);
  vtkNew<vtkMolecule> fullMolecule2;
  InitSimpleMolecule(fullMolecule2);
  AddAtomData(fullMolecule2);
  vtkNew<vtkMolecule> fullMolecule3;
  InitSimpleMolecule(fullMolecule3);
  AddAtomData(fullMolecule3);

  // duplicate first atom of molecule 2 to be ghost in molecule 3, and vice versa.
  vtkAtom firstAtom2 = fullMolecule2->GetAtom(0);
  vtkAtom firstAtom3 = fullMolecule3->GetAtom(0);

  vtkAtom ghostAtom2 =
    fullMolecule2->AppendAtom(firstAtom3.GetAtomicNumber(), firstAtom3.GetPosition());
  vtkBond ghostBond2 = fullMolecule2->AppendBond(firstAtom2, ghostAtom2, 1);

  vtkAtom ghostAtom3 =
    fullMolecule3->AppendAtom(firstAtom2.GetAtomicNumber(), firstAtom2.GetPosition());
  vtkBond ghostBond3 = fullMolecule3->AppendBond(firstAtom3, ghostAtom3, 1);

  // set ghost flag on relevant atoms and bonds.
  fullMolecule1->AllocateAtomGhostArray();
  fullMolecule1->AllocateBondGhostArray();

  fullMolecule2->AllocateAtomGhostArray();
  fullMolecule2->GetAtomGhostArray()->SetValue(ghostAtom2.GetId(), 1);
  fullMolecule2->AllocateBondGhostArray();
  fullMolecule2->GetBondGhostArray()->SetValue(ghostBond2.GetId(), 1);

  fullMolecule3->AllocateAtomGhostArray();
  fullMolecule3->GetAtomGhostArray()->SetValue(ghostAtom3.GetId(), 1);
  fullMolecule3->AllocateBondGhostArray();
  fullMolecule3->GetBondGhostArray()->SetValue(ghostBond3.GetId(), 1);

  // --------------------------------------------------------------------------
  // First part: 2 molecules, ghost and data

  // APPEND 1 with 2
  vtkNew<vtkMoleculeAppend> appender2;
  appender2->AddInputData(fullMolecule1);
  appender2->AddInputData(fullMolecule2);
  appender2->Update();
  vtkMolecule* resultFullMolecule = appender2->GetOutput();

  // CHECK RESULT
  int nbOfExpectedAtoms = fullMolecule1->GetNumberOfAtoms() + fullMolecule2->GetNumberOfAtoms();
  int nbOfExpectedBonds = fullMolecule1->GetNumberOfBonds() + fullMolecule2->GetNumberOfBonds();
  int nbOfExpectedArrays = fullMolecule1->GetAtomData()->GetNumberOfArrays();
  int nbOfExpectedBondArrays = fullMolecule1->GetBondData()->GetNumberOfArrays();
  vtkNew<vtkDoubleArray> expectedResultValues;
  expectedResultValues->InsertNextValue(
    fullMolecule1->GetAtomData()->GetArray("Data")->GetTuple1(0));
  expectedResultValues->InsertNextValue(
    fullMolecule1->GetAtomData()->GetArray("Data")->GetTuple1(1));
  expectedResultValues->InsertNextValue(
    fullMolecule2->GetAtomData()->GetArray("Data")->GetTuple1(0));
  expectedResultValues->InsertNextValue(
    fullMolecule2->GetAtomData()->GetArray("Data")->GetTuple1(1));
  expectedResultValues->InsertNextValue(
    fullMolecule2->GetAtomData()->GetArray("Data")->GetTuple1(2));

  int res = CheckMolecule(resultFullMolecule, nbOfExpectedAtoms, nbOfExpectedBonds,
    nbOfExpectedArrays, nbOfExpectedBondArrays, expectedResultValues, 1, 1);
  if (res == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  // --------------------------------------------------------------------------
  // Second part: 3 molecules, ghost and data, no merge
  // APPEND third molecule
  appender2->MergeCoincidentAtomsOff();
  appender2->AddInputData(fullMolecule3);
  appender2->Update();
  resultFullMolecule = appender2->GetOutput();

  // CHECK RESULT
  nbOfExpectedAtoms = fullMolecule1->GetNumberOfAtoms() + fullMolecule2->GetNumberOfAtoms() +
    fullMolecule3->GetNumberOfAtoms();
  nbOfExpectedBonds = fullMolecule1->GetNumberOfBonds() + fullMolecule2->GetNumberOfBonds() +
    fullMolecule3->GetNumberOfBonds();
  nbOfExpectedArrays = fullMolecule1->GetAtomData()->GetNumberOfArrays();
  nbOfExpectedBondArrays = fullMolecule1->GetBondData()->GetNumberOfArrays();

  // Result contains data of non ghost atom.
  expectedResultValues->InsertNextValue(
    fullMolecule3->GetAtomData()->GetArray("Data")->GetTuple1(0));
  expectedResultValues->InsertNextValue(
    fullMolecule3->GetAtomData()->GetArray("Data")->GetTuple1(1));
  expectedResultValues->InsertNextValue(
    fullMolecule3->GetAtomData()->GetArray("Data")->GetTuple1(2));

  res = CheckMolecule(resultFullMolecule, nbOfExpectedAtoms, nbOfExpectedBonds, nbOfExpectedArrays,
    nbOfExpectedBondArrays, expectedResultValues, 2, 2);
  if (res == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  // --------------------------------------------------------------------------
  // Third part: 3 molecules, ghost and data, merge coincident atoms

  appender2->MergeCoincidentAtomsOn();
  appender2->Update();
  resultFullMolecule = appender2->GetOutput();
  // the ghost atoms are not duplicated in output.
  nbOfExpectedAtoms = fullMolecule1->GetNumberOfAtoms() + fullMolecule2->GetNumberOfAtoms() +
    fullMolecule3->GetNumberOfAtoms() - 2;
  // the ghost bond is not duplicated in output.
  nbOfExpectedBonds = fullMolecule1->GetNumberOfBonds() + fullMolecule2->GetNumberOfBonds() +
    fullMolecule3->GetNumberOfBonds() - 1;
  expectedResultValues->Resize(nbOfExpectedAtoms);
  expectedResultValues->InsertValue(
    4, fullMolecule3->GetAtomData()->GetArray("Data")->GetTuple1(0));
  expectedResultValues->InsertValue(
    4, fullMolecule3->GetAtomData()->GetArray("Data")->GetTuple1(1));

  return CheckMolecule(resultFullMolecule, nbOfExpectedAtoms, nbOfExpectedBonds, nbOfExpectedArrays,
    nbOfExpectedBondArrays, expectedResultValues, 0, 0);
}
