/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRedistributeDataSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRedistributeDataSetFilter.h"

#include "vtkAppendFilter.h"
#include "vtkCellData.h"
#include "vtkDIYKdTreeUtilities.h"
#include "vtkDIYUtilities.h"
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
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkPointData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLinks.h"
#include "vtkTableBasedClipDataSet.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

namespace
{
static const char* CELL_OWNERSHIP_ARRAYNAME = "__RDSF_CELL_OWNERSHIP__";
static const char* GHOST_CELL_ARRAYNAME = "__RDSF_GHOST_CELLS__";
}

namespace detail
{
vtkBoundingBox GetBounds(vtkDataObject* dobj)
{
  if (auto pds = vtkPartitionedDataSet::SafeDownCast(dobj))
  {
    double bds[6];
    pds->GetBounds(bds);
    return vtkBoundingBox(bds);
  }
  else if (auto mbds = vtkMultiBlockDataSet::SafeDownCast(dobj))
  {
    double bds[6];
    mbds->GetBounds(bds);
    return vtkBoundingBox(bds);
  }
  else if (auto ds = vtkDataSet::SafeDownCast(dobj))
  {
    double bds[6];
    ds->GetBounds(bds);
    return vtkBoundingBox(bds);
  }
  return vtkBoundingBox();
}

/**
 * For each cell in the `dataset`, this function will return the cut-indexes for
 * the `cuts` provided that the cell belongs to. If `duplicate_boundary_cells` is
 * `true`, the for boundary cells, there will be multiple cut-indexes that the
 * cell may belong to. Otherwise, a cell can belong to at most 1 region.
 */
std::vector<std::vector<int> > GenerateCellRegions(
  vtkDataSet* dataset, const std::vector<vtkBoundingBox>& cuts, bool duplicate_boundary_cells)
{
  assert(dataset != nullptr && cuts.size() > 0 && dataset->GetNumberOfCells() > 0);

  auto ghostCells = vtkUnsignedCharArray::SafeDownCast(
    dataset->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));

  std::vector<std::vector<int> > cellRegions(dataset->GetNumberOfCells());

  // call GetCell/GetCellBounds once to make it thread safe (see vtkDataSet::GetCell).
  vtkNew<vtkGenericCell> acell;
  dataset->GetCell(0, acell);
  double bds[6];
  dataset->GetCellBounds(0, bds);

  const auto numCells = dataset->GetNumberOfCells();
  if (duplicate_boundary_cells)
  {
    // vtkKdNode helps us do fast cell/cut intersections. So convert each cut to a
    // vtkKdNode.
    std::vector<vtkSmartPointer<vtkKdNode> > kdnodes;
    for (const auto& bbox : cuts)
    {
      auto kdnode = vtkSmartPointer<vtkKdNode>::New();
      kdnode->SetDim(-1); // leaf.

      double cut_bounds[6];
      bbox.GetBounds(cut_bounds);
      kdnode->SetBounds(cut_bounds);
      kdnodes.push_back(std::move(kdnode));
    }
    vtkSMPThreadLocalObject<vtkGenericCell> gcellLO;
    vtkSMPTools::For(0, numCells,
      [dataset, ghostCells, &kdnodes, &gcellLO, &cellRegions](vtkIdType first, vtkIdType last) {
        auto gcell = gcellLO.Local();
        std::vector<double> weights(dataset->GetMaxCellSize());
        for (vtkIdType cellId = first; cellId < last; ++cellId)
        {
          if (ghostCells != nullptr &&
            (ghostCells->GetTypedComponent(cellId, 0) & vtkDataSetAttributes::DUPLICATECELL) != 0)
          {
            // skip ghost cells, they will not be extracted since they will be
            // extracted on ranks where they are not marked as ghosts.
            continue;
          }
          dataset->GetCell(cellId, gcell);
          double cellBounds[6];
          dataset->GetCellBounds(cellId, cellBounds);
          for (int cutId = 0; cutId < static_cast<int>(kdnodes.size()); ++cutId)
          {
            if (kdnodes[cutId]->IntersectsCell(
                  gcell, /*useDataBounds*/ 0, /*cellRegion*/ -1, cellBounds))
            {
              cellRegions[cellId].push_back(cutId);
            }
          }
        }
      });
  }
  else
  {
    // simply assign to region contain the cell center.
    vtkSMPThreadLocalObject<vtkGenericCell> gcellLO;
    vtkSMPTools::For(0, numCells,
      [dataset, ghostCells, &cuts, &gcellLO, &cellRegions](vtkIdType first, vtkIdType last) {
        auto gcell = gcellLO.Local();
        std::vector<double> weights(dataset->GetMaxCellSize());
        for (vtkIdType cellId = first; cellId < last; ++cellId)
        {
          if (ghostCells != nullptr &&
            (ghostCells->GetTypedComponent(cellId, 0) & vtkDataSetAttributes::DUPLICATECELL) != 0)
          {
            // skip ghost cells, they will not be extracted since they will be
            // extracted on ranks where they are not marked as ghosts.
            continue;
          }
          dataset->GetCell(cellId, gcell);
          double pcenter[3], center[3];
          int subId = gcell->GetParametricCenter(pcenter);
          gcell->EvaluateLocation(subId, pcenter, center, &weights[0]);
          for (int cutId = 0; cutId < static_cast<int>(cuts.size()); ++cutId)
          {
            const auto& bbox = cuts[cutId];
            if (bbox.ContainsPoint(center))
            {
              cellRegions[cellId].push_back(cutId);
              assert(cellRegions[cellId].size() == 1);
              break;
            }
          }
        }
      });
  }

