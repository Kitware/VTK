// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGhostCellsGenerator.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDIYGhostUtilities.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkGenerateGlobalIds.h"
#include "vtkGenerateProcessIds.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
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
struct vtkGhostCellsGenerator::StaticMeshCache
{
  // Output with ghost cells already generated
  vtkSmartPointer<vtkDataObject> Cache;
  vtkMTimeType CachedMeshMTime = 0;
  bool Initialized = false;
};

//----------------------------------------------------------------------------
/**
 * Interface to dispatch work over every contained vtkDataSet.
 * If input is a vtkDataSet subclass, forward it directly to
 * ComputeDataSet.
 * If input is a vtkDataObjectTree subclass, iterate over
 * inner non empty vtkDataSet leaves.
 * @note From vtkDataObjectMeshCache
 */
struct GenericDataObjectWorker
{
  virtual ~GenericDataObjectWorker() = default;

  /**
   * Entry point. In the end, call ComputeDataSet for every
   * contained vtkDataSet.
   */
  void Compute(vtkDataObject* dataobject)
  {
    auto dataset = vtkDataSet::SafeDownCast(dataobject);
    if (dataset)
    {
      this->ComputeDataSet(dataset);
      return;
    }
    auto composite = vtkDataObjectTree::SafeDownCast(dataobject);
    if (composite)
    {
      this->ComputeComposite(composite);
      return;
    }

    this->SkippedData = true;
  }

  /**
   * Iterate over inner vtkDataSet to call ComputeDataSet
   */
  void ComputeComposite(vtkDataObjectTree* composite)
  {
    auto options = vtk::DataObjectTreeOptions::TraverseSubTree |
      vtk::DataObjectTreeOptions::SkipEmptyNodes | vtk::DataObjectTreeOptions::VisitOnlyLeaves;
    for (auto dataLeaf : vtk::Range(composite, options))
    {
      auto dataset = vtkDataSet::SafeDownCast(dataLeaf);
      if (dataset)
      {
        this->ComputeDataSet(dataset);
      }
      else
      {
        this->SkippedData = true;
      }
    }
  }

  /**
   * To be reimplemented to do the actual work.
   * Will be called multiple times for composite.
   */
  virtual void ComputeDataSet(vtkDataSet* dataset) = 0;

  bool SkippedData = false;
};

/**
 * Worker to compute mesh mtime.
 * For composite, return the max value.
 * @note From vtkDataObjectMeshCache
 */
struct MeshMTimeWorker : public GenericDataObjectWorker
{
  ~MeshMTimeWorker() override = default;

  void ComputeDataSet(vtkDataSet* dataset) override
  {
    auto polydata = vtkPolyData::SafeDownCast(dataset);
    auto ugrid = vtkUnstructuredGrid::SafeDownCast(dataset);

    if (polydata)
    {
      this->MeshTime = std::max(this->MeshTime, polydata->GetMeshMTime());
    }

    if (ugrid)
    {
      this->MeshTime = std::max(this->MeshTime, ugrid->GetMeshMTime());
    }
  }

  vtkMTimeType MeshTime = 0;
};

/**
 * @brief Helper to get the Mesh Modified time of any type of dataset
 * @note From vtkDataObjectMeshCache
 * @note No longer necessary when vtkCompositeDataset / vtkDataset support GetMeshMTime themselves
 */
vtkMTimeType GetMeshMTime(vtkDataObject* input)
{
  MeshMTimeWorker meshtime;
  meshtime.Compute(input);
  return meshtime.MeshTime;
}

//----------------------------------------------------------------------------
vtkGhostCellsGenerator::vtkGhostCellsGenerator()
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->MeshCache = std::make_shared<StaticMeshCache>();
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
vtkMultiProcessController* vtkGhostCellsGenerator::GetController()
{
  return this->Controller.Get();
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
  return this->Execute(vtkDataObject::GetData(inputVector[0], 0), outputVector);
}

