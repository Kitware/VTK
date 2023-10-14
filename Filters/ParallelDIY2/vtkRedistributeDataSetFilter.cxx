// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRedistributeDataSetFilter.h"

#include "vtkAppendFilter.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDIYKdTreeUtilities.h"
#include "vtkDIYUtilities.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkExtractCells.h"
#include "vtkFieldData.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkKdNode.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNativePartitioningStrategy.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPartitioningStrategy.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkPointData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLinks.h"
#include "vtkTableBasedClipDataSet.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <memory>

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/assigner.hpp)
// clang-format on

namespace
{
const char* CELL_OWNERSHIP_ARRAYNAME = "__RDSF_CELL_OWNERSHIP__";
const char* GHOST_CELL_ARRAYNAME = "__RDSF_GHOST_CELLS__";
// for returning when Strategy is nullptr
const std::vector<vtkBoundingBox> EmptyBuffer;
// for returning when Strategy is nullptr
const vtkBoundingBox EmptyBox;

// for checking whether a strategy is valid and native
bool CheckNativeStrategy(vtkPartitioningStrategy* strat)
{
  if (!strat || !vtkNativePartitioningStrategy::SafeDownCast(strat))
  {
    vtkErrorWithObjectMacro(nullptr, "Either no strategy set or it is not native");
    return false;
  }
  return true;
}
}

namespace detail
{
VTK_ABI_NAMESPACE_BEGIN
vtkBoundingBox GetBounds(vtkDataObject* dobj, diy::mpi::communicator& comm)
{
  auto lbounds = vtkDIYUtilities::GetLocalBounds(dobj);
  vtkDIYUtilities::AllReduce(comm, lbounds);
  return lbounds;
}

/**
 * Clip the dataset by the provided plane using vtkmClip.
 */
vtkSmartPointer<vtkUnstructuredGrid> ClipPlane(vtkDataSet* dataset, vtkSmartPointer<vtkPlane> plane)
{
  if (!dataset)
    return nullptr;

  vtkNew<vtkTableBasedClipDataSet> clipper;
  clipper->SetInputDataObject(dataset);
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();
  clipper->Update();

  auto clipperOutput = vtkUnstructuredGrid::SafeDownCast(clipper->GetOutputDataObject(0));
  if (clipperOutput &&
    (clipperOutput->GetNumberOfCells() > 0 || clipperOutput->GetNumberOfPoints() > 0))
  {
    return clipperOutput;
  }
  return nullptr;
}

/** Set partitions in vtkPartitionedDataSet to the exact count, merging if
 * needed.
 */
void SetPartitionCount(vtkPartitionedDataSet* pdc, unsigned int target)
{
  const auto count = pdc->GetNumberOfPartitions();
  if (count <= target)
  {
    pdc->SetNumberOfPartitions(target);
    return;
  }

  // we need to merge `count` partitions into `target`. This is done in
  // a contiguous fashion.
  vtkNew<vtkAppendFilter> appender;
  appender->MergePointsOn();
  const diy::ContiguousAssigner assigner(static_cast<int>(target), static_cast<int>(count));
  for (unsigned int cc = 0; cc < target; ++cc)
  {
    std::vector<int> lids;
    assigner.local_gids(cc, lids);
    for (const auto& lid : lids)
    {
      if (auto ptd = pdc->GetPartition(lid))
      {
        appender->AddInputDataObject(ptd);
      }
    }

    if (appender->GetNumberOfInputConnections(0) > 0)
    {
      appender->Update();
      appender->RemoveAllInputs();

      vtkNew<vtkUnstructuredGrid> clone;
      clone->ShallowCopy(appender->GetOutputDataObject(0));
      pdc->SetPartition(cc, clone);
    }
  }
  pdc->SetNumberOfPartitions(target);
}

VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkRedistributeDataSetFilter);

//------------------------------------------------------------------------------
vtkCxxSetSmartPointerMacro(vtkRedistributeDataSetFilter, Strategy, vtkPartitioningStrategy);

