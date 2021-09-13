/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGhostCells.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkExtractGhostCells.h"

#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkThreshold.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkExtractGhostCells);

//------------------------------------------------------------------------------
vtkExtractGhostCells::vtkExtractGhostCells()
  : OutputGhostArrayName(nullptr)
{
  this->SetOutputGhostArrayName("GhostType");
}

//------------------------------------------------------------------------------
vtkExtractGhostCells::~vtkExtractGhostCells()
{
  this->SetOutputGhostArrayName(nullptr);
}

//------------------------------------------------------------------------------
int vtkExtractGhostCells::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractGhostCells::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto outputUG = vtkUnstructuredGrid::GetData(outputVector, 0);

  if (!inputDO)
  {
    return 1;
  }

  if (!outputUG)
  {
    vtkErrorMacro("Output does not downcast to vtkUnstructuredGrid. Aborting.");
    return 0;
  }

  vtkNew<vtkThreshold> threshold;
  threshold->SetInputData(inputDO);
  // DUPLICATECELL == 1. Any number above that is a ghost.
  threshold->SetUpperThreshold(vtkDataSetAttributes::CellGhostTypes::DUPLICATECELL);
  threshold->SetThresholdFunction(vtkThreshold::THRESHOLD_UPPER);
  threshold->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, vtkDataSetAttributes::GhostArrayName());
  threshold->Update();

  outputUG->ShallowCopy(threshold->GetOutputDataObject(0));

  if (vtkUnsignedCharArray* ghosts = outputUG->GetCellGhostArray())
  {
    // This is fine since vtkThreshold has already created a copy of the array.
    if (!this->OutputGhostArrayName)
    {
      vtkWarningMacro("OutputGhostArrayName not set... Setting name in output as \"GhostType\"");
      ghosts->SetName("GhostType");
    }
    else
    {
      ghosts->SetName(this->OutputGhostArrayName);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractGhostCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputGhostArrayName: "
     << (this->OutputGhostArrayName ? this->OutputGhostArrayName : "(nullptr)") << std::endl;
}
