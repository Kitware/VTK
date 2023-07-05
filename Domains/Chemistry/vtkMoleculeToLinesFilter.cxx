// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMoleculeToLinesFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkMolecule.h"
#include "vtkPointData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMoleculeToLinesFilter);

//------------------------------------------------------------------------------
void vtkMoleculeToLinesFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkMoleculeToLinesFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMolecule* input = vtkMolecule::SafeDownCast(vtkDataObject::GetData(inputVector[0]));
  vtkPolyData* output = vtkPolyData::SafeDownCast(vtkDataObject::GetData(outputVector));

  vtkNew<vtkCellArray> bonds;
  // 2 point ids + 1 VTKCellType = 3 values per bonds
  bonds->AllocateEstimate(input->GetNumberOfBonds(), 2);

  for (vtkIdType bondInd = 0; bondInd < input->GetNumberOfBonds(); ++bondInd)
  {
    vtkBond bond = input->GetBond(bondInd);
    vtkIdType ids[2] = { bond.GetBeginAtomId(), bond.GetEndAtomId() };
    bonds->InsertNextCell(2, ids);
  }

  output->SetPoints(input->GetAtomicPositionArray());
  output->SetLines(bonds);
  output->GetPointData()->DeepCopy(input->GetAtomData());
  output->GetCellData()->DeepCopy(input->GetBondData());

  return 1;
}
VTK_ABI_NAMESPACE_END