//------------------------------------------------------------------------------
vtkRedistributeDataSetFilter::vtkRedistributeDataSetFilter()
  : Assigner(nullptr)
  , Controller(nullptr)
  , BoundaryMode(vtkRedistributeDataSetFilter::ASSIGN_TO_ONE_REGION)
  , PreservePartitionsInOutput(false)
  , GenerateGlobalCellIds(true)
  , EnableDebugging(false)
  , ValidDim{ true, true, true }
  , Strategy(vtkSmartPointer<vtkNativePartitioningStrategy>::New())
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkRedistributeDataSetFilter::~vtkRedistributeDataSetFilter()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
vtkMTimeType vtkRedistributeDataSetFilter::GetMTime()
{
  if (!this->Strategy)
  {
    return this->Superclass::GetMTime();
  }
  return std::max(this->Superclass::GetMTime(), this->Strategy->GetMTime());
}

//------------------------------------------------------------------------------
int vtkRedistributeDataSetFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::SetController(vtkMultiProcessController* ctr)
{
  if (this->Strategy)
  {
    this->Strategy->SetController(ctr);
  }
  vtkSetObjectBodyMacro(Controller, vtkMultiProcessController, ctr);
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::SetExplicitCuts(const std::vector<vtkBoundingBox>& boxes)
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  native->SetExplicitCuts(boxes);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::RemoveAllExplicitCuts()
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  native->RemoveAllExplicitCuts();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::AddExplicitCut(const vtkBoundingBox& bbox)
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  native->AddExplicitCut(bbox);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::AddExplicitCut(const double bounds[6])
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  native->AddExplicitCut(bounds);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkRedistributeDataSetFilter::GetNumberOfExplicitCuts() const
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return 0;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  return native->GetNumberOfExplicitCuts();
}

//------------------------------------------------------------------------------
const vtkBoundingBox& vtkRedistributeDataSetFilter::GetExplicitCut(int index) const
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return ::EmptyBox;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  return native->GetExplicitCut(index);
}

//------------------------------------------------------------------------------
const std::vector<vtkBoundingBox>& vtkRedistributeDataSetFilter::GetExplicitCuts() const
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return ::EmptyBuffer;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  return native->GetExplicitCuts();
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::SetUseExplicitCuts(bool use)
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  native->SetUseExplicitCuts(use);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkRedistributeDataSetFilter::GetUseExplicitCuts() const
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return false;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  return native->GetUseExplicitCuts();
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::SetExpandExplicitCuts(bool use)
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  native->SetExpandExplicitCuts(use);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkRedistributeDataSetFilter::GetExpandExplicitCuts() const
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return false;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  return native->GetExpandExplicitCuts();
}

