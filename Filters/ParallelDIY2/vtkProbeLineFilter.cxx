/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeLineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProbeLineFilter.h"

#include "vtkAggregateDataSetFilter.h"
#include "vtkAppendArcLength.h"
#include "vtkAppendDataSets.h"
#include "vtkCell.h"
#include "vtkCellCenters.h"
#include "vtkCellIterator.h"
#include "vtkCellLocatorStrategy.h"
#include "vtkCompositeDataSet.h"
#include "vtkCutter.h"
#include "vtkDIYExplicitAssigner.h"
#include "vtkDIYUtilities.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkFindCellStrategy.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometricLocator.h"
#include "vtkHyperTreeGridLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLineSource.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPHyperTreeGridProbeFilter.h"
#include "vtkPProbeFilter.h"
#include "vtkPlane.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyLineSource.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStaticCellLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStripper.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVectorOperators.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/mpi.hpp)
// clang-format off

vtkStandardNewMacro(vtkProbeLineFilter);

vtkCxxSetObjectMacro(vtkProbeLineFilter, Controller, vtkMultiProcessController);

namespace
{
//==============================================================================
/**
 * Store the information of the intersection between a cell and a ray. InT and OutT
 * are the parametric distances on the ray for the first (and second for 3D cells)
 * intersection between the ray and the cell. CellId is the id of the intersected cell.
 * A value of -1 means that the intersection is happening outside the cell.
 */
struct HitCellInfo
{
  double InT;
  double OutT;
  vtkIdType CellId;

  operator bool() const noexcept
  {
    return this->InT >= 0.0 && this->OutT >= 0.0;
  }

  bool operator<(const HitCellInfo& r) const noexcept
  {
    return this->InT < r.InT;
  }
};

//==============================================================================
/**
 * Return the entry point and exit point of a given cell for the segment [p1,p2].
 */
HitCellInfo GetInOutCell(const vtkVector3d& p1, const vtkVector3d& p2, vtkCell* cell, double tolerance)
{
  double t, x[3], dummy3[3];
  int dummy;
  HitCellInfo res{-1.0, -1.0, -1};

  if (cell->IntersectWithLine(p1.GetData(), p2.GetData(), tolerance, t, x, dummy3, dummy))
  {
    res.InT = t;
  }
  if (cell->IntersectWithLine(p2.GetData(), p1.GetData(), tolerance, t, x, dummy3, dummy))
  {
    res.OutT = 1.0 - t;
  }

  return res;
}

//==============================================================================
/**
 * Return the intersection of a point p1 with a cell in an input dataset (and its locator).
 * Also return the intersection from this point to the closest surface in the direction
 * of p2.
 */
HitCellInfo ProcessLimitPoint(vtkVector3d p1, vtkVector3d p2, int pattern, vtkDataSet* input, vtkAbstractCellLocator* locator, double tolerance)
{
  const double norm = (p2 - p1).Norm();
  HitCellInfo result{0.0, -1.0, -1};

  // We offset a bit P1 only for finding its corresponding cell so there is no
  // ambiguity in case of consecutive 3D cells
  vtkVector3d findCellLocation = p1 +  (p2 - p1) * (tolerance / norm);
  vtkIdType cellId = locator->FindCell(findCellLocation.GetData());
  if (cellId >= 0)
  {
    vtkCell* cell = input->GetCell(cellId);
    result.CellId = cellId;
    double outT;
    double tmp[3];
    double tmp2[3];
    int tmpi;
    cell->IntersectWithLine(p2.GetData(), p1.GetData(), tolerance, outT, tmp, tmp2, tmpi);
    result.OutT = std::max(0.0, 1.0 - outT - tolerance / norm);
  }
  else if (pattern == vtkProbeLineFilter::SAMPLE_LINE_AT_CELL_BOUNDARIES)
  {
    double t, x[3], pcoords[3];
    int id;
    if (locator->IntersectWithLine(p1.GetData(), p2.GetData(), tolerance, t, x, pcoords, id))
    {
      result.OutT = t - tolerance / norm;
    }
  }

  return result;
}

//==============================================================================
/**
 * Return the intersection of a point p1 with a cell in a HyperTreeGrid (inside its locator).
 * Also return the intersection from this point to the closest surface in the direction
 * of p2.
 */
HitCellInfo ProcessLimitPoint(vtkVector3d p1, vtkVector3d p2, int pattern, vtkHyperTreeGridLocator* locator, double tolerance)
{
  const double norm = (p2 - p1).Norm();
  HitCellInfo result{0.0, -1.0, -1};

  // We offset a bit P1 only for finding its corresponding cell so there is no
  // ambiguity in case of consecutive 3D cells
  vtkVector3d findCellLocation = p1 +  (p2 - p1) * (tolerance / norm);
  vtkNew<vtkGenericCell> cell;
  int subId = 0;
  double pcoords[3] = {0.0, 0.0, 0.0};
  std::vector<double> weights(std::pow(2, locator->GetHTG()->GetDimension()), 0.0);
  vtkIdType cellId = locator->FindCell(findCellLocation.GetData(), tolerance, cell, subId, pcoords, weights.data());
  if (cellId >= 0)
  {
    result.CellId = cellId;
    double outT;
    double tmp[3];
    double tmp2[3];
    int tmpi;
    cell->IntersectWithLine(p2.GetData(), p1.GetData(), tolerance, outT, tmp, tmp2, tmpi);
    result.OutT = std::max(0.0, 1.0 - outT - tolerance / norm);
  }
  else if (pattern == vtkProbeLineFilter::SAMPLE_LINE_AT_CELL_BOUNDARIES)
  {
    double t, x[3];
    int id;
    if (locator->IntersectWithLine(p1.GetData(), p2.GetData(), tolerance, t, x, pcoords, id, cellId, cell))
    {
      result.OutT = t - tolerance / norm;
    }
  }

  return result;
}

//==============================================================================
/**
 * Workers to project back intersections from their parametric representation to
 * actual 3D coordinates.
 */
struct PointProjectionBordersWorker
{
  PointProjectionBordersWorker(const vtkVector3d& p1, const vtkVector3d& p2, const std::vector<HitCellInfo>& inter, vtkPoints* result)
    : P1(p1)
    , V12(p2 - p1)
    , Intersections(inter)
    , Result(result)
  {}

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    vtkIdType idx = 2 + 2 * startId;
    for (vtkIdType i = startId; i < endId; ++i)
    {
      vtkVector3d point = this->P1 + this->Intersections[i].InT * this->V12;
      this->Result->SetPoint(idx, point.GetData());
      ++idx;
      point = this->P1 + this->Intersections[i].OutT * this->V12;
      this->Result->SetPoint(idx, point.GetData());
      ++idx;
    }
  }

