/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetToMoleculeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointSetToMoleculeFilter.h"

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkMolecule.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"

vtkStandardNewMacro(vtkPointSetToMoleculeFilter);

//----------------------------------------------------------------------------
vtkPointSetToMoleculeFilter::vtkPointSetToMoleculeFilter()
  : ConvertLinesIntoBonds(true)
{
  this->SetNumberOfInputPorts(1);

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
int vtkPointSetToMoleculeFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPointSetToMoleculeFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPointSet* input = vtkPointSet::SafeDownCast(vtkDataObject::GetData(inputVector[0]));
  vtkMolecule* output = vtkMolecule::SafeDownCast(vtkDataObject::GetData(outputVector));

  if (!input)
  {
    vtkErrorMacro(<< "No input provided.");
    return 0;
  }

  vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (input->GetNumberOfPoints() > 0 && !inScalars)
  {
    vtkErrorMacro(<< "vtkPointSetToMoleculeFilter does not have atomic numbers as input.");
    return 0;
  }

  int res = output->Initialize(input->GetPoints(), inScalars, input->GetPointData());

  if (res != 0 && this->GetConvertLinesIntoBonds())
  {
    vtkNew<vtkIdList> inputBondsId;
    vtkNew<vtkIdList> outputBondsId;
    vtkSmartPointer<vtkCellIterator> iter =
      vtkSmartPointer<vtkCellIterator>::Take(input->NewCellIterator());
    // Get bond orders array. Use scalars as default.
    vtkDataArray* bondOrders = input->GetCellData()->HasArray(output->GetBondOrdersArrayName())
      ? input->GetCellData()->GetArray(output->GetBondOrdersArrayName())
      : input->GetCellData()->GetScalars();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
    {
      if (iter->GetCellType() != VTK_LINE)
      {
        continue;
      }
      vtkIdList* ptsId = iter->GetPointIds();
      unsigned short bondOrder = bondOrders ? bondOrders->GetTuple1(iter->GetCellId()) : 1;
      vtkBond bond = output->AppendBond(ptsId->GetId(0), ptsId->GetId(1), bondOrder);
      inputBondsId->InsertNextId(iter->GetCellId());
      outputBondsId->InsertNextId(bond.GetId());
    }

    output->GetBondData()->CopyAllocate(input->GetCellData());
    output->GetBondData()->CopyData(input->GetCellData(), inputBondsId, outputBondsId);
  }

  return res;
}
