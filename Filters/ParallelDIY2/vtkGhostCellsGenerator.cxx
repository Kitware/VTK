// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGhostCellsGenerator.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDIYGhostUtilities.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkRange.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGhostCellsGenerator);
vtkCxxSetObjectMacro(vtkGhostCellsGenerator, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkGhostCellsGenerator::vtkGhostCellsGenerator()
  : Controller(nullptr)
  , NumberOfGhostLayers(1)
  , BuildIfRequired(true)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkGhostCellsGenerator::~vtkGhostCellsGenerator()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkGhostCellsGenerator::Initialize()
{
  this->NumberOfGhostLayers = 1;
  this->BuildIfRequired = true;
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
int vtkGhostCellsGenerator::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkGhostCellsGenerator::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  bool error = false;
  int retVal = 1;

  int reqGhostLayers =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  int numberOfGhostLayersToCompute =
    this->BuildIfRequired ? reqGhostLayers : std::max(reqGhostLayers, this->NumberOfGhostLayers);

  std::vector<vtkDataObject*> inputPDSs, outputPDSs;

  if (auto inputPDSC = vtkPartitionedDataSetCollection::SafeDownCast(inputDO))
  {
    auto outputPDSC = vtkPartitionedDataSetCollection::SafeDownCast(outputDO);
    outputPDSC->CopyStructure(inputPDSC);

    for (unsigned int pdsId = 0; pdsId < inputPDSC->GetNumberOfPartitionedDataSets(); ++pdsId)
    {
      inputPDSs.emplace_back(inputPDSC->GetPartitionedDataSet(pdsId));
      outputPDSs.emplace_back(outputPDSC->GetPartitionedDataSet(pdsId));
    }
  }
  else
  {
    inputPDSs.emplace_back(inputDO);
    outputPDSs.emplace_back(outputDO);
  }

  for (int partitionId = 0; partitionId < static_cast<int>(inputPDSs.size()); ++partitionId)
  {
    vtkDataObject* inputPartition = inputPDSs[partitionId];
    vtkDataObject* outputPartition = outputPDSs[partitionId];

    if (auto outputCDS = vtkDataObjectTree::SafeDownCast(outputPartition))
    {
      if (auto inputCDS = vtkDataObjectTree::SafeDownCast(inputPartition))
      {
        using Opts = vtk::DataObjectTreeOptions;
        outputCDS->CopyStructure(inputCDS);
        auto outputs = vtk::Range(outputCDS, Opts::VisitOnlyLeaves | Opts::TraverseSubTree);
        auto inputs = vtk::Range(inputCDS, Opts::VisitOnlyLeaves | Opts::TraverseSubTree);
        for (auto inIt = inputs.begin(), outIt = outputs.begin(); inIt != inputs.end();
             ++inIt, ++outIt)
        {
          if (*inIt)
          {
            *outIt = vtkSmartPointer<vtkDataObject>::Take(inIt->NewInstance());
          }
          else
          {
            *outIt = nullptr;
          }
        }
      }
      else
      {
        error = true;
      }
    }
    else if (!vtkDataSet::SafeDownCast(outputPartition) ||
      !vtkDataSet::SafeDownCast(inputPartition))
    {
      error = true;
    }

    if (vtkHyperTreeGrid::SafeDownCast(inputPartition) ||
      vtkExplicitStructuredGrid::SafeDownCast(inputPartition))
    {
      error = true;
      vtkErrorMacro(<< "Input data set type " << inputPartition->GetClassName()
                    << " not supported. The input will be shallow copied into the output.");
    }

    if (error)
    {
      vtkErrorMacro(<< "Could not generate ghosts in output.");
      outputPartition->ShallowCopy(inputPartition);
      continue;
    }

    std::vector<vtkImageData*> inputsID =
      vtkCompositeDataSet::GetDataSets<vtkImageData>(inputPartition);
    std::vector<vtkImageData*> outputsID =
      vtkCompositeDataSet::GetDataSets<vtkImageData>(outputPartition);

    std::vector<vtkRectilinearGrid*> inputsRG =
      vtkCompositeDataSet::GetDataSets<vtkRectilinearGrid>(inputPartition);
    std::vector<vtkRectilinearGrid*> outputsRG =
      vtkCompositeDataSet::GetDataSets<vtkRectilinearGrid>(outputPartition);

    std::vector<vtkStructuredGrid*> inputsSG =
      vtkCompositeDataSet::GetDataSets<vtkStructuredGrid>(inputPartition);
    std::vector<vtkStructuredGrid*> outputsSG =
      vtkCompositeDataSet::GetDataSets<vtkStructuredGrid>(outputPartition);

    std::vector<vtkUnstructuredGrid*> inputsUG =
      vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(inputPartition);
    std::vector<vtkUnstructuredGrid*> outputsUG =
      vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(outputPartition);

    std::vector<vtkPolyData*> inputsPD =
      vtkCompositeDataSet::GetDataSets<vtkPolyData>(inputPartition);
    std::vector<vtkPolyData*> outputsPD =
      vtkCompositeDataSet::GetDataSets<vtkPolyData>(outputPartition);

    if (!inputsID.empty() && !inputsRG.empty() && !inputsSG.empty() && !inputsUG.empty())
    {
      vtkWarningMacro(<< "Ghost cell generator called with mixed types."
                      << "Ghosts are not exchanged between data sets of different types.");
    }

    retVal &= vtkDIYGhostUtilities::GenerateGhostCellsImageData(
                inputsID, outputsID, numberOfGhostLayersToCompute, this->Controller) &&
      vtkDIYGhostUtilities::GenerateGhostCellsRectilinearGrid(
        inputsRG, outputsRG, numberOfGhostLayersToCompute, this->Controller) &&
      vtkDIYGhostUtilities::GenerateGhostCellsStructuredGrid(
        inputsSG, outputsSG, numberOfGhostLayersToCompute, this->Controller) &&
      vtkDIYGhostUtilities::GenerateGhostCellsUnstructuredGrid(
        inputsUG, outputsUG, numberOfGhostLayersToCompute, this->Controller) &&
      vtkDIYGhostUtilities::GenerateGhostCellsPolyData(
        inputsPD, outputsPD, numberOfGhostLayersToCompute, this->Controller);
  }

  return retVal && !error;
}

//----------------------------------------------------------------------------
int vtkGhostCellsGenerator::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  // we can't trust any ghost levels coming in so we notify all filters before
  // this that we don't need ghosts
  inputVector[0]->GetInformationObject(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  return 1;
}

//----------------------------------------------------------------------------
void vtkGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
VTK_ABI_NAMESPACE_END
