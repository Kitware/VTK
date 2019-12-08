/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridGhostCellsGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkUnstructuredGridGhostCellsGenerator.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

namespace
{
const char* UGGCG_GLOBAL_POINT_IDS = "GlobalNodeIds";
const char* UGGCG_GLOBAL_CELL_IDS = "GlobalCellIds";
}

//----------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkUnstructuredGridGhostCellsGenerator);

//----------------------------------------------------------------------------
vtkUnstructuredGridGhostCellsGenerator::vtkUnstructuredGridGhostCellsGenerator()
{
  this->BuildIfRequired = true;
  this->MinimumNumberOfGhostLevels = 1;

  this->UseGlobalPointIds = true;
  this->GlobalPointIdsArrayName = nullptr;
  this->SetGlobalPointIdsArrayName(UGGCG_GLOBAL_POINT_IDS);

  this->HasGlobalCellIds = false;
  this->GlobalCellIdsArrayName = nullptr;
  this->SetGlobalCellIdsArrayName(UGGCG_GLOBAL_CELL_IDS);
}

//----------------------------------------------------------------------------
vtkUnstructuredGridGhostCellsGenerator::~vtkUnstructuredGridGhostCellsGenerator()
{
  this->SetGlobalPointIdsArrayName(nullptr);
  this->SetGlobalCellIdsArrayName(nullptr);
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "UseGlobalPointIds: " << this->UseGlobalPointIds << endl;
  os << indent << "GlobalPointIdsArrayName: "
     << (this->GlobalPointIdsArrayName == nullptr ? "(nullptr)" : this->GlobalPointIdsArrayName)
     << endl;
  os << indent << "HasGlobalCellIds: " << HasGlobalCellIds << endl;
  os << indent << "GlobalCellIdsArrayName: "
     << (this->GlobalCellIdsArrayName == nullptr ? "(nullptr)" : this->GlobalCellIdsArrayName)
     << endl;
  os << indent << "BuildIfRequired: " << this->BuildIfRequired << endl;
  os << indent << "MinimumNumberOfGhostLevels: " << this->MinimumNumberOfGhostLevels << endl;
}

//--------------------------------------------------------------------------
int vtkUnstructuredGridGhostCellsGenerator::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  // we can't trust any ghost levels coming in so we notify all filters before
  // this that we don't need ghosts
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkUnstructuredGridGhostCellsGenerator::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output. Input may just have the UnstructuredGridBase
  // interface, but output should be an unstructured grid.
  vtkUnstructuredGridBase* input =
    vtkUnstructuredGridBase::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!input)
  {
    vtkErrorMacro("No input data!");
    return 0;
  }

  output->ShallowCopy(input);
  return 1;
}
