// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOverlappingAMRLevelIdScalars.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"
#include "vtkUnsignedCharArray.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOverlappingAMRLevelIdScalars);
//------------------------------------------------------------------------------
vtkOverlappingAMRLevelIdScalars::vtkOverlappingAMRLevelIdScalars() = default;

//------------------------------------------------------------------------------
vtkOverlappingAMRLevelIdScalars::~vtkOverlappingAMRLevelIdScalars() = default;

//------------------------------------------------------------------------------
void vtkOverlappingAMRLevelIdScalars::AddColorLevels(
  vtkUniformGridAMR* input, vtkUniformGridAMR* output)
{
  assert("pre: input should not be nullptr" && (input != nullptr));
  assert("pre: output should not be nullptr" && (output != nullptr));

  unsigned int numLevels = input->GetNumberOfLevels();
  output->CopyStructure(input);
  for (unsigned int levelIdx = 0; levelIdx < numLevels; levelIdx++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    unsigned int numDS = input->GetNumberOfDataSets(levelIdx);
    for (unsigned int cc = 0; cc < numDS; cc++)
    {
      vtkUniformGrid* ds = input->GetDataSet(levelIdx, cc);
      if (ds != nullptr)
      {
        vtkUniformGrid* copy = this->ColorLevel(ds, levelIdx);
        output->SetDataSet(levelIdx, cc, copy);
        copy->Delete();
      }
    }
  }
}

//------------------------------------------------------------------------------
// Map ids into attribute data
int vtkOverlappingAMRLevelIdScalars::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkUniformGridAMR* input =
    vtkUniformGridAMR::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    return 0;
  }

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkUniformGridAMR* output =
    vtkUniformGridAMR::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    return 0;
  }

  this->AddColorLevels(input, output);
  return 1;
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkOverlappingAMRLevelIdScalars::ColorLevel(vtkUniformGrid* input, int group)
{
  vtkUniformGrid* output = input->NewInstance();
  output->ShallowCopy(input);
  vtkDataSet* dsOutput = vtkDataSet::SafeDownCast(output);
  vtkIdType numCells = dsOutput->GetNumberOfCells();
  vtkUnsignedCharArray* cArray = vtkUnsignedCharArray::New();
  cArray->SetNumberOfTuples(numCells);
  for (vtkIdType cellIdx = 0; cellIdx < numCells; cellIdx++)
  {
    cArray->SetValue(cellIdx, group);
  }
  cArray->SetName("BlockIdScalars");
  dsOutput->GetCellData()->AddArray(cArray);
  cArray->Delete();
  return output;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRLevelIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