  return cellRegions;
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

}

vtkStandardNewMacro(vtkRedistributeDataSetFilter);
vtkCxxSetObjectMacro(vtkRedistributeDataSetFilter, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkRedistributeDataSetFilter::vtkRedistributeDataSetFilter()
  : Controller(nullptr)
  , BoundaryMode(vtkRedistributeDataSetFilter::ASSIGN_TO_ONE_REGION)
  , NumberOfPartitions(0)
  , PreservePartitionsInOutput(false)
  , GenerateGlobalCellIds(true)
  , UseExplicitCuts(false)
  , ExpandExplicitCuts(true)
  , EnableDebugging(false)
  , ValidDim{ true, true, true }
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkRedistributeDataSetFilter::~vtkRedistributeDataSetFilter()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int vtkRedistributeDataSetFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::SetExplicitCuts(const std::vector<vtkBoundingBox>& boxes)
{
  if (this->ExplicitCuts != boxes)
  {
    this->ExplicitCuts = boxes;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::RemoveAllExplicitCuts()
{
  if (this->ExplicitCuts.size() > 0)
  {
    this->ExplicitCuts.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::AddExplicitCut(const vtkBoundingBox& bbox)
{
  if (bbox.IsValid() &&
    std::find(this->ExplicitCuts.begin(), this->ExplicitCuts.end(), bbox) ==
      this->ExplicitCuts.end())
  {
    this->ExplicitCuts.push_back(bbox);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::AddExplicitCut(const double bounds[6])
{
  vtkBoundingBox bbox(bounds);
  this->AddExplicitCut(bbox);
}

//----------------------------------------------------------------------------
int vtkRedistributeDataSetFilter::GetNumberOfExplicitCuts() const
{
  return static_cast<int>(this->ExplicitCuts.size());
}

//----------------------------------------------------------------------------
const vtkBoundingBox& vtkRedistributeDataSetFilter::GetExplicitCut(int index) const
{
  if (index >= 0 && index < this->GetNumberOfExplicitCuts())
  {
    return this->ExplicitCuts[index];
  }

  static vtkBoundingBox nullbox;
  return nullbox;
}

//----------------------------------------------------------------------------
int vtkRedistributeDataSetFilter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = vtkDataObject::GetData(outputVector, 0);
  auto outInfo = outputVector->GetInformationObject(0);

  if (vtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    if (!vtkMultiBlockDataSet::SafeDownCast(outputDO))
    {
      auto output = vtkMultiBlockDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
  }
  else if (!this->PreservePartitionsInOutput && !vtkUnstructuredGrid::GetData(outputVector, 0))
  {
    auto output = vtkUnstructuredGrid::New();
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    output->FastDelete();
  }
  else if (this->PreservePartitionsInOutput && !vtkPartitionedDataSet::GetData(outputVector, 0))
  {
    auto output = vtkPartitionedDataSet::New();
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    output->FastDelete();
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkRedistributeDataSetFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = vtkDataObject::GetData(outputVector, 0);
  this->MarkValidDimensions(inputDO);

  if (this->UseExplicitCuts && this->ExpandExplicitCuts)
  {
    auto bbox = detail::GetBounds(inputDO);
    if (bbox.IsValid())
    {
      bbox.Inflate(0.1 * bbox.GetDiagonalLength());
    }
    this->Cuts = vtkRedistributeDataSetFilter::ExpandCuts(this->ExplicitCuts, bbox);
  }
  else if (this->UseExplicitCuts)
  {
    this->Cuts = this->ExplicitCuts;
  }
  else
  {
    this->Cuts = this->GenerateCuts(inputDO);
  }
  this->UpdateProgress(0.25);

  if (auto inputMBDS = vtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    this->SetProgressShiftScale(0.25, 0.75);
    auto outputMBDS = vtkMultiBlockDataSet::SafeDownCast(outputDO);
    if (!outputMBDS)
    {
      vtkLogF(ERROR, "output should be a vtkMultiBlockDataSet");
      return 0;
    }
    vtkIdType mb_offset = 0;
    return this->RedistributeMultiBlockDataSet(inputMBDS, outputMBDS, &mb_offset);
  }

  vtkSmartPointer<vtkPartitionedDataSet> parts = vtkPartitionedDataSet::SafeDownCast(outputDO);
  if (!parts)
  {
    parts = vtkSmartPointer<vtkPartitionedDataSet>::New();
  }

  this->SetProgressShiftScale(0.25, 0.5);
  if (!this->Redistribute(inputDO, parts, this->Cuts))
  {
    return 0;
  }

  if (auto outputPDS = vtkPartitionedDataSet::SafeDownCast(outputDO))
  {
    if (!this->EnableDebugging)
    {
      // if output is vtkPartitionedDataSet, let's prune empty partitions. Not
      // necessary, but should help avoid people reading too much into the
      // partitions generated on each rank.
      outputPDS->RemoveNullPartitions();
    }
  }
  else if (auto outputUG = vtkUnstructuredGrid::SafeDownCast(outputDO))
  {
    vtkNew<vtkAppendFilter> appender;
    for (unsigned int cc = 0; cc < parts->GetNumberOfPartitions(); ++cc)
    {
      if (auto ds = parts->GetPartition(cc))
      {
        appender->AddInputDataObject(ds);
      }
    }
    if (appender->GetNumberOfInputConnections(0) > 1)
    {
      appender->Update();
      outputUG->ShallowCopy(appender->GetOutputDataObject(0));
    }
    else if (appender->GetNumberOfInputConnections(0) == 1)
    {
      outputUG->ShallowCopy(appender->GetInputDataObject(0, 0));
    }
    outputUG->GetFieldData()->PassData(inputDO->GetFieldData());
  }
  this->SetProgressShiftScale(0.0, 1.0);
  this->UpdateProgress(1.0);

  return 1;
}

//----------------------------------------------------------------------------
std::vector<vtkBoundingBox> vtkRedistributeDataSetFilter::GenerateCuts(vtkDataObject* dobj)
{
  auto controller = this->GetController();
  const int num_partitions = (controller && this->GetNumberOfPartitions() == 0)
    ? controller->GetNumberOfProcesses()
    : this->GetNumberOfPartitions();
  return vtkDIYKdTreeUtilities::GenerateCuts(
    dobj, std::max(1, num_partitions), /*use_cell_centers=*/true, controller);
}

//----------------------------------------------------------------------------
bool vtkRedistributeDataSetFilter::Redistribute(vtkDataObject* inputDO,
  vtkPartitionedDataSet* outputPDS, const std::vector<vtkBoundingBox>& cuts,
  vtkIdType* mb_offset /*=nullptr*/)
{
  assert(outputPDS != nullptr);
  this->UpdateProgress(0.0);

  if (auto inputPDS = vtkPartitionedDataSet::SafeDownCast(inputDO))
  {
    outputPDS->SetNumberOfPartitions(static_cast<unsigned int>(cuts.size()));

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

    // since number of partitions need not match up across ranks, we do a quick
    // reduction to determine the number of iterations over partitions.
    // we limit to non-empty partitions.
    std::vector<vtkDataSet*> input_partitions;
    for (unsigned int cc = 0; cc < xfmedInput->GetNumberOfPartitions(); ++cc)
    {
      auto ds = xfmedInput->GetPartition(cc);
      if (ds && (ds->GetNumberOfPoints() > 0 || ds->GetNumberOfCells() > 0))
      {
        input_partitions.push_back(ds);
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

    if (input_partitions.size() == 0)
    {
      // all ranks have empty data.
      return true;
    }

    std::vector<vtkSmartPointer<vtkPartitionedDataSet> > results;
    for (auto& ds : input_partitions)
    {
      vtkNew<vtkPartitionedDataSet> curOutput;
      if (this->RedistributeDataSet(ds, curOutput, cuts))
      {
        assert(curOutput->GetNumberOfPartitions() == static_cast<unsigned int>(cuts.size()));
        results.push_back(curOutput);
      }
    }

    // combine leaf nodes an all parts in the results to generate the output.
    for (unsigned int part = 0; part < outputPDS->GetNumberOfPartitions(); ++part)
    {
      vtkNew<vtkAppendFilter> appender;
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
  }
  else if (auto inputDS = vtkDataSet::SafeDownCast(inputDO))
  {
    // assign global cell ids to inputDO, if not preset.
    // we do this assignment before distributing cells if boundary mode is not
    // set to SPLIT_BOUNDARY_CELLS in which case we do after the split.
    vtkSmartPointer<vtkDataSet> xfmedInput;
    if (this->GenerateGlobalCellIds && this->BoundaryMode != SPLIT_BOUNDARY_CELLS)
    {
      xfmedInput = this->AssignGlobalCellIds(inputDS, mb_offset);
    }
    else
    {
      xfmedInput = inputDS;
    }
    if (!this->RedistributeDataSet(xfmedInput, outputPDS, cuts))
    {
      return false;
    }
  }
  this->UpdateProgress(0.5);

  switch (this->GetBoundaryMode())
  {
    case vtkRedistributeDataSetFilter::SPLIT_BOUNDARY_CELLS:
      // by this point, boundary cells have been cloned on all boundary ranks.
      // locally, we will now simply clip each dataset by the corresponding
      // partition bounds.
      for (unsigned int cc = 0, max = outputPDS->GetNumberOfPartitions(); cc < max; ++cc)
      {
        if (auto ds = outputPDS->GetPartition(cc))
        {
          outputPDS->SetPartition(cc, this->ClipDataSet(ds, cuts[cc]));
        }
      }

      if (this->GenerateGlobalCellIds)
      {
        auto result = this->AssignGlobalCellIds(outputPDS, mb_offset);
        outputPDS->ShallowCopy(result);
      }
      break;

    case vtkRedistributeDataSetFilter::ASSIGN_TO_ONE_REGION:
      // nothing to do, since we already assigned cells uniquely when splitting.
      break;

    case vtkRedistributeDataSetFilter::ASSIGN_TO_ALL_INTERSECTING_REGIONS:
      // mark ghost cells using cell ownership information generated in
      // `SplitDataSet`.
      this->MarkGhostCells(outputPDS);
      break;

    default:
      // nothing to do.
      break;
  }
  this->UpdateProgress(0.75);

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
  this->UpdateProgress(1.0);

  return true;
}

//----------------------------------------------------------------------------
int vtkRedistributeDataSetFilter::RedistributeMultiBlockDataSet(
  vtkMultiBlockDataSet* input, vtkMultiBlockDataSet* output, vtkIdType* mb_offset /*=nullptr*/)
{
  if (!input || !output)
  {
    return 0;
  }

  output->CopyStructure(input);
  for (unsigned int block_id = 0; block_id < input->GetNumberOfBlocks(); ++block_id)
  {
    auto in_block = input->GetBlock(block_id);
    auto out_block = output->GetBlock(block_id);
    if (auto in_mbds = vtkMultiBlockDataSet::SafeDownCast(in_block))
    {
      auto out_mbds = vtkMultiBlockDataSet::SafeDownCast(out_block);
      this->RedistributeMultiBlockDataSet(in_mbds, out_mbds, mb_offset);
    }
    else if (auto in_mp = vtkMultiPieceDataSet::SafeDownCast(in_block))
    {
      auto out_mp = vtkMultiPieceDataSet::SafeDownCast(out_block);
      this->RedistributeMultiPieceDataSet(in_mp, out_mp, mb_offset);
    }
    else
    {
      // It's okay for inputDS to be null. Redistribute() should appropriately handle this.
      auto inputDS = vtkDataSet::SafeDownCast(in_block);
      vtkNew<vtkPartitionedDataSet> parts;

      if (!this->Redistribute(inputDS, parts, this->Cuts, mb_offset))
      {
        continue;
      }

      if (this->PreservePartitionsInOutput)
      {
        // have this block remain as a PDS
        output->SetBlock(block_id, parts);
      }
      else
      {
        // convert this block to an UG
        vtkNew<vtkAppendFilter> appender;
        for (unsigned int cc = 0; cc < parts->GetNumberOfPartitions(); ++cc)
        {
          if (auto ds = parts->GetPartition(cc))
          {
            appender->AddInputDataObject(ds);
          }
        }
        if (appender->GetNumberOfInputConnections(0) > 0)
        {
          appender->Update();
          output->SetBlock(block_id, appender->GetOutput(0));
        }
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
// only for vtkMultiPieceDataSets that are part of a vtkMultiBlockDataSet
int vtkRedistributeDataSetFilter::RedistributeMultiPieceDataSet(
  vtkMultiPieceDataSet* input, vtkMultiPieceDataSet* output, vtkIdType* mb_offset /*=nullptr*/)
{
  if (!input || !output)
  {
    return 0;
  }

  output->CopyStructure(input);

  // because different ranks may have different numbers of pieces, we combine
  // into a single unstructured grid before redistributing data
  vtkNew<vtkAppendFilter> input_appender;
  vtkNew<vtkUnstructuredGrid> inputUG;
  for (unsigned int piece_id = 0; piece_id < input->GetNumberOfPieces(); ++piece_id)
  {
    if (auto ds = input->GetPiece(piece_id))
    {
      input_appender->AddInputDataObject(ds);
    }
  }
  if (input_appender->GetNumberOfInputConnections(0) > 0)
  {
    input_appender->Update();
    inputUG->ShallowCopy(input_appender->GetOutput(0));
  }

  vtkNew<vtkPartitionedDataSet> parts;

  if (!this->Redistribute(inputUG, parts, this->Cuts, mb_offset))
  {
    return 0;
  }

  if (this->PreservePartitionsInOutput)
  {
    // NOTE: Can't remove null partitions because it can result in errors
    // due to different ranks having different structures; only relevant
    // when we're working with vtkMultiBlockDataSets
    output->SetNumberOfPieces(parts->GetNumberOfPartitions());
    for (unsigned int piece_id = 0; piece_id < output->GetNumberOfPieces(); ++piece_id)
    {
      output->SetPiece(piece_id, parts->GetPartition(piece_id));
    }
  }
  else
  {
    // convert this block to an UG
    output->SetNumberOfPieces(1);
    vtkNew<vtkAppendFilter> appender;
    for (unsigned int cc = 0; cc < parts->GetNumberOfPartitions(); ++cc)
    {
      if (auto ds = parts->GetPartition(cc))
      {
        appender->AddInputDataObject(ds);
      }
    }
    if (appender->GetNumberOfInputConnections(0) > 0)
    {
      appender->Update();
      output->SetPiece(0, appender->GetOutput(0));
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
bool vtkRedistributeDataSetFilter::RedistributeDataSet(
  vtkDataSet* inputDS, vtkPartitionedDataSet* outputPDS, const std::vector<vtkBoundingBox>& cuts)
{
  // note: inputDS can be null.
  auto parts = this->SplitDataSet(inputDS, cuts);
  assert(parts->GetNumberOfPartitions() == static_cast<unsigned int>(cuts.size()));

  auto pieces = vtkDIYKdTreeUtilities::Exchange(parts, this->GetController());
  assert(pieces->GetNumberOfPartitions() == parts->GetNumberOfPartitions());
  outputPDS->ShallowCopy(pieces);
  return true;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataSet> vtkRedistributeDataSetFilter::ClipDataSet(
  vtkDataSet* dataset, const vtkBoundingBox& bbox)
{
  assert(dataset != nullptr);

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

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPartitionedDataSet> vtkRedistributeDataSetFilter::SplitDataSet(
  vtkDataSet* dataset, const std::vector<vtkBoundingBox>& cuts)
{
  if (!dataset || cuts.size() == 0 || dataset->GetNumberOfCells() == 0)
  //        (dataset->GetNumberOfCells() == 0 && dataset->GetNumberOfPoints() == 0))
  {
    vtkNew<vtkPartitionedDataSet> result;
    result->SetNumberOfPartitions(static_cast<unsigned int>(cuts.size()));
    return result;
  }

  const auto numCells = dataset->GetNumberOfCells();

  // cell_regions tells us for each cell, which regions it belongs to.
  const bool duplicate_cells =
    this->GetBoundaryMode() != vtkRedistributeDataSetFilter::ASSIGN_TO_ONE_REGION;
  auto cell_regions = detail::GenerateCellRegions(dataset, cuts, duplicate_cells);
  assert(static_cast<vtkIdType>(cell_regions.size()) == numCells);

  // cell_ownership value is set to -1 is the cell doesn't belong to any cut
  // else it's set to the index of the cut in the cuts vector.
  vtkSmartPointer<vtkIntArray> cell_ownership;
  if (duplicate_cells)
  {
    // unless duplicating cells along boundary, no need to generate the
    // cell_ownership array. cell_ownership array is used to mark ghost cells
    // later on which don't exist if boundary cells are not duplicated.
    cell_ownership = vtkSmartPointer<vtkIntArray>::New();
    cell_ownership->SetName(CELL_OWNERSHIP_ARRAYNAME);
    cell_ownership->SetNumberOfComponents(1);
    cell_ownership->SetNumberOfTuples(numCells);
    cell_ownership->FillValue(-1);
  }

  // convert cell_regions to a collection of cell-ids for each region so that we
  // can use `vtkExtractCells` to extract cells for each region.
  std::vector<std::vector<vtkIdType> > region_cell_ids(cuts.size());
  vtkSMPTools::For(0, static_cast<int>(cuts.size()),
    [&region_cell_ids, &cell_regions, &numCells, &cell_ownership](int first, int last) {
      for (int cutId = first; cutId < last; ++cutId)
      {
        auto& cell_ids = region_cell_ids[cutId];
        for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
        {
          const auto& cut_ids = cell_regions[cellId];
          auto iter = std::lower_bound(cut_ids.begin(), cut_ids.end(), cutId);
          if (iter != cut_ids.end() && *iter == cutId)
          {
            cell_ids.push_back(cellId);

            if (cell_ownership != nullptr && iter == cut_ids.begin())
            {
              // we treat the first cut number in the cut_ids vector as the
              // owner of the cell. `cell_ownership` array
              // will only be written to by that cut to avoid race condition
              // (note the vtkSMPTools::For()).

              // cell is owned by the numerically smaller cut.
              cell_ownership->SetTypedComponent(cellId, 0, cutId);
            }
          }
        }
      }
    });

  vtkNew<vtkPartitionedDataSet> result;
  result->SetNumberOfPartitions(static_cast<unsigned int>(cuts.size()));

  // we create a clone of the input and add the
  // cell_ownership cell arrays to it so that they are propagated to each of the
  // extracted subsets and exchanged. It will be used later on to mark
  // ghost cells.
  auto clone = vtkSmartPointer<vtkDataSet>::Take(dataset->NewInstance());
  clone->ShallowCopy(dataset);
  clone->GetCellData()->AddArray(cell_ownership);

  vtkNew<vtkExtractCells> extractor;
  extractor->SetInputDataObject(clone);

  for (size_t region_idx = 0; region_idx < region_cell_ids.size(); ++region_idx)
  {
    const auto& cell_ids = region_cell_ids[region_idx];
    if (cell_ids.size() > 0)
    {
      extractor->SetCellIds(&cell_ids[0], static_cast<vtkIdType>(cell_ids.size()));
      extractor->Update();

      vtkNew<vtkUnstructuredGrid> ug;
      ug->ShallowCopy(extractor->GetOutputDataObject(0));
      result->SetPartition(static_cast<unsigned int>(region_idx), ug);
    }
  }
  return result;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
std::vector<vtkBoundingBox> vtkRedistributeDataSetFilter::ExpandCuts(
  const std::vector<vtkBoundingBox>& cuts, const vtkBoundingBox& bounds)
{
  vtkBoundingBox cutsBounds;
  for (const auto& bbox : cuts)
  {
    cutsBounds.AddBox(bbox);
  }

  if (!bounds.IsValid() || !cutsBounds.IsValid() || cutsBounds.Contains(bounds))
  {
    // nothing to do.
    return cuts;
  }

  std::vector<vtkBoundingBox> result = cuts;
  for (auto& bbox : result)
  {
    if (!bbox.IsValid())
    {
      continue;
    }

    double bds[6];
    bbox.GetBounds(bds);
    for (int face = 0; face < 6; ++face)
    {
      if (bds[face] == cutsBounds.GetBound(face))
      {
        bds[face] = (face % 2 == 0) ? std::min(bds[face], bounds.GetBound(face))
                                    : std::max(bds[face], bounds.GetBound(face));
      }
    }
    bbox.SetBounds(bds);
    assert(bbox.IsValid()); // input valid implies output is valid too.
  }

  return result;
}

//----------------------------------------------------------------------------
// Determine which dimensions in the initial bounding box (before any inflation
// of the bounds occurs) has a non-zero length. This is necessary for clipping
// when the BoundaryMode is set to SPLIT_BOUNDARY_CELLS. Otherwise if a dataset
// ends up being 2D, performing plane clips on all sides of the bounding box may
// result in full dataset being clipped away.
void vtkRedistributeDataSetFilter::MarkValidDimensions(vtkDataObject* inputDO)
{
  static const int max_dim = 3;
  auto bbox = detail::GetBounds(inputDO);
  auto comm = vtkDIYUtilities::GetCommunicator(this->Controller);
  vtkDIYUtilities::AllReduce(comm, bbox);

  double len[max_dim];
  bbox.GetLengths(len);
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

//----------------------------------------------------------------------------
void vtkRedistributeDataSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "BoundaryMode: " << this->BoundaryMode << endl;
  os << indent << "NumberOfPartitions: " << this->NumberOfPartitions << endl;
  os << indent << "PreservePartitionsInOutput: " << this->PreservePartitionsInOutput << endl;
  os << indent << "GenerateGlobalCellIds: " << this->GenerateGlobalCellIds << endl;
  os << indent << "UseExplicitCuts: " << this->UseExplicitCuts << endl;
  os << indent << "ExpandExplicitCuts: " << this->ExpandExplicitCuts << endl;
  os << indent << "EnableDebugging: " << this->EnableDebugging << endl;
}
