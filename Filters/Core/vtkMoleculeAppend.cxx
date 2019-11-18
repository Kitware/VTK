/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMoleculeAppend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkMoleculeAppend.h"

#include "vtkAlgorithmOutput.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"

#include <set>
#include <utility>

vtkStandardNewMacro(vtkMoleculeAppend);

//----------------------------------------------------------------------------
vtkMoleculeAppend::vtkMoleculeAppend()
  : MergeCoincidentAtoms(true)
{
}

//----------------------------------------------------------------------------
vtkDataObject* vtkMoleculeAppend::GetInput(int idx)
{
  if (this->GetNumberOfInputConnections(0) <= idx)
  {
    return nullptr;
  }
  return vtkMolecule::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
int vtkMoleculeAppend::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMolecule* output = vtkMolecule::GetData(outputVector, 0);
  vtkDataSetAttributes* outputAtomData = output->GetAtomData();
  vtkDataSetAttributes* outputBondData = output->GetBondData();

  // ********************
  // Create output data arrays following first input arrays.
  vtkMolecule* mol0 = vtkMolecule::SafeDownCast(this->GetInput());
  outputAtomData->CopyStructure(mol0->GetAtomData());
  outputBondData->CopyStructure(mol0->GetBondData());
  output->SetAtomicNumberArrayName(mol0->GetAtomicNumberArrayName());
  output->SetBondOrdersArrayName(mol0->GetBondOrdersArrayName());
  vtkUnsignedCharArray* outputGhostAtoms = output->GetAtomGhostArray();
  vtkUnsignedCharArray* outputGhostBonds = output->GetBondGhostArray();

  // ********************
  // Initialize unique atoms/bonds containers
  vtkNew<vtkMergePoints> uniquePoints;
  vtkNew<vtkPoints> uniquePointsList;
  double bounds[6] = { 0., 0., 0., 0., 0., 0. };
  uniquePoints->InitPointInsertion(uniquePointsList, bounds, 0);
  std::set<std::pair<vtkIdType, vtkIdType> > uniqueBonds;

  // ********************
  // Process each input
  for (int idx = 0; idx < this->GetNumberOfInputConnections(0); ++idx)
  {
    vtkMolecule* input = vtkMolecule::GetData(inputVector[0], idx);

    // --------------------
    // Sanity check on input
    int inputNbAtomArrays = input->GetAtomData()->GetNumberOfArrays();
    if (inputNbAtomArrays != outputAtomData->GetNumberOfArrays())
    {
      vtkErrorMacro(<< "Input " << idx << ": Wrong number of atom array. Has " << inputNbAtomArrays
                    << " instead of " << outputAtomData->GetNumberOfArrays());
      return 0;
    }

    int inputNbBondArrays = input->GetBondData()->GetNumberOfArrays();
    if (input->GetNumberOfBonds() > 0 && inputNbBondArrays != outputBondData->GetNumberOfArrays())
    {
      vtkErrorMacro(<< "Input " << idx << ": Wrong number of bond array. Has " << inputNbBondArrays
                    << " instead of " << outputBondData->GetNumberOfArrays());
      return 0;
    }

    for (vtkIdType ai = 0; ai < inputNbAtomArrays; ai++)
    {
      vtkAbstractArray* inArray = input->GetAtomData()->GetAbstractArray(ai);
      if (!this->CheckArrays(inArray, outputAtomData->GetAbstractArray(inArray->GetName())))
      {
        vtkErrorMacro(<< "Input " << idx << ": atoms arrays do not match with output");
        return 0;
      }
    }

    for (vtkIdType ai = 0; ai < inputNbBondArrays; ai++)
    {
      vtkAbstractArray* inArray = input->GetBondData()->GetAbstractArray(ai);
      if (!this->CheckArrays(inArray, outputBondData->GetAbstractArray(inArray->GetName())))
      {
        vtkErrorMacro(<< "Input " << idx << ": bonds arrays do not match with output");
        return 0;
      }
    }

    // --------------------
    // add atoms and bonds without duplication

    // map from 'input molecule atom ids' to 'output molecule atom ids'
    std::vector<vtkIdType> atomIdMap(input->GetNumberOfAtoms(), -1);

    vtkIdType previousNbOfAtoms = output->GetNumberOfAtoms();
    int nbOfAtoms = 0;
    for (vtkIdType i = 0; i < input->GetNumberOfAtoms(); i++)
    {
      double pt[3];
      input->GetAtomicPositionArray()->GetPoint(i, pt);
      bool addAtom = true;
      if (this->MergeCoincidentAtoms)
      {
        addAtom = uniquePoints->InsertUniquePoint(pt, atomIdMap[i]) == 1;
      }
      else
      {
        atomIdMap[i] = previousNbOfAtoms + nbOfAtoms;
      }

      if (addAtom)
      {
        nbOfAtoms++;
        vtkAtom atom = input->GetAtom(i);
        output->AppendAtom(atom.GetAtomicNumber(), atom.GetPosition()).GetId();
        if (outputGhostAtoms)
        {
          outputGhostAtoms->InsertValue(atomIdMap[i], 255);
        }
      }
    }
    vtkIdType previousNbOfBonds = output->GetNumberOfBonds();
    int nbOfBonds = 0;
    for (vtkIdType i = 0; i < input->GetNumberOfBonds(); i++)
    {
      vtkBond bond = input->GetBond(i);
      // as bonds are undirected, put min atom number at first to avoid duplication.
      vtkIdType atom1 = atomIdMap[bond.GetBeginAtomId()];
      vtkIdType atom2 = atomIdMap[bond.GetEndAtomId()];
      auto result =
        uniqueBonds.insert(std::make_pair(std::min(atom1, atom2), std::max(atom1, atom2)));
      if (result.second)
      {
        nbOfBonds++;
        output->AppendBond(atom1, atom2, bond.GetOrder());
      }
    }

    // --------------------
    // Reset arrays size (and allocation if needed)
    for (vtkIdType ai = 0; ai < input->GetAtomData()->GetNumberOfArrays(); ai++)
    {
      vtkAbstractArray* inArray = input->GetAtomData()->GetAbstractArray(ai);
      vtkAbstractArray* outArray = output->GetAtomData()->GetAbstractArray(inArray->GetName());
      outArray->Resize(previousNbOfAtoms + nbOfAtoms);
    }

    for (vtkIdType ai = 0; ai < input->GetBondData()->GetNumberOfArrays(); ai++)
    {
      // skip bond orders array as it is auto-filled by AppendBond method
      vtkAbstractArray* inArray = input->GetBondData()->GetAbstractArray(ai);
      if (!strcmp(inArray->GetName(), input->GetBondOrdersArrayName()))
      {
        continue;
      }
      vtkAbstractArray* outArray = output->GetBondData()->GetAbstractArray(inArray->GetName());
      outArray->Resize(previousNbOfBonds + nbOfBonds);
    }

    // --------------------
    // Fill DataArrays
    for (vtkIdType i = 0; i < input->GetNumberOfAtoms(); i++)
    {
      for (vtkIdType ai = 0; ai < input->GetAtomData()->GetNumberOfArrays(); ai++)
      {
        vtkAbstractArray* inArray = input->GetAtomData()->GetAbstractArray(ai);
        vtkAbstractArray* outArray = output->GetAtomData()->GetAbstractArray(inArray->GetName());
        // Use Value of non-ghost atom.
        if (outputGhostAtoms && outputGhostAtoms->GetValue(atomIdMap[i]) == 0)
        {
          continue;
        }
        outArray->InsertTuple(atomIdMap[i], i, inArray);
      }
    }
    for (vtkIdType i = 0; i < input->GetNumberOfBonds(); i++)
    {
      vtkBond bond = input->GetBond(i);
      vtkIdType outputBondId =
        output->GetBondId(atomIdMap[bond.GetBeginAtomId()], atomIdMap[bond.GetEndAtomId()]);

      for (vtkIdType ai = 0; ai < input->GetBondData()->GetNumberOfArrays(); ai++)
      {
        // skip bond orders array as it is auto-filled by AppendBond method
        vtkAbstractArray* inArray = input->GetBondData()->GetAbstractArray(ai);
        if (!strcmp(inArray->GetName(), input->GetBondOrdersArrayName()))
        {
          continue;
        }
        vtkAbstractArray* outArray = output->GetBondData()->GetAbstractArray(inArray->GetName());
        outArray->InsertTuple(outputBondId, i, inArray);
      }
    }
  }

  if (outputGhostBonds)
  {
    outputGhostBonds->SetNumberOfTuples(output->GetNumberOfBonds());
    outputGhostBonds->Fill(0);
    for (vtkIdType bondId = 0; bondId < output->GetNumberOfBonds(); bondId++)
    {
      vtkIdType atom1 = output->GetBond(bondId).GetBeginAtomId();
      vtkIdType atom2 = output->GetBond(bondId).GetEndAtomId();
      if (outputGhostAtoms->GetValue(atom1) == 1 || outputGhostAtoms->GetValue(atom2) == 1)
      {
        outputGhostBonds->SetValue(bondId, 1);
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMoleculeAppend::FillInputPortInformation(int i, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return this->Superclass::FillInputPortInformation(i, info);
}

//----------------------------------------------------------------------------
bool vtkMoleculeAppend::CheckArrays(vtkAbstractArray* array1, vtkAbstractArray* array2)
{
  if (strcmp(array1->GetName(), array2->GetName()))
  {
    vtkErrorMacro(<< "Execute: input name (" << array1->GetName() << "), must match output name ("
                  << array2->GetName() << ")");
    return false;
  }

  if (array1->GetDataType() != array2->GetDataType())
  {
    vtkErrorMacro(<< "Execute: input ScalarType (" << array1->GetDataType()
                  << "), must match output ScalarType (" << array2->GetDataType() << ")");
    return false;
  }

  if (array1->GetNumberOfComponents() != array2->GetNumberOfComponents())
  {
    vtkErrorMacro("Components of the inputs do not match");
    return false;
  }

  return true;
}
