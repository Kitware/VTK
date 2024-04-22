// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGenerateGlobalIds.h"

#include "vtkCellData.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkPointData.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
int COMM_TAG = 1212;
const char* GLOBAL_IDS_ARR_NAME = "GlobalIds";

//------------------------------------------------------------------------------
void GenerateLocalGlobalIds(const std::vector<vtkHyperTreeGrid*>& htgs, vtkIdType offset)
{
  vtkIdType currentId = 0;
  for (const auto htg : htgs)
  {
    vtkNew<vtkIdTypeArray> cellIds;
    cellIds->SetNumberOfValues(htg->GetNumberOfCells());

    for (vtkIdType id = 0; id < htg->GetNumberOfCells(); id++)
    {
      cellIds->SetValue(id, currentId + offset);
      currentId++;
    }

    cellIds->SetName(GLOBAL_IDS_ARR_NAME);
    htg->GetCellData()->SetGlobalIds(cellIds);
  }
}

//------------------------------------------------------------------------------
void GenerateGlobalIds(
  vtkMultiProcessController* controller, const std::vector<vtkHyperTreeGrid*>& htgs)
{
  // Compute the local number of cells on current rank
  vtkIdType localNbOfCells = 0;
  for (const auto htg : htgs)
  {
    localNbOfCells += htg->GetNumberOfCells();
  }

  // Gather everything on rank 0
  std::vector<vtkIdType> gatheredLocalNbOfCells;
  gatheredLocalNbOfCells.resize(controller->GetNumberOfProcesses(), 0);
  controller->Gather(&localNbOfCells, gatheredLocalNbOfCells.data(), 1, 0);

  // Compute and communicate global offset to all ranks
  vtkIdType localOffset = 0;
  if (controller->GetLocalProcessId() == 0)
  {
    vtkIdType remoteOffset = 0;
    for (int rank = 1; rank < controller->GetNumberOfProcesses(); rank++)
    {
      remoteOffset += gatheredLocalNbOfCells[rank - 1];
      controller->Send(&remoteOffset, 1, rank, COMM_TAG);
    }
  }
  else
  {
    controller->Receive(&localOffset, 1, 0, COMM_TAG);
  }

  // Compute global ids on current rank
  ::GenerateLocalGlobalIds(htgs, localOffset);
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHyperTreeGridGenerateGlobalIds);

//------------------------------------------------------------------------------
vtkCxxSetSmartPointerMacro(
  vtkHyperTreeGridGenerateGlobalIds, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkHyperTreeGridGenerateGlobalIds::vtkHyperTreeGridGenerateGlobalIds()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
}

//------------------------------------------------------------------------------
vtkHyperTreeGridGenerateGlobalIds::~vtkHyperTreeGridGenerateGlobalIds() = default;

//------------------------------------------------------------------------------
int vtkHyperTreeGridGenerateGlobalIds::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // Check that every leaf of the composite input is a HTG instance
  vtkDataObjectTree* inputComposite = vtkDataObjectTree::GetData(inInfo);
  if (inputComposite)
  {
    vtkSmartPointer<vtkDataObjectTreeIterator> iter;
    iter.TakeReference(inputComposite->NewTreeIterator());
    iter->VisitOnlyLeavesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (vtkHyperTreeGrid::SafeDownCast(iter->GetCurrentDataObject()))
      {
        continue;
      }

      vtkErrorMacro(
        "Input composite dataset should only contain vtkHyperTreeGrid instances as leaves.");
      return 0;
    }
  }

  return this->Superclass::RequestDataObject(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGenerateGlobalIds::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObjectTree* inputComposite = vtkDataObjectTree::GetData(inInfo);
  vtkDataObjectTree* outputComposite = vtkDataObjectTree::GetData(outInfo);

  if (inputComposite && outputComposite)
  {
    outputComposite->ShallowCopy(inputComposite);
    std::vector<vtkHyperTreeGrid*> htgs =
      vtkCompositeDataSet::GetDataSets<vtkHyperTreeGrid>(outputComposite);
    htgs.erase(
      std::remove_if(htgs.begin(), htgs.end(),
        [](vtkHyperTreeGrid* htg) { return htg == nullptr || htg->GetNumberOfCells() == 0; }),
      htgs.end());
    ::GenerateGlobalIds(this->Controller, htgs);
    return 1;
  }

  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::GetData(inInfo);
  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::GetData(outInfo);

  if (inputHTG && outputHTG)
  {
    outputHTG->ShallowCopy(inputHTG);
    std::vector<vtkHyperTreeGrid*> htg;
    htg.emplace_back(outputHTG);
    ::GenerateGlobalIds(this->Controller, htg);
    return 1;
  }

  vtkErrorMacro(<< "Unable to retrieve input / output as supported type.");
  return 0;
}

//------------------------------------------------------------------------------
vtkMultiProcessController* vtkHyperTreeGridGenerateGlobalIds::GetController()
{
  return this->Controller.Get();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateGlobalIds::PrintSelf(ostream& os, vtkIndent indent)
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
int vtkHyperTreeGridGenerateGlobalIds::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
  return 1;
}
VTK_ABI_NAMESPACE_END