  const vtkVector3d P1;
  const vtkVector3d V12;
  const std::vector<HitCellInfo>& Intersections;
  vtkPoints* Result;
};

struct PointProjectionCentersWorker
{
  PointProjectionCentersWorker(const vtkVector3d& p1, const vtkVector3d& p2, const std::vector<HitCellInfo>& inter, vtkPoints* result)
    : P1(p1)
    , V12(p2 - p1)
    , Intersections(inter)
    , Result(result)
  {}

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    for (vtkIdType i = startId; i < endId; ++i)
    {
      vtkVector3d point = this->P1 + (this->Intersections[i].InT + this->Intersections[i].OutT) * 0.5 * this->V12;
      this->Result->SetPoint(i + 1, point.GetData());
    }
  }

  const vtkVector3d P1;
  const vtkVector3d V12;
  const std::vector<HitCellInfo>& Intersections;
  vtkPoints* Result;
};

}

//------------------------------------------------------------------------------
struct vtkProbeLineFilter::vtkInternals
{
  vtkMTimeType PreviousInputTime = 0;
  std::map<vtkDataSet*,vtkSmartPointer<vtkFindCellStrategy>> Strategies;
  vtkSmartPointer<vtkHyperTreeGridLocator> HyperTreeGridLocator;

  void UpdateLocators(vtkDataObject* input, int pattern, const double tolerance)
  {
    vtkMTimeType inputTime = input->GetMTime();
    bool isInputDifferent = inputTime != this->PreviousInputTime;
    bool needLocators =
      pattern == vtkProbeLineFilter::SAMPLE_LINE_AT_CELL_BOUNDARIES
      || pattern == vtkProbeLineFilter::SAMPLE_LINE_AT_SEGMENT_CENTERS;
    if (isInputDifferent && needLocators)
    {
      this->PreviousInputTime = inputTime;

      const auto& inputs = vtkCompositeDataSet::GetDataSets(input);
      for (vtkDataSet* ds : inputs)
      {
        if (!ds || !ds->GetNumberOfCells())
        {
          continue;
        }

        vtkNew<vtkStaticCellLocator> locator;
        locator->SetDataSet(ds);
        locator->SetTolerance(tolerance);
        locator->BuildLocator();

        vtkCellLocatorStrategy* strategy = vtkCellLocatorStrategy::New();
        strategy->SetCellLocator(locator);

        this->Strategies[ds] = vtkSmartPointer<vtkFindCellStrategy>::Take(static_cast<vtkFindCellStrategy*>(strategy));
      }
    }
  }
  vtkSmartPointer<vtkPolyData> CheckPointsCloseReturnLine(const vtkVector3d& p1, const vtkVector3d& p2) const
  {
    if (vtkMathUtilities::NearlyEqual(p1[0], p2[0]) &&
        vtkMathUtilities::NearlyEqual(p1[1], p2[1]) &&
        vtkMathUtilities::NearlyEqual(p1[2], p2[2]))
    {
      // In this instance, we probe only Point1 and Point2.
      vtkNew<vtkLineSource> line;
      line->SetPoint1(p1.GetData());
      line->SetPoint2(p2.GetData());
      line->Update();
      return vtkPolyData::SafeDownCast(line->GetOutputDataObject(0));
    }
    return nullptr;
  }