//----------------------------------------------------------------------------
int vtkGhostCellsGenerator::Execute(vtkDataObject* inputDO, vtkInformationVector* outputVector)
{
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int retVal = 1;

  vtkSmartPointer<vtkDataObject> modifInputDO =
    vtkSmartPointer<vtkDataObject>::Take(inputDO->NewInstance());
  modifInputDO->ShallowCopy(inputDO);
  if (this->GenerateProcessIds)
  {
    vtkNew<vtkGenerateProcessIds> pidGenerator;
    pidGenerator->SetInputData(modifInputDO);
    pidGenerator->GenerateCellDataOn();
    pidGenerator->GeneratePointDataOn();
    pidGenerator->Update();
    modifInputDO->ShallowCopy(pidGenerator->GetOutputDataObject(0));
  }
  if (this->GenerateGlobalIds)
  {
    vtkNew<vtkGenerateGlobalIds> gidGenerator;
    gidGenerator->SetInputData(modifInputDO);
    gidGenerator->Update();
    modifInputDO->ShallowCopy(gidGenerator->GetOutputDataObject(0));
  }

  int reqGhostLayers =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (this->UseStaticMeshCache)
  {
    if (!this->MeshCache->Initialized)
    {
      this->MeshCache->Cache.TakeReference(inputDO->NewInstance());
      this->MeshCache->Initialized = true;
      this->MeshCache->CachedMeshMTime = 0;
    }

    vtkMTimeType inputMeshMTime = GetMeshMTime(inputDO);
    if (this->MeshCache->CachedMeshMTime < inputMeshMTime)
    {
      // Generate ghost cells and cache them
      retVal &= this->GenerateGhostCells(inputDO, this->MeshCache->Cache, reqGhostLayers, false);
      this->MeshCache->CachedMeshMTime = inputMeshMTime;

      outputDO->ShallowCopy(this->MeshCache->Cache);
    }
    else
    {
      // Sync only
      retVal &= this->GenerateGhostCells(inputDO, this->MeshCache->Cache, reqGhostLayers, true);
      outputDO->ShallowCopy(this->MeshCache->Cache);
    }
  }
  else
  {
    retVal &=
      this->GenerateGhostCells(modifInputDO, outputDO, reqGhostLayers, this->SynchronizeOnly);
  }

  return retVal;
}

//----------------------------------------------------------------------------
int vtkGhostCellsGenerator::GenerateGhostCells(
  vtkDataObject* inputDO, vtkDataObject* outputDO, int reqGhostLayers, bool syncOnly)
{
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

    // Note: We synchronize only if both points AND cells can be synchronized, it would be possible
    // to improve that if the generating part is able to generate only cells or points at some
    // point.
    bool canSyncCell = false;
    bool canSyncPoint = false;
    if (syncOnly &&
      vtkGhostCellsGenerator::CanSynchronize(inputPartition, canSyncCell, canSyncPoint))
    {
      std::vector<vtkDataSet*> inputsDS =
        vtkCompositeDataSet::GetDataSets<vtkDataSet>(inputPartition);
      std::vector<vtkDataSet*> outputsDS =
        vtkCompositeDataSet::GetDataSets<vtkDataSet>(outputPartition);
      retVal &= vtkDIYGhostUtilities::SynchronizeGhostData(
        inputsDS, outputsDS, this->Controller, canSyncCell, canSyncPoint);
    }
    else
    {
      int numberOfGhostLayersToCompute = this->BuildIfRequired
        ? reqGhostLayers
        : std::max(reqGhostLayers, this->NumberOfGhostLayers);

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
bool vtkGhostCellsGenerator::CanSynchronize(
  vtkDataObject* input, bool& canSyncCell, bool& canSyncPoint)
{
  vtkDataSetAttributes* inputCell = input->GetAttributes(vtkDataObject::AttributeTypes::CELL);
  vtkDataSetAttributes* inputPoint = input->GetAttributes(vtkDataObject::AttributeTypes::POINT);
  canSyncCell = inputCell && inputCell->GetGhostArray() && inputCell->GetGlobalIds() &&
    inputCell->GetProcessIds();
  canSyncPoint = inputPoint && inputPoint->GetGhostArray() && inputPoint->GetGlobalIds() &&
    inputPoint->GetProcessIds();

  return canSyncCell && canSyncPoint;
}

//----------------------------------------------------------------------------
void vtkGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
VTK_ABI_NAMESPACE_END
