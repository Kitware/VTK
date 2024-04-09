// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGenerateProcessIds.h"

#include "vtkCellData.h"
#include "vtkConstantArray.h"
#include "vtkDataSet.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN
namespace
{
const char* PROCESS_ID_ARR_NAME = "ProcessIds";

//------------------------------------------------------------------------------
vtkSmartPointer<vtkConstantArray<vtkIdType>> GenerateProcessIds(
  vtkIdType piece, vtkIdType numberOfTuples)
{
  vtkNew<vtkConstantArray<vtkIdType>> arr;
  arr->ConstructBackend(piece);
  arr->SetNumberOfComponents(1);
  arr->SetNumberOfTuples(numberOfTuples);

  return arr;
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHyperTreeGridGenerateProcessIds);

//------------------------------------------------------------------------------
vtkCxxSetSmartPointerMacro(
  vtkHyperTreeGridGenerateProcessIds, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkHyperTreeGridGenerateProcessIds::vtkHyperTreeGridGenerateProcessIds()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
}

//------------------------------------------------------------------------------
vtkHyperTreeGridGenerateProcessIds::~vtkHyperTreeGridGenerateProcessIds() = default;

//------------------------------------------------------------------------------
int vtkHyperTreeGridGenerateProcessIds::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::GetData(inInfo);
  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::GetData(outInfo);

  if (!inputHTG || !outputHTG)
  {
    vtkErrorMacro(<< "Unable to retrieve input / output as supported type.");
    return 0;
  }

  vtkIdType piece =
    this->Controller ? static_cast<vtkIdType>(this->Controller->GetLocalProcessId()) : 0;
  outputHTG->ShallowCopy(inputHTG);

  vtkIdType numberOfCells = inputHTG->GetNumberOfCells();
  auto processIds = ::GenerateProcessIds(piece, numberOfCells);
  processIds->SetName(PROCESS_ID_ARR_NAME);
  outputHTG->GetCellData()->SetProcessIds(processIds);

  return 1;
}

//------------------------------------------------------------------------------
vtkMultiProcessController* vtkHyperTreeGridGenerateProcessIds::GetController()
{
  return this->Controller.Get();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateProcessIds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: ";
  if (this->Controller)
  {
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << std::endl;
  }
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGenerateProcessIds::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGenerateProcessIds::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}
VTK_ABI_NAMESPACE_END