  vtkSmartPointer<vtkPolyData> DistributeIntersectionsAndGenerateLines(vtkMultiProcessController * controller, int samplingPattern, const vtkVector3d& p1, const vtkVector3d& p2, std::vector<HitCellInfo>& intersections, HitCellInfo & p1Hit, HitCellInfo & p2Hit) const
  {
    // Sort our array of projections so the merge across ranks is faster afterwards.
    // Also add intersection information for the beginning and end of the array so it is
    // easier to process when we gather data from all ranks.
    vtkSMPTools::Sort(intersections.begin(), intersections.end());
    intersections.emplace_back(p1Hit);
    intersections.emplace_back(p2Hit);

    // We need to gather points from every ranks to every ranks because vtkProbeFilter
    // assumes that its input is replicated in every ranks.
    using PointSetBlock = std::vector<std::vector<HitCellInfo>>;
    diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(controller);
    diy::Master master(
        comm, 1, -1, []() { return static_cast<void*>(new PointSetBlock()); },
        [](void* b) -> void { delete static_cast<PointSetBlock*>(b); });
    vtkDIYExplicitAssigner assigner(comm, 1);
    diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
        /*dim*/ 1, diy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
    decomposer.decompose(comm.rank(), assigner, master);

    diy::all_to_all(
        master, assigner, [&intersections](PointSetBlock* block, const diy::ReduceProxy& srp)
        {
          int myBlockId = srp.gid();
          if (srp.round() == 0)
          {
            for (int i = 0; i < srp.out_link().size(); ++i)
            {
              const diy::BlockID& blockId = srp.out_link().target(i);
              if (blockId.gid != myBlockId)
              {
                srp.enqueue(blockId, intersections);
              }
            }
          }
          else
          {
            for (int i = 0; i < static_cast<int>(srp.in_link().size()); ++i)
            {
              const diy::BlockID& blockId = srp.in_link().target(i);
              if (blockId.gid != myBlockId)
              {
                std::vector<HitCellInfo> data;
                srp.dequeue(blockId, data);
                block->emplace_back(std::move(data));
              }
            }
          }
        });

    auto ReduceLimitPointHit = [&p1Hit, &p2Hit](std::vector<HitCellInfo>& inter)
    {
      HitCellInfo p2InterHit = inter.back();
      inter.pop_back();
      HitCellInfo p1InterHit = inter.back();
      inter.pop_back();

      if (p1InterHit.OutT < p1Hit.OutT)
      {
        p1Hit = p1InterHit;
      }
      if (p2InterHit.InT > p2Hit.InT)
      {
        p2Hit = p2InterHit;
      }
    };

    ReduceLimitPointHit(intersections);

    // Merge local intersections with intersections from all other ranks
    PointSetBlock* block = master.block<PointSetBlock>(0);
    for (auto& distIntersections : *block)
    {
      ReduceLimitPointHit(distIntersections);
      const size_t numIntersections = intersections.size();
      intersections.insert(intersections.end(), distIntersections.begin(), distIntersections.end());
      auto prevEnd = intersections.begin() + numIntersections;
      std::inplace_merge(intersections.begin(), prevEnd, intersections.end());
    }

    // Tranform back the cells hit informations to 3D coordinates
    vtkNew<vtkPoints> coordinates;
    if (samplingPattern == SAMPLE_LINE_AT_CELL_BOUNDARIES)
    {
      const vtkVector3d v12 = p2 - p1;
      if (intersections.empty())
      {
        coordinates->InsertNextPoint(p1.GetData());
        if (p1Hit.CellId != p2Hit.CellId)
        {
          vtkVector3d point = p1 + p1Hit.OutT * v12;
          coordinates->InsertNextPoint(point.GetData());
          point = p1 + p2Hit.InT * v12;
          coordinates->InsertNextPoint(point.GetData());
        }
        coordinates->InsertNextPoint(p2.GetData());
      }
      else
      {
        vtkIdType numberOfPoints = intersections.size() * 2 + 4;
        coordinates->SetNumberOfPoints(numberOfPoints);

        vtkVector3d point = p1 + p1Hit.OutT * v12;
        coordinates->SetPoint(0, p1.GetData());
        coordinates->SetPoint(1, point.GetData());

        ::PointProjectionBordersWorker worker(p1, p2, intersections, coordinates);
        vtkSMPTools::For(0, intersections.size(), worker);

        point = p1 + p2Hit.InT * v12;
        coordinates->SetPoint(numberOfPoints - 2, point.GetData());
        coordinates->SetPoint(numberOfPoints - 1, p2.GetData());
      }
    }
    else // SamplingPattern == SAMPLE_LINE_AT_SEGMENT_CENTERS
    {
      coordinates->SetNumberOfPoints(intersections.size() + 2);
      coordinates->SetPoint(0, p1.GetData());
      if (!intersections.empty())
      {
        ::PointProjectionCentersWorker worker(p1, p2, intersections, coordinates);
        vtkSMPTools::For(0, intersections.size(), worker);
      }
      coordinates->SetPoint(intersections.size() + 1, p2.GetData());
    }

    vtkNew<vtkPolyLineSource> polyLine;
    polyLine->SetPoints(coordinates);
    polyLine->Update();

    return vtkPolyData::SafeDownCast(polyLine->GetOutputDataObject(0));

  }

};

