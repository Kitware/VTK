/*=========================================================================
      </ProxyProperty>

  Program:   Visualization Toolkit
  Module:    vtkGhostCellsGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Hide VTK_DEPRECATED_IN_9_1_0() warning for this class
#define VTK_DEPRECATION_LEVEL 0

#include "vtkGhostCellsGenerator.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDIYGhostUtilities.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkRange.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

vtkStandardNewMacro(vtkGhostCellsGenerator);
vtkCxxSetObjectMacro(vtkGhostCellsGenerator, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkGhostCellsGenerator::vtkGhostCellsGenerator()
  : Controller(nullptr)
  , NumberOfGhostLayers(1)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkGhostCellsGenerator::~vtkGhostCellsGenerator()
{
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

  bool error = false;
  int retVal = 1;

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
        auto outputs = vtk::Range(outputCDS, Opts::None);
        auto inputs = vtk::Range(inputCDS, Opts::None);
        for (auto inIt = inputs.begin(), outIt = outputs.begin(); inIt != inputs.end();
             ++inIt, ++outIt)
        {
          *outIt = vtkSmartPointer<vtkDataObject>::Take(inIt->NewInstance());
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

    if (error)
    {
      vtkErrorMacro(<< "Could not generate output.");
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

    std::vector<vtkUnstructuredGrid*> inputsUGWithoutGhosts(inputsUG.size());
    std::vector<vtkSmartPointer<vtkUnstructuredGrid>> inputsUGWithoutGhostsCleaner(inputsUG.size());

    // FIXME
    // We do a deep copy for unstructured grids removing ghost cells.
    // Ideally, we should avoid doing such a thing and skip ghost cells in the input
    // by remapping the input to the output while ignoring the input ghosts.
    for (int localId = 0; localId < static_cast<vtkIdType>(inputsUG.size()); ++localId)
    {
      vtkUnstructuredGrid* input = inputsUG[localId];
      if (input->GetGhostArray(vtkDataObject::FIELD_ASSOCIATION_CELLS))
      {
        inputsUGWithoutGhostsCleaner[localId] = vtkSmartPointer<vtkUnstructuredGrid>::New();
        vtkUnstructuredGrid* cleanInput = inputsUGWithoutGhostsCleaner[localId];
        cleanInput->DeepCopy(input);
        cleanInput->RemoveGhostCells();
        cleanInput->GetCellData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
        cleanInput->GetPointData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
        inputsUGWithoutGhosts[localId] = cleanInput;
      }
      else
      {
        inputsUGWithoutGhosts[localId] = input;
      }
    }

    if (!inputsID.empty() && !inputsRG.empty() && !inputsSG.empty() && !inputsUG.empty())
    {
      vtkWarningMacro(<< "Ghost cell generator called with mixed types."
                      << "Ghosts are not exchanged between data sets of different types.");
    }

    retVal &= vtkDIYGhostUtilities::GenerateGhostCells(
                inputsID, outputsID, this->NumberOfGhostLayers, this->Controller) &&
      vtkDIYGhostUtilities::GenerateGhostCells(
        inputsRG, outputsRG, this->NumberOfGhostLayers, this->Controller) &&
      vtkDIYGhostUtilities::GenerateGhostCells(
        inputsSG, outputsSG, this->NumberOfGhostLayers, this->Controller) &&
      vtkDIYGhostUtilities::GenerateGhostCells(
        inputsUGWithoutGhosts, outputsUG, this->NumberOfGhostLayers, this->Controller);
  }

  return retVal && !error;
}

//----------------------------------------------------------------------------
int vtkGhostCellsGenerator::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  outputVector->GetInformationObject(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->NumberOfGhostLayers);
  return 1;
}

//----------------------------------------------------------------------------
void vtkGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
