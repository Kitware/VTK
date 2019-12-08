/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemoveGhosts.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "vtkRemoveGhosts.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

vtkStandardNewMacro(vtkRemoveGhosts);

//-----------------------------------------------------------------------------
vtkRemoveGhosts::vtkRemoveGhosts() = default;

//-----------------------------------------------------------------------------
vtkRemoveGhosts::~vtkRemoveGhosts() = default;

//-----------------------------------------------------------------------------
void vtkRemoveGhosts::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkRemoveGhosts::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkRemoveGhosts::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro("RequestData");

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkUnsignedCharArray* ghostArray = vtkUnsignedCharArray::SafeDownCast(
    input->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));
  if (ghostArray == nullptr)
  {
    // no ghost information so can just shallow copy input
    output->ShallowCopy(input);
    output->GetPointData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
    return 1;
  }

  if (ghostArray && ghostArray->GetValueRange()[1] == 0)
  {
    // we have ghost cell arrays but there are no ghost entities so we just need
    // to remove those arrays and can skip modifying the data set itself
    output->ShallowCopy(input);
    output->GetPointData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
    output->GetCellData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
    return 1;
  }

  output->DeepCopy(input);
  if (vtkUnstructuredGrid* ugOutput = vtkUnstructuredGrid::SafeDownCast(output))
  {
    ugOutput->RemoveGhostCells();
  }
  else if (vtkPolyData* pdOutput = vtkPolyData::SafeDownCast(output))
  {
    pdOutput->RemoveGhostCells();
  }
  output->GetCellData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
  output->GetPointData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
  return 1;
}

//----------------------------------------------------------------------------
int vtkRemoveGhosts::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}