//------------------------------------------------------------------------------
const std::vector<vtkBoundingBox>& vtkRedistributeDataSetFilter::GetCuts() const
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return ::EmptyBuffer;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  return native->GetCuts();
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::SetLoadBalanceAcrossAllBlocks(bool use)
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  native->SetLoadBalanceAcrossAllBlocks(use);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkRedistributeDataSetFilter::GetLoadBalanceAcrossAllBlocks()
{
  if (!::CheckNativeStrategy(this->Strategy))
  {
    return false;
  }
  auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
  return native->GetLoadBalanceAcrossAllBlocks();
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::SetNumberOfPartitions(vtkIdType parts)
{
  if (!this->Strategy)
  {
    vtkErrorMacro("No strategy set");
    return;
  }
  this->Strategy->SetNumberOfPartitions(parts);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkIdType vtkRedistributeDataSetFilter::GetNumberOfPartitions() const
{
  if (!this->Strategy)
  {
    vtkErrorMacro("No strategy set");
    return false;
  }
  return this->Strategy->GetNumberOfPartitions();
}

//------------------------------------------------------------------------------
vtkPartitioningStrategy* vtkRedistributeDataSetFilter::GetStrategy()
{
  vtkDebugMacro(<< " returning Strategy address "
                << static_cast<vtkPartitioningStrategy*>(this->Strategy));
  return this->Strategy;
}

//------------------------------------------------------------------------------
int vtkRedistributeDataSetFilter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = vtkDataObject::GetData(outputVector, 0);
  auto outInfo = outputVector->GetInformationObject(0);

  if (vtkPartitionedDataSetCollection::SafeDownCast(inputDO) ||
    (vtkMultiBlockDataSet::SafeDownCast(inputDO) != nullptr && this->PreservePartitionsInOutput))
  {
    if (!vtkPartitionedDataSetCollection::SafeDownCast(outputDO))
    {
      auto output = vtkPartitionedDataSetCollection::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
  }
  else if (vtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    if (!vtkMultiBlockDataSet::SafeDownCast(outputDO))
    {
      auto output = vtkMultiBlockDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
  }
  else if (vtkPartitionedDataSet::SafeDownCast(inputDO) || this->PreservePartitionsInOutput)
  {
    if (!vtkPartitionedDataSet::SafeDownCast(outputDO))
    {
      auto output = vtkPartitionedDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
  }
  else if (vtkUnstructuredGrid::SafeDownCast(outputDO) == nullptr)
  {
    auto output = vtkUnstructuredGrid::New();
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    output->FastDelete();
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkRedistributeDataSetFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = vtkDataObject::GetData(outputVector, 0);

  // a flag used to avoid changing input structure.
  // this is primarily used for multiblock inputs so that we don't
  // accidentally change the input structure.
  bool preserve_input_hierarchy = false;

  // ******************************************************
  // Step 1: Convert input to vtkPartitionedDataSetCollection
  // ******************************************************
  vtkSmartPointer<vtkPartitionedDataSetCollection> inputCollection;
  if (auto inputMB = vtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    // convert MB to PDC.
    vtkNew<vtkDataAssembly> hierarchyUnused;
    inputCollection = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
    if (!vtkDataAssemblyUtilities::GenerateHierarchy(inputMB, hierarchyUnused, inputCollection))
    {
      vtkErrorMacro("Failed to generate hierarchy for input!");
      return 0;
    }

    // if this->PreservePartitionsInOutput, we need to preserve input hierarchy.
    preserve_input_hierarchy = !this->PreservePartitionsInOutput;
  }
  else if (auto inputPTD = vtkPartitionedDataSet::SafeDownCast(inputDO))
  {
    // input it PD, simply put it in a new collection.
    inputCollection.TakeReference(vtkPartitionedDataSetCollection::New());
    inputCollection->SetPartitionedDataSet(0, inputPTD);
  }
  else if (auto inputPTDC = vtkPartitionedDataSetCollection::SafeDownCast(inputDO))
  {
    // nothing to do!
    inputCollection = inputPTDC;
  }
  else
  {
    // input it some other dataset type, simply put it in a new collection.
    inputCollection.TakeReference(vtkPartitionedDataSetCollection::New());
    inputCollection->SetPartition(0, 0, inputDO);
  }

  // ******************************************************
  // Execute core algorithm now on the vtkPartitionedDataSetCollection as a
  // whole or each vtkPartitionedDataSet in the collection based on user
  // selection.
  // ******************************************************
  vtkNew<vtkPartitionedDataSetCollection> result;
  result->CopyStructure(inputCollection);

  /*
   * Use Strategy to compute the partitions without exchanging any actual data
   */
  auto partitionInformation = this->Strategy->ComputePartition(inputCollection);
  this->UpdateProgress(0.5);
  this->SetProgressShiftScale(0.5, 0.9);

  /*
   * Use the partitions generated by the strategy to redistribute the data
   */
  if (!this->Redistribute(inputCollection, result, partitionInformation, preserve_input_hierarchy))
  {
    vtkErrorMacro("Redistribution failed");
    return 0;
  }

  std::vector<vtkDataSet*> resultVector = vtkCompositeDataSet::GetDataSets(result);
  for (vtkDataSet* ds : resultVector)
  {
    // Ghost arrays become irrelevant after this filter is done, we remove them.
    ds->GetPointData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
    ds->GetCellData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
  }

  // ******************************************************
  // Now, package the result into the output.
  // ******************************************************
  vtkPartitionedDataSetCollection* outputPDSC =
    vtkPartitionedDataSetCollection::SafeDownCast(outputDO);
  vtkPartitionedDataSet* outputPDS = vtkPartitionedDataSet::SafeDownCast(outputDO);
  vtkMultiBlockDataSet* outputMB = vtkMultiBlockDataSet::SafeDownCast(outputDO);

  if (outputPDSC)
  {
    outputPDSC->CompositeShallowCopy(result);
  }
  else if (outputPDS)
  {
    if (result->GetNumberOfPartitionedDataSets() != 1)
    {
      vtkErrorMacro("Number of partitioned data sets in result  incorrect");
      return 0;
    }
    outputPDS->CompositeShallowCopy(result->GetPartitionedDataSet(0));
  }
  else if (outputMB)
  {
    // convert result (vtkPartitionedDataSetCollection) to vtkMultiBlockDataSet.
    if (auto mbresult = vtkDataAssemblyUtilities::GenerateCompositeDataSetFromHierarchy(
          result, result->GetDataAssembly()))
    {
      outputMB->CompositeShallowCopy(mbresult);
    }
    else
    {
      vtkErrorMacro("Failed to convert back to vtkMultiBlockDataSet.");
    }
  }
  else
  {
    if (!vtkUnstructuredGrid::SafeDownCast(outputDO))
    {
      vtkErrorMacro("Output should be unstructured grid and is not");
      return 0;
    }

    vtkNew<vtkAppendFilter> appender;
    appender->MergePointsOn();

    using Opts = vtk::DataObjectTreeOptions;
    for (vtkDataObject* part : vtk::Range(result.GetPointer(),
           Opts::SkipEmptyNodes | Opts::VisitOnlyLeaves | Opts::TraverseSubTree))
    {
      if (!part)
      {
        vtkErrorMacro("Part is nullptr and should have been skipped");
        return 0;
      }
      appender->AddInputDataObject(part);
    }
    if (appender->GetNumberOfInputConnections(0) > 1)
    {
      appender->Update();
      outputDO->ShallowCopy(appender->GetOutputDataObject(0));
    }
    else if (appender->GetNumberOfInputConnections(0) == 1)
    {
      outputDO->ShallowCopy(appender->GetInputDataObject(0, 0));
    }
    outputDO->GetFieldData()->PassData(inputDO->GetFieldData());
  }

  this->SetProgressShiftScale(0.0, 1.0);
  this->UpdateProgress(1.0);
  return 1;
}

//------------------------------------------------------------------------------
bool vtkRedistributeDataSetFilter::Redistribute(vtkPartitionedDataSetCollection* inputCollection,
  vtkPartitionedDataSetCollection* outputCollection,
  const std::vector<vtkPartitioningStrategy::PartitionInformation>& info,
  bool preserve_input_hierarchy)
{
  // an offset counters used to ensure cell global ids, if requested are
  // assigned uniquely across all blocks.
  vtkIdType mb_offset = 0;
  // an running counter for offsetting the partitions in the info vector
  unsigned int ptdOffset = 0;
  for (unsigned int part = 0, max = inputCollection->GetNumberOfPartitionedDataSets(); part < max;
       ++part)
  {
    auto inputPTD = inputCollection->GetPartitionedDataSet(part);

    auto outputPTD = outputCollection->GetPartitionedDataSet(part);

    if (!inputPTD || !outputPTD)
    {
      vtkErrorMacro("Input or output partitioned data set is nullptr");
      return false;
    }

    // redistribute each block using cuts already computed.
    if (!this->RedistributePTD(inputPTD, outputPTD, info, &ptdOffset, &mb_offset))
    {
      vtkErrorMacro("Failed in redistribution of single PartitionedDataSet");
      return false;
    }

    if (!this->EnableDebugging)
    {
      // let's prune empty partitions; not necessary, but should help
      // avoid people reading too much into the partitions generated
      // on each rank.
      outputPTD->RemoveNullPartitions();
    }

    const auto inCount = inputPTD->GetNumberOfPartitions();
    const auto outCount = outputPTD->GetNumberOfPartitions();
    if (preserve_input_hierarchy && inCount > outCount)
    {
      detail::SetPartitionCount(outputPTD, inCount);
    }
    this->UpdateProgress(part / max);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkRedistributeDataSetFilter::RedistributePTD(vtkPartitionedDataSet* inputPDS,
  vtkPartitionedDataSet* outputPDS,
  const std::vector<vtkPartitioningStrategy::PartitionInformation>& info, unsigned int* ptdOffset,
  vtkIdType* mb_offset)
{
  if (!inputPDS || !outputPDS)
  {
    vtkErrorMacro("Either input or output PartitionedDataSet is nullptr");
    return false;
  }

  // assign global cell ids to inputDO, if not present.
  // we do this assignment before distributing cells if boundary mode is not
  // set to SPLIT_BOUNDARY_CELLS in which case we do after the split.
  vtkSmartPointer<vtkPartitionedDataSet> xfmedInput;
  if (this->GenerateGlobalCellIds && this->BoundaryMode != SPLIT_BOUNDARY_CELLS)
  {
    xfmedInput = this->AssignGlobalCellIds(inputPDS, mb_offset);
  }
  else
  {
    xfmedInput = inputPDS;
  }

  // We are distributing a vtkPartitionedDataSet. Our strategy is simple:
  // we split and distribute each input partition individually.
  // We then merge corresponding parts together to form the output partitioned
  // dataset.
  std::vector<vtkDataSet*> input_partitions;
  for (unsigned int cc = 0; cc < xfmedInput->GetNumberOfPartitions(); ++cc)
  {
    auto ds = xfmedInput->GetPartition(cc);
    if (ds && (ds->GetNumberOfPoints() > 0 || ds->GetNumberOfCells() > 0))
    {
      input_partitions.emplace_back(ds);
    }
    else
    {
      input_partitions.emplace_back(nullptr);
    }
  }

  auto controller = this->GetController();
  if (controller && controller->GetNumberOfProcesses() > 1)
  {
    unsigned int mysize = static_cast<unsigned int>(input_partitions.size());
    unsigned int allsize = 0;
    controller->AllReduce(&mysize, &allsize, 1, vtkCommunicator::MAX_OP);
    assert(allsize >= mysize);
    input_partitions.resize(allsize, nullptr);
  }

  if (input_partitions.empty())
  {
    // all ranks have empty data.
    return true;
  }

  if (!info.empty())
  {
    if (info.size() <= *ptdOffset)
    {
      vtkErrorMacro("Information about partitions not lining up with partition number offsetting");
      return false;
    }
    else
    {
      outputPDS->SetNumberOfPartitions(
        static_cast<unsigned int>(info[*ptdOffset].NumberOfPartitions));
    }
  }

  std::vector<vtkSmartPointer<vtkPartitionedDataSet>> results;
  int inputPartId = 0;
  for (auto& ds : input_partitions)
  {
    vtkNew<vtkPartitionedDataSet> curOutput;
    if (this->RedistributeDataSet(ds, curOutput, info[*ptdOffset + inputPartId]))
    {
      if (curOutput->GetNumberOfPartitions() !=
        static_cast<unsigned int>(info[*ptdOffset + inputPartId].NumberOfPartitions))
      {
        vtkWarningMacro("Number of partitions not lining up");
      }
      results.emplace_back(curOutput);
    }
    inputPartId++;
  }
  *ptdOffset += static_cast<unsigned int>(input_partitions.size());

  // TODO: this may not be necessary -- need to see if we can avoid this
  // entirely to speed things up or make it optional.
  // combine leaf nodes an all parts in the results to generate the output.
  for (unsigned int part = 0; part < outputPDS->GetNumberOfPartitions(); ++part)
  {
    vtkNew<vtkAppendFilter> appender;
    appender->MergePointsOn();
    for (auto& pds : results)
    {
      if (auto ds = pds->GetPartition(part))
      {
        appender->AddInputDataObject(ds);
      }
    }
    if (appender->GetNumberOfInputConnections(0) == 1)
    {
      outputPDS->SetPartition(part, appender->GetInputDataObject(0, 0));
    }
    else if (appender->GetNumberOfInputConnections(0) > 1)
    {
      appender->Update();
      outputPDS->SetPartition(part, appender->GetOutputDataObject(0));
    }
  }

  switch (this->GetBoundaryMode())
  {
    case vtkRedistributeDataSetFilter::SPLIT_BOUNDARY_CELLS:
    {
      // This boundary mode can really only work with the native partitioning strategy
      auto native = vtkNativePartitioningStrategy::SafeDownCast(this->Strategy);
      if (native && native->GetLoadBalanceAcrossAllBlocks())
      {
        // by this point, boundary cells have been cloned on all boundary ranks.
        // locally, we will now simply clip each dataset by the corresponding
        // partition bounds.
        auto comm = vtkDIYUtilities::GetCommunicator(this->Controller);
        auto gbounds = detail::GetBounds(outputPDS, comm);
        this->MarkValidDimensions(gbounds);
        for (unsigned int cc = 0, max = outputPDS->GetNumberOfPartitions(); cc < max; ++cc)
        {
          if (auto ds = outputPDS->GetPartition(cc))
          {
            outputPDS->SetPartition(cc, this->ClipDataSet(ds, native->GetCuts()[cc]));
          }
        }

        if (this->GenerateGlobalCellIds)
        {
          auto result = this->AssignGlobalCellIds(outputPDS, mb_offset);
          outputPDS->CompositeShallowCopy(result);
        }
        break;
      }
      vtkWarningMacro("The SPLIT_BOUNDARY_CELLS boundary mode only works for the native strategy "
                      "when LoadBalanceAcrossAllBlocks is on. Defaulting to the "
                      "ASSIGN_TO_ALL_INTERSECTING_REGIONS mode.");
      this->MarkGhostCells(outputPDS);
    }
    break;

    case vtkRedistributeDataSetFilter::ASSIGN_TO_ALL_INTERSECTING_REGIONS:
      // mark ghost cells using cell ownership information generated in
      // `SplitDataSet`.
      this->MarkGhostCells(outputPDS);
      break;

    case vtkRedistributeDataSetFilter::ASSIGN_TO_ONE_REGION:
      // nothing to do, since we already assigned cells uniquely when splitting.
      break;

    default:
      // nothing to do.
      break;
  }

  if (!this->EnableDebugging)
  {
    // drop internal arrays
    for (unsigned int partId = 0, max = outputPDS->GetNumberOfPartitions(); partId < max; ++partId)
    {
      if (auto dataset = outputPDS->GetPartition(partId))
      {
        dataset->GetCellData()->RemoveArray(CELL_OWNERSHIP_ARRAYNAME);
        if (auto arr = dataset->GetCellData()->GetArray(GHOST_CELL_ARRAYNAME))
        {
          arr->SetName(vtkDataSetAttributes::GhostArrayName());
        }
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkRedistributeDataSetFilter::RedistributeDataSet(vtkDataSet* inputDS,
  vtkPartitionedDataSet* outputPDS, const vtkPartitioningStrategy::PartitionInformation& info)
{
  // note: inputDS can be null.
  auto parts = this->SplitDataSet(inputDS, info);
  if (parts->GetNumberOfPartitions() != static_cast<unsigned int>(info.NumberOfPartitions))
  {
    vtkWarningMacro("Did not split into correct number of parts");
  }

  auto pieces = vtkDIYKdTreeUtilities::Exchange(parts, this->GetController(), this->Assigner);
  if (pieces->GetNumberOfPartitions() != parts->GetNumberOfPartitions())
  {
    vtkWarningMacro("Did not exchange into correct number of pieces");
  }
  outputPDS->CompositeShallowCopy(pieces);
  return true;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataSet> vtkRedistributeDataSetFilter::ClipDataSet(
  vtkDataSet* dataset, const vtkBoundingBox& bbox)
{
  if (!dataset)
  {
    vtkErrorMacro("Cannot clip nullptr dataset");
    return nullptr;
  }

  double bounds[6];
  bbox.GetBounds(bounds);
  vtkNew<vtkPlanes> box_planes;
  box_planes->SetBounds(bounds);

  vtkSmartPointer<vtkUnstructuredGrid> clipperOutput;
  for (int i = 0; i < box_planes->GetNumberOfPlanes(); ++i)
  {
    int dim = i / 2;
    // Only clip if this dimension in the original dataset's bounding box
    // (before redistribution) had a non-zero length, so we don't accidentally
    // clip away the full dataset.
    if (this->ValidDim[dim])
    {
      if (!clipperOutput)
      {
        clipperOutput = detail::ClipPlane(dataset, box_planes->GetPlane(i));
      }
      else
      {
        clipperOutput = detail::ClipPlane(clipperOutput, box_planes->GetPlane(i));
      }
    }
  }

  if (clipperOutput &&
    (clipperOutput->GetNumberOfCells() > 0 || clipperOutput->GetNumberOfPoints() > 0))
  {
    return clipperOutput;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPartitionedDataSet> vtkRedistributeDataSetFilter::SplitDataSet(
  vtkDataSet* dataset, const vtkPartitioningStrategy::PartitionInformation& info)
{
  if (!dataset || info.NumberOfPartitions == 0 || dataset->GetNumberOfCells() == 0)
  {
    vtkNew<vtkPartitionedDataSet> result;
    result->SetNumberOfPartitions(static_cast<unsigned int>(info.NumberOfPartitions));
    return result;
  }

  const auto numCells = dataset->GetNumberOfCells();
  const bool duplicate_cells =
    this->GetBoundaryMode() != vtkRedistributeDataSetFilter::ASSIGN_TO_ONE_REGION;

  // cell_ownership value should be set to -1 is the cell doesn't belong to any cut
  // else it's set to the index of the correct partition.
  vtkSmartPointer<vtkIdTypeArray> cell_ownership;
  if (duplicate_cells)
  {
    // unless duplicating cells along boundary, no need to set the
    // cell_ownership array. cell_ownership array is used to mark ghost cells
    // later on which don't exist if boundary cells are not duplicated.
    cell_ownership = info.TargetPartitions;
    cell_ownership->SetName(CELL_OWNERSHIP_ARRAYNAME);
  }

  // convert cell_regions to a collection of cell-ids for each region so that we
  // can use `vtkExtractCells` to extract cells for each region.
  std::vector<std::vector<vtkIdType>> region_cell_ids(info.NumberOfPartitions);
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
  {
    auto part = info.TargetPartitions->GetValue(cellId);
    if (part == -1)
    {
      continue;
    }
    region_cell_ids[part].emplace_back(cellId);
  }
  if (duplicate_cells)
  {
    for (vtkIdType bId = 0; bId < info.BoundaryNeighborPartitions->GetNumberOfTuples(); ++bId)
    {
      vtkIdType tup[2];
      info.BoundaryNeighborPartitions->GetTypedTuple(bId, tup);
      region_cell_ids[tup[1]].emplace_back(tup[0]);
    }
  }

  vtkNew<vtkPartitionedDataSet> result;
  result->SetNumberOfPartitions(static_cast<unsigned int>(info.NumberOfPartitions));

  // we create a clone of the input and add the
  // cell_ownership cell arrays to it so that they are propagated to each of the
  // extracted subsets and exchanged. It will be used later on to mark
  // ghost cells.
  auto clone = vtkSmartPointer<vtkDataSet>::Take(dataset->NewInstance());
  clone->ShallowCopy(dataset);
  clone->GetCellData()->AddArray(cell_ownership);

  vtkNew<vtkExtractCells> extractor;
  extractor->SetInputDataObject(clone);
  extractor->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for (size_t region_idx = 0; region_idx < region_cell_ids.size(); ++region_idx)
  {
    const auto& cell_ids = region_cell_ids[region_idx];
    if (!cell_ids.empty())
    {
      extractor->SetCellIds(cell_ids.data(), static_cast<vtkIdType>(cell_ids.size()));
      extractor->Update();

      vtkNew<vtkUnstructuredGrid> ug;
      ug->ShallowCopy(extractor->GetOutputDataObject(0));
      result->SetPartition(static_cast<unsigned int>(region_idx), ug);
    }
  }
  return result;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataSet> vtkRedistributeDataSetFilter::AssignGlobalCellIds(
  vtkDataSet* input, vtkIdType* mb_offset /*=nullptr*/)
{
  vtkNew<vtkPartitionedDataSet> pds;
  pds->SetNumberOfPartitions(1);
  pds->SetPartition(0, input);
  auto output = this->AssignGlobalCellIds(pds, mb_offset);
  assert(output->GetNumberOfPartitions() == 1);
  return output->GetPartition(0);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPartitionedDataSet> vtkRedistributeDataSetFilter::AssignGlobalCellIds(
  vtkPartitionedDataSet* pieces, vtkIdType* mb_offset /*=nullptr*/)
{
  // if global cell ids are present everywhere, there's nothing to do!
  int missing_gids = 0;
  for (unsigned int partId = 0; partId < pieces->GetNumberOfPartitions(); ++partId)
  {
    vtkDataSet* dataset = pieces->GetPartition(partId);
    if (dataset && dataset->GetNumberOfCells() > 0 &&
      dataset->GetCellData()->GetGlobalIds() == nullptr)
    {
      missing_gids = 1;
      break;
    }
  }

  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    int any_missing_gids = 0;
    this->Controller->AllReduce(&missing_gids, &any_missing_gids, 1, vtkCommunicator::MAX_OP);
    missing_gids = any_missing_gids;
  }

  if (missing_gids == 0)
  {
    // input already has global cell ids.
    return pieces;
  }

  // We need to generate global cells ids since not all pieces (if any) have global cell
  // ids.
  vtkNew<vtkPartitionedDataSet> result;
  result->SetNumberOfPartitions(pieces->GetNumberOfPartitions());
  for (unsigned int partId = 0; partId < pieces->GetNumberOfPartitions(); ++partId)
  {
    if (auto dataset = pieces->GetPartition(partId))
    {
      auto clone = dataset->NewInstance();
      clone->ShallowCopy(dataset);
      result->SetPartition(partId, clone);
      clone->FastDelete();
    }
  }

  vtkDIYKdTreeUtilities::GenerateGlobalCellIds(result, this->Controller, mb_offset);
  return result;
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::MarkGhostCells(vtkPartitionedDataSet* pieces)
{
  for (unsigned int partId = 0; partId < pieces->GetNumberOfPartitions(); ++partId)
  {
    vtkDataSet* dataset = pieces->GetPartition(partId);
    if (dataset == nullptr || dataset->GetNumberOfCells() == 0)
    {
      continue;
    }

    auto cell_ownership =
      vtkIntArray::SafeDownCast(dataset->GetCellData()->GetArray(CELL_OWNERSHIP_ARRAYNAME));
    if (!cell_ownership)
    {
      // cell_ownership is not generated if cells are being assigned uniquely to
      // parts since in that case there are no ghost cells.
      continue;
    }

    auto ghostCells = vtkUnsignedCharArray::SafeDownCast(
      dataset->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));
    if (!ghostCells)
    {
      ghostCells = vtkUnsignedCharArray::New();
      // the array is renamed later on
      // ghostCells->SetName(vtkDataSetAttributes::GhostArrayName());
      ghostCells->SetName(GHOST_CELL_ARRAYNAME);
      ghostCells->SetNumberOfTuples(dataset->GetNumberOfCells());
      ghostCells->FillValue(0);
      dataset->GetCellData()->AddArray(ghostCells);
      ghostCells->FastDelete();
    }

    vtkSMPTools::For(0, dataset->GetNumberOfCells(), [&](vtkIdType start, vtkIdType end) {
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        // any cell now owned by the current part is marked as a ghost cell.
        const auto cell_owner = cell_ownership->GetTypedComponent(cc, 0);
        auto gflag = ghostCells->GetTypedComponent(cc, 0);
        if (static_cast<int>(partId) == cell_owner)
        {
          gflag &= (~vtkDataSetAttributes::DUPLICATECELL);
        }
        else
        {
          gflag |= vtkDataSetAttributes::DUPLICATECELL;
        }
        ghostCells->SetTypedComponent(cc, 0, gflag);
      }
    });
  }
}

//------------------------------------------------------------------------------
// Determine which dimensions in the initial bounding box (before any inflation
// of the bounds occurs) has a non-zero length. This is necessary for clipping
// when the BoundaryMode is set to SPLIT_BOUNDARY_CELLS. Otherwise if a dataset
// ends up being 2D, performing plane clips on all sides of the bounding box may
// result in full dataset being clipped away.
void vtkRedistributeDataSetFilter::MarkValidDimensions(const vtkBoundingBox& gbounds)
{
  static const int max_dim = 3;
  double len[max_dim];
  gbounds.GetLengths(len);
  for (int i = 0; i < max_dim; ++i)
  {
    if (len[i] <= 0)
    {
      this->ValidDim[i] = false;
    }
    else
    {
      this->ValidDim[i] = true;
    }
  }
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::SetAssigner(std::shared_ptr<diy::Assigner> assigner)
{
  if (this->Assigner != assigner)
  {
    this->Assigner = assigner;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
std::shared_ptr<diy::Assigner> vtkRedistributeDataSetFilter::GetAssigner()
{
  return this->Assigner;
}

//------------------------------------------------------------------------------
std::shared_ptr<const diy::Assigner> vtkRedistributeDataSetFilter::GetAssigner() const
{
  return this->Assigner;
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "BoundaryMode: " << this->BoundaryMode << endl;
  os << indent << "PreservePartitionsInOutput: " << this->PreservePartitionsInOutput << endl;
  os << indent << "GenerateGlobalCellIds: " << this->GenerateGlobalCellIds << endl;
  os << indent << "EnableDebugging: " << this->EnableDebugging << endl;
  os << indent << "Strategy:" << std::endl;
  if (this->Strategy)
  {
    this->Strategy->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent.GetNextIndent() << "nullptr" << std::endl;
  }
}
VTK_ABI_NAMESPACE_END