//------------------------------------------------------------------------------
vtkProbeLineFilter::vtkProbeLineFilter()
  : Internal(new vtkInternals)
{
  this->SetNumberOfInputPorts(2);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkProbeLineFilter::~vtkProbeLineFilter()
{
  this->SetController(nullptr);
  delete this->Internal;
}

//------------------------------------------------------------------------------
int vtkProbeLineFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Check inputs / outputs
  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* samplerInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo || !inputInfo || !samplerInfo)
  {
    vtkErrorMacro("Missing input or output information");
    return 0;
  }

  vtkDataObject* input = inputInfo->Get(vtkDataObject::DATA_OBJECT());
  auto* samplerLocal = vtkPointSet::SafeDownCast(samplerInfo->Get(vtkDataObject::DATA_OBJECT()));
  auto* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  bool outputIsValid = this->AggregateAsPolyData
    ? vtkPolyData::SafeDownCast(output) != nullptr
    : vtkMultiBlockDataSet::SafeDownCast(output) != nullptr;
  if (!output || !input || !samplerLocal || !outputIsValid)
  {
    vtkErrorMacro("Missing input or output");
    return 0;
  }

  bool inputIsHTG = (vtkHyperTreeGrid::SafeDownCast(input) != nullptr);

  bool computeTolerance = this->ComputeTolerance;

  // The probe locations source need to be the same on all ranks : always take rank 0 source
  auto sampler = vtkSmartPointer<vtkPointSet>::Take(samplerLocal->NewInstance());
  if (this->Controller->GetLocalProcessId() == 0)
  {
    this->Controller->Broadcast(samplerLocal, 0);
    sampler->ShallowCopy(samplerLocal);
  }
  else
  {
    this->Controller->Broadcast(sampler, 0);
  }

  // Compute tolerance
  double tolerance = this->Tolerance;
  if (this->ComputeTolerance)
  {
    double bounds[6] = {0, 0, 0, 0, 0, 0};
    if (auto cds = vtkCompositeDataSet::SafeDownCast(input))
    {
      cds->GetBounds(bounds);
    }
    else if (auto ds = vtkDataSet::SafeDownCast(input))
    {
      ds->GetBounds(bounds);
    }
    vtkBoundingBox bb(bounds);
    if (!bb.IsValid())
    {
      // There is no geometry in the dataset : this can happen
      // if the input is not distributed on all MPI ranks
      tolerance = 0.0;
    }
    else
    {
      tolerance = VTK_TOL * bb.GetDiagonalLength();
    }
  }
  if(!inputIsHTG)
  {
    this->Internal->UpdateLocators(input, this->SamplingPattern, tolerance);
  }
  else
  {
    vtkNew<vtkHyperTreeGridGeometricLocator> htgLocator;
    htgLocator->SetHTG(vtkHyperTreeGrid::SafeDownCast(input));
    this->Internal->HyperTreeGridLocator = htgLocator;
  }

  // For each cell create a polyline to probe with
  auto samplerCellsIt = vtkSmartPointer<vtkCellIterator>::Take(sampler->NewCellIterator());
  vtkNew<vtkMultiBlockDataSet> multiBlockOutput;
  for (samplerCellsIt->InitTraversal(); !samplerCellsIt->IsDoneWithTraversal(); samplerCellsIt->GoToNextCell())
  {
    if (samplerCellsIt->GetCellType() == VTK_LINE || samplerCellsIt->GetCellType() == VTK_POLY_LINE)
    {
      auto polyline = this->CreateSamplingPolyLine(sampler->GetPoints(), samplerCellsIt->GetPointIds(), input, tolerance);
      if (!polyline)
      {
        continue;
      }

      switch (this->SamplingPattern)
      {
        case SAMPLE_LINE_AT_CELL_BOUNDARIES:
        case SAMPLE_LINE_AT_SEGMENT_CENTERS:
          // We already shift samples so they lie strictly inside cells. We do not need
          // to use any tolerance, which could actually probe the wrong cells if
          // vtkPProbeFilter has a looser tolerance definition than us.
          tolerance = 0;
          computeTolerance = false;
          break;
        default:
          break;
      }

      vtkSmartPointer<vtkDataSetAlgorithm> prober;
      if(!inputIsHTG){
        vtkNew<vtkPProbeFilter> dsProber;
        dsProber->SetController(this->Controller);
        dsProber->SetPassPartialArrays(this->PassPartialArrays);
        dsProber->SetPassCellArrays(this->PassCellArrays);
        dsProber->SetPassPointArrays(this->PassPointArrays);
        dsProber->SetPassFieldArrays(this->PassFieldArrays);
        dsProber->SetComputeTolerance(computeTolerance);
        dsProber->SetTolerance(tolerance);
        dsProber->SetSourceData(input);
        dsProber->SetFindCellStrategyMap(this->Internal->Strategies);
        dsProber->SetInputData(polyline);
        dsProber->Update();
        prober = dsProber;
      }
      else
      {
        vtkNew<vtkPHyperTreeGridProbeFilter> htgProber;
        htgProber->SetController(this->Controller);
        htgProber->SetPassCellArrays(this->PassCellArrays);
        htgProber->SetPassPointArrays(this->PassPointArrays);
        htgProber->SetPassFieldArrays(this->PassFieldArrays);
        htgProber->SetSourceData(vtkHyperTreeGrid::SafeDownCast(input));
        htgProber->SetLocator(this->Internal->HyperTreeGridLocator);
        htgProber->SetInputData(polyline);
        htgProber->Update();
        prober = htgProber;
      }

      if (this->Controller->GetLocalProcessId() == 0 &&
        this->SamplingPattern == SAMPLE_LINE_AT_CELL_BOUNDARIES && !inputIsHTG)
      {
        // We move points to the cell interfaces.
        // They were artificially moved away from the cell interfaces so probing works well.
        // XXX: this actually assume that every cells is next to each other i.e. this is only
        // valid for 3D ImageData/RectilinearGrid/StructuredGrid.
        vtkPointSet* pointSet = vtkPointSet::SafeDownCast(prober->GetOutputDataObject(0));
        auto pointsRange = vtk::DataArrayTupleRange<3>(pointSet->GetPoints()->GetData());
        using PointRef = decltype(pointsRange)::TupleReferenceType;
        for (vtkIdType pointId = 1; pointId < pointsRange.size() - 1; pointId += 2)
        {
          PointRef p1 = pointsRange[pointId];
          PointRef p2 = pointsRange[pointId + 1];

          p1[0] = p2[0] = 0.5 * (p1[0] + p2[0]);
          p1[1] = p2[1] = 0.5 * (p1[1] + p2[1]);
          p1[2] = p2[2] = 0.5 * (p1[2] + p2[2]);
        }
      }

      vtkNew<vtkAppendArcLength> arcs;
      arcs->SetInputConnection(prober->GetOutputPort());
      arcs->Update();

      const unsigned int block = multiBlockOutput->GetNumberOfBlocks();
      multiBlockOutput->SetNumberOfBlocks(block + 1);
      multiBlockOutput->SetBlock(block, arcs->GetOutputDataObject(0));
    }
    else
    {
      vtkWarningMacro("Found non Line/PolyLine cell in the prober source at: " << samplerCellsIt->GetCellId());
    }
  } // end for each cell

  if (this->AggregateAsPolyData)
  {
    vtkNew<vtkAppendDataSets> appender;
    appender->SetMergePoints(false);
    appender->SetOutputDataSetType(VTK_POLY_DATA);
    for (unsigned int i = 0; i < multiBlockOutput->GetNumberOfBlocks(); ++i)
    {
      appender->AddInputData(multiBlockOutput->GetBlock(i));
    }
    appender->Update();
    output->ShallowCopy(appender->GetOutputDataObject(0));
  }
  else
  {
    output->ShallowCopy(multiBlockOutput);
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkProbeLineFilter::CreateSamplingPolyLine(
  vtkPoints* points, vtkIdList* pointIds, vtkDataObject* input, double tolerance) const
{
  vtkNew<vtkPoints> resPoints;
  vtkNew<vtkIdList> resPointIds;
  for (vtkIdType i = 0; i < pointIds->GetNumberOfIds() - 1; ++i)
  {
    const vtkVector3d p1(points->GetPoint(pointIds->GetId(i)));
    const vtkVector3d p2(points->GetPoint(pointIds->GetId(i + 1)));
    vtkSmartPointer<vtkPolyData> tmp;
    switch (this->SamplingPattern)
    {
      case SAMPLE_LINE_AT_CELL_BOUNDARIES:
      case SAMPLE_LINE_AT_SEGMENT_CENTERS:
        tmp = this->SampleLineAtEachCell(p1, p2, input, tolerance);
        break;
      case SAMPLE_LINE_UNIFORMLY:
        tmp = this->SampleLineUniformly(p1, p2);
        break;
      default:
        vtkErrorMacro("Sampling heuristic wrongly set, abort filter");
        return nullptr;
    }

    vtkPoints* tmpPoints = tmp->GetPoints();
    vtkIdList* tmpPointIds = tmp->GetCell(0)->PointIds;
    // We should have a single cell containing all points
    // assert for future development insurance
    assert(tmpPoints->GetNumberOfPoints() == tmpPointIds->GetNumberOfIds());

    // If the pattern is not SAMPLE_LINE_AT_CELL_BOUNDARIES and we already have some
    // generated probe location, we don't want to duplicate the previous last point with
    // the current first point, which are at the same position.
    vtkIdType oldNumberOfElmts = resPoints->GetNumberOfPoints();
    vtkIdType newNumberOfElmts = oldNumberOfElmts + tmpPoints->GetNumberOfPoints();
    vtkIdType offset = 0;
    if (this->SamplingPattern != SAMPLE_LINE_AT_CELL_BOUNDARIES && oldNumberOfElmts != 0)
    {
      newNumberOfElmts -= 1;
      offset = 1;
    }

    // Merge new points
    if (!resPoints->Resize(newNumberOfElmts))
    {
      vtkErrorMacro("Error during allocation, abort filter");
      return nullptr;
    }
    resPoints->SetNumberOfPoints(newNumberOfElmts);
    for (vtkIdType p = offset; p < tmpPoints->GetNumberOfPoints(); ++p)
    {
      resPoints->SetPoint(p + oldNumberOfElmts - offset, tmpPoints->GetPoint(p));
    }

    // Merge point ids
    if (!resPointIds->Resize(newNumberOfElmts))
    {
      vtkErrorMacro("Error during allocation, abort filter");
      return nullptr;
    }
    resPointIds->SetNumberOfIds(newNumberOfElmts);
    for (vtkIdType p = offset; p < tmpPointIds->GetNumberOfIds(); ++p)
    {
      resPointIds->SetId(p + oldNumberOfElmts - offset, tmpPointIds->GetId(p) + oldNumberOfElmts - offset);
    }
  }

  if (resPoints->GetNumberOfPoints() == 0 || resPointIds->GetNumberOfIds() == 0)
  {
    return nullptr;
  }

  auto polyline = vtkSmartPointer<vtkPolyData>::New();
  polyline->SetPoints(resPoints);
  if (resPointIds->GetNumberOfIds() > 0)
  {
    vtkNew<vtkCellArray> cell;
    cell->InsertNextCell(resPointIds);
    polyline->SetLines(cell);
  }

  return polyline;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkProbeLineFilter::SampleLineUniformly(const vtkVector3d& p1, const vtkVector3d& p2) const
{
  vtkNew<vtkLineSource> lineSource;
  lineSource->SetPoint1(p1.GetData());
  lineSource->SetPoint2(p2.GetData());
  lineSource->SetResolution(this->LineResolution);
  lineSource->Update();
  return vtkPolyData::SafeDownCast(lineSource->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkProbeLineFilter::SampleLineAtEachCell(const vtkVector3d& p1, const vtkVector3d& p2,
  vtkDataObject * input, const double tolerance) const
{
  vtkHyperTreeGrid * htgInput = vtkHyperTreeGrid::SafeDownCast(input);
  bool inputIsHTG = (htgInput != nullptr);
  if(!inputIsHTG)
  {
    return this->SampleLineAtEachCell(p1, p2, vtkCompositeDataSet::GetDataSets(input), tolerance);
  }
  else
  {
    return this->SampleLineAtEachCell(p1, p2, htgInput, tolerance);
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkProbeLineFilter::SampleLineAtEachCell(const vtkVector3d& p1, const vtkVector3d& p2,
  const std::vector<vtkDataSet*>& inputs, const double tolerance) const
{
  {
    vtkSmartPointer<vtkPolyData> line = this->Internal->CheckPointsCloseReturnLine(p1, p2);
    if(line)
    {
      return line;
    }
  }

  vtkVector3d v12Epsilon = p2 - p1;
  const double v12NormEpsilon = tolerance / v12Epsilon.Normalize();
  v12Epsilon = v12Epsilon * tolerance;
  HitCellInfo p1Hit{0.0, 1.0, -1};
  HitCellInfo p2Hit{0.0, 1.0, -1};
  std::vector<HitCellInfo> intersections;

  // Add every intersection with all blocks of the dataset on our current rank.
  // First loop on all block of the input
  for (std::size_t dsId = 0; dsId < inputs.size(); ++dsId)
  {
    vtkDataSet* input = inputs[dsId];

    if (!input || !input->GetNumberOfCells())
    {
      continue;
    }

    auto* strategy = vtkCellLocatorStrategy::SafeDownCast(this->Internal->Strategies.at(input));
    assert(strategy);
    vtkAbstractCellLocator* locator = strategy->GetCellLocator();

    vtkNew<vtkIdList> intersectedIds;
    locator->FindCellsAlongLine(p1.GetData(), p2.GetData(), 0.0, intersectedIds);

    // We process p1 and p2 a bit differently so in the case of their intersection with a cell
    // they are not duplicated
    auto AddLimitPointToIntersections = [&](const vtkVector3d& start, const vtkVector3d& end, bool inverse, HitCellInfo& hit)
    {
      auto processed = ::ProcessLimitPoint(start, end, this->SamplingPattern, input, locator, tolerance);

      if (processed.OutT >= 0.0)
      {
        if (inverse)
        {
          processed.InT = 1.0 - processed.OutT;
          processed.OutT = 1.0; // We should subtract by processed.InT here but we don't because it is 0.0
        }

        bool shouldReplace = inverse ? (hit.InT < processed.InT) : (hit.OutT > processed.OutT);
        if (shouldReplace)
        {
          hit = processed;
        }

        if (processed.CellId >= 0.0)
        {
          intersectedIds->DeleteId(processed.CellId);
        }
      }
    };
    AddLimitPointToIntersections(p1, p2, false, p1Hit);
    AddLimitPointToIntersections(p2, p1, true, p2Hit);

    // Process every cell intersection once we're done with limit points
    for (vtkIdType i = 0; i < intersectedIds->GetNumberOfIds(); ++i)
    {
      vtkIdType cellId = intersectedIds->GetId(i);
      if (input->HasAnyGhostCells() && input->GetCellGhostArray()->GetValue(cellId))
      {
        continue;
      }

      vtkCell* cell = input->GetCell(cellId);
      auto inOut = ::GetInOutCell(p1, p2, cell, tolerance);
      if (!inOut)
      {
        continue;
      }

      // Add intersected cell
      inOut.CellId = cellId;
      if (cell->GetCellDimension() == 3)
      {
        if (vtkMathUtilities::NearlyEqual(inOut.InT, inOut.OutT, tolerance))
        {
          continue;
        }
        inOut.InT += v12NormEpsilon;
        inOut.OutT -= v12NormEpsilon;
      }

      intersections.emplace_back(inOut);
    }
  }

  return this->Internal->DistributeIntersectionsAndGenerateLines(this->Controller, this->SamplingPattern, p1, p2, intersections, p1Hit, p2Hit);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkProbeLineFilter::SampleLineAtEachCell(const vtkVector3d& p1, const vtkVector3d& p2,
  vtkHyperTreeGrid * input, const double tolerance) const
{
  {
    vtkSmartPointer<vtkPolyData> line = this->Internal->CheckPointsCloseReturnLine(p1, p2);
    if(line)
    {
      return line;
    }
  }

  vtkVector3d v12Epsilon = p2 - p1;
  const double v12NormEpsilon = tolerance / v12Epsilon.Normalize();
  v12Epsilon = v12Epsilon * tolerance;
  HitCellInfo p1Hit{0.0, 1.0, -1};
  HitCellInfo p2Hit{0.0, 1.0, -1};

  vtkNew<vtkGenericCell> cell;
  vtkNew<vtkIdList> intersectedIds;
  vtkNew<vtkPoints> pointsFound;
  {
    vtkNew<vtkIdList> forward;
    vtkNew<vtkIdList> backward;
    vtkNew<vtkPoints> fPoints;
    vtkNew<vtkPoints> bPoints;
    this->Internal->HyperTreeGridLocator->IntersectWithLine(p1.GetData(), p2.GetData(), 0.0, fPoints, forward, cell);
    this->Internal->HyperTreeGridLocator->IntersectWithLine(p2.GetData(), p1.GetData(), 0.0, bPoints, backward, cell);
    intersectedIds->SetNumberOfIds(forward->GetNumberOfIds() + backward->GetNumberOfIds());
    std::copy(forward->begin(), forward->end(), intersectedIds->begin());
    std::copy(backward->begin(), backward->end(), intersectedIds->begin() + forward->GetNumberOfIds());
    pointsFound->SetNumberOfPoints(fPoints->GetNumberOfPoints() + bPoints->GetNumberOfPoints());
    pointsFound->InsertPoints(0, fPoints->GetNumberOfPoints(), 0, fPoints);
    pointsFound->InsertPoints(fPoints->GetNumberOfPoints(), bPoints->GetNumberOfPoints(), 0, bPoints);
  }

  auto AddLimitPointToIntersections = [&](const vtkVector3d& start, const vtkVector3d& end, bool inverse, HitCellInfo& hit)
  {
    auto processed = ::ProcessLimitPoint(start, end, this->SamplingPattern, this->Internal->HyperTreeGridLocator, tolerance);

    if (processed.OutT >= 0.0)
    {
      if (inverse)
      {
        processed.InT = 1.0 - processed.OutT;
        processed.OutT = 1.0; //  We should subtract by processed.InT here but we don't because it is 0.0
      }

      bool shouldReplace = inverse ? (hit.InT < processed.InT) : (hit.OutT > processed.OutT);
      if (shouldReplace)
      {
        hit = processed;
      }

      if (processed.CellId >= 0.0)
      {
        intersectedIds->DeleteId(processed.CellId);
      }
    }
  };
  AddLimitPointToIntersections(p1, p2, false, p1Hit);
  AddLimitPointToIntersections(p2, p1, true, p2Hit);

  std::vector<HitCellInfo> intersections(intersectedIds->GetNumberOfIds()/2);
  {
    std::map<vtkIdType, HitCellInfo> intersectionMap;
    std::vector<double> ptBuffer(3, 0.0);
    double norm = (p2 - p1).Norm();
    for (vtkIdType i = 0; i < intersectedIds->GetNumberOfIds(); ++i)
    {
      vtkIdType cellId = intersectedIds->GetId(i);
      if (input->HasAnyGhostCells() && input->GetGhostCells()->GetValue(cellId))
      {
        continue;
      }

      // Add intersected cell
      intersectionMap[cellId].CellId = cellId;
      pointsFound->GetPoint(i, ptBuffer.data());
      vtkMath::Subtract(ptBuffer.data(), p1.GetData(), ptBuffer.data());
      double thisT = vtkMath::Norm(ptBuffer.data()) / norm;
      if(i < static_cast<vtkIdType>(intersections.size()))
      {
        intersectionMap[cellId].InT = thisT;
      }
      else
      {
        intersectionMap[cellId].OutT = thisT;
      }
    }
    int counter = 0;
    for(auto it = intersectionMap.begin(); it != intersectionMap.end(); it++)
    {
      intersections[counter] = it->second;
      counter++;
    }
  }
  if (input->GetDimension() == 3)
    for(auto it = intersections.begin(); it != intersections.end(); it++)
    {

      {
        if (vtkMathUtilities::NearlyEqual(it->InT, it->OutT, tolerance))
        {
          continue;
        }
        it->InT += v12NormEpsilon;
        it->OutT -= v12NormEpsilon;
      }
    }
  return this->Internal->DistributeIntersectionsAndGenerateLines(this->Controller, this->SamplingPattern, p1, p2, intersections, p1Hit, p2Hit);
}

//------------------------------------------------------------------------------
int vtkProbeLineFilter::FillInputPortInformation(int port, vtkInformation* info)
{

  switch (port)
  {
  case 0:
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
    break;

  case 1:
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
    break;

  default:
    break;
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkProbeLineFilter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (this->AggregateAsPolyData)
  {
    vtkPolyData* output = vtkPolyData::GetData(outInfo);
    if (!output)
    {
      auto newOutput = vtkSmartPointer<vtkPolyData>::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    }
  }
  else
  {
    vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outInfo);
    if (!output)
    {
      auto newOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkProbeLineFilter::SetSourceConnection(vtkAlgorithmOutput* input)
{
  this->SetInputConnection(1, input);
}

//------------------------------------------------------------------------------
void vtkProbeLineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  switch (this->SamplingPattern)
  {
    case SAMPLE_LINE_AT_CELL_BOUNDARIES:
      os << indent << "SamplingPattern: SAMPLE_LINE_AT_CELL_BOUNDARIES" << endl;
      break;
    case SAMPLE_LINE_AT_SEGMENT_CENTERS:
      os << indent << "SamplingPattern: SAMPLE_LINE_AT_SEGMENT_CENTERS" << endl;
      break;
    case SAMPLE_LINE_UNIFORMLY:
      os << indent << "SamplingPattern: SAMPLE_LINE_UNIFORMLY" << endl;
      break;
    default:
      os << indent << "SamplingPattern: UNDEFINED" << endl;
      break;
  }
  os << indent << "LineResolution: " << this->LineResolution << endl;
  os << indent << "AggregateAsPolyData: " << this->AggregateAsPolyData << endl;
  os << indent << "PassPartialArrays: " << this->PassPartialArrays << endl;
  os << indent << "PassCellArrays: " << this->PassCellArrays << endl;
  os << indent << "PassPointArrays: " << this->PassPointArrays << endl;
  os << indent << "PassFieldArrays: " << this->PassFieldArrays << endl;
  os << indent << "ComputeTolerance: " << this->ComputeTolerance << endl;
  os << indent << "Tolerance: " << this->Tolerance << endl;
}
