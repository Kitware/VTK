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
#include "vtkCellCenters.h"
#include "vtkCellLocatorStrategy.h"
#include "vtkCompositeDataSet.h"
#include "vtkCutter.h"
#include "vtkDIYExplicitAssigner.h"
#include "vtkDIYUtilities.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkFindCellStrategy.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLineSource.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
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

//------------------------------------------------------------------------------
struct vtkProbeLineFilter::vtkInternals
{
  vtkMTimeType PreviousInputTime = 0;
  std::map<vtkDataSet*,vtkSmartPointer<vtkFindCellStrategy>> Strategies;

  void UpdateLocators(vtkDataObject* input, int pattern)
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
        vtkNew<vtkStaticCellLocator> locator;
        locator->SetDataSet(ds);
        locator->SetTolerance(0.0);
        locator->UseDiagonalLengthToleranceOff();
        locator->BuildLocator();

        vtkCellLocatorStrategy* strategy = vtkCellLocatorStrategy::New();
        strategy->SetCellLocator(locator);
        vtkSmartPointer<vtkFindCellStrategy> holder;
        holder.TakeReference(static_cast<vtkFindCellStrategy*>(strategy));

        this->Strategies[ds] = holder;
      }
    }
  }
};

namespace
{
//==============================================================================
struct ProjInfo
{
  double Projection;
  vtkIdType CellID;

  static bool SortFunction(const ProjInfo& l, const ProjInfo& r)
  {
    return l.Projection < r.Projection;
  }
  static bool EqualFunction(const ProjInfo& l, const ProjInfo& r)
  {
    return l.CellID == r.CellID && vtkMathUtilities::NearlyEqual(l.Projection, r.Projection);
  }
};
struct OptionalPosition
{
  bool IsValid;
  vtkVector3d Position;
};

//==============================================================================
/**
 * Return the entry point and exit point (respectively result.first and result.second) of a given 3D cell for
 * the segment [p1,p2]. If only one intersection is found (can happen at the very tip of the cell) only the out
 * position is valid. If no intersection then both intersections are not valid.
 *
 * XXX(c++17): replace `OptionalPosition` with `std::optional<vtkVector3d>`
 */
std::pair<OptionalPosition,OptionalPosition> GetInOutCell3D(const vtkVector3d& p1, const vtkVector3d& p2, vtkCell* cell, double tolerance)
{
  using PosAndDist = std::pair<vtkVector3d,double>;

  const int nface = cell->GetNumberOfFaces();
  std::vector<PosAndDist> intersections;
  intersections.reserve(nface);
  double t, x[3], dummy3[3];
  int dummy;
  for (int i = 0; i < nface; ++i)
  {
    if (cell->GetFace(i)->IntersectWithLine(p1.GetData(), p2.GetData(), 0.0, t, x, dummy3, dummy))
    {
      intersections.emplace_back(PosAndDist(vtkVector3d(x), t));
    }
  }
  std::sort(intersections.begin(), intersections.end(), [](const PosAndDist& l, const PosAndDist& r)
    {
      return l.second < r.second;
    });
  auto last = std::unique(intersections.begin(), intersections.end(), [tolerance](const PosAndDist& l, const PosAndDist& r)
    {
      return vtkMathUtilities::NearlyEqual(l.second, r.second, tolerance);
    });

  const std::size_t size = std::distance(intersections.begin(), last);
  OptionalPosition inProj{false, vtkVector3d()};
  OptionalPosition outProj{false, vtkVector3d()};
  if (size == 1)
  {
    outProj = {true, intersections[0].first};
  }
  else if (size >= 2)
  {
    inProj = {true, intersections[0].first};
    outProj = {true, (last - 1)->first};
  }

  return std::pair<OptionalPosition,OptionalPosition>(inProj, outProj);
}

//==============================================================================
/**
 * Process the limit points of the intersection of the @c input dataset (and its @c locator) with the segment [p1,p2].
 * Return the list of intersection projections between [p1,p2] and the cells p1 and p2 belong to. Never returns p1 nor
 * p2 itself.
 */
std::vector<ProjInfo> ProcessLimitPoints(vtkVector3d p1, vtkVector3d p2, int pattern, vtkDataSet* input, vtkAbstractCellLocator* locator, double tolerance)
{
  std::vector<ProjInfo> projections;
  vtkVector3d nline = (p2 - p1).Normalized();

  vtkIdType p1Cell = locator->FindCell(p1.GetData());
  if (p1Cell >= 0)
  {
    vtkCell* cell = input->GetCell(p1Cell);
    if (cell->GetCellDimension() == 3)
    {
      OptionalPosition outPos = ::GetInOutCell3D(p1, p2, cell, tolerance).second;
      if (outPos.IsValid)
      {
        double projection = (pattern == vtkProbeLineFilter::SamplingPattern::SAMPLE_LINE_AT_SEGMENT_CENTERS)
          ? nline.Dot(outPos.Position - p1) * 0.5
          : nline.Dot((outPos.Position - p1) - tolerance * nline);
        projections.emplace_back(ProjInfo{projection, p1Cell});
      }
    }
  }

  vtkIdType p2Cell = locator->FindCell(p2.GetData());
  if (p2Cell >= 0)
  {
    vtkCell* cell = input->GetCell(p2Cell);
    if (cell->GetCellDimension() == 3)
    {
      auto inOut = ::GetInOutCell3D(p1, p2, cell, tolerance);
      OptionalPosition inPos = inOut.first.IsValid ? inOut.first : inOut.second;
      if (inPos.IsValid)
      {
        double projection = (pattern == vtkProbeLineFilter::SamplingPattern::SAMPLE_LINE_AT_SEGMENT_CENTERS)
          ? nline.Dot((inPos.Position + p2) * 0.5 - p1)
          : nline.Dot((inPos.Position - p1) + tolerance * nline);
        projections.emplace_back(ProjInfo{projection, p2Cell});
      }
    }
  }

  return projections;
}
}

//------------------------------------------------------------------------------
vtkProbeLineFilter::vtkProbeLineFilter()
  : Controller(nullptr)
  , SamplingPattern(SAMPLE_LINE_AT_CELL_BOUNDARIES)
  , LineResolution(1000)
  , Point1{ -0.5, 0.0, 0.0 }
  , Point2{ 0.5, 0.0, 0.0 }
  , ComputeTolerance(true)
  , Tolerance(1.0)
  , Internal(new vtkInternals)
{
  this->SetNumberOfInputPorts(1);
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
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo || !inInfo)
  {
    vtkErrorMacro("No input or output information");
  }

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->Internal->UpdateLocators(input, this->SamplingPattern);

  vtkSmartPointer<vtkPolyData> sampledLine;
  switch (this->SamplingPattern)
  {
    case SAMPLE_LINE_AT_CELL_BOUNDARIES:
    case SAMPLE_LINE_AT_SEGMENT_CENTERS:
      sampledLine = this->SampleLineAtEachCell(vtkCompositeDataSet::GetDataSets(input));
      break;
    case SAMPLE_LINE_UNIFORMLY:
      sampledLine = this->SampleLineUniformly();
      break;
    default:
      vtkErrorMacro("Sampling heuristic wrongly set... Aborting");
      return 0;
  }

  vtkNew<vtkPProbeFilter> prober;
  prober->SetController(this->Controller);
  prober->SetPassPartialArrays(this->PassPartialArrays);
  prober->SetPassCellArrays(this->PassCellArrays);
  prober->SetPassPointArrays(this->PassPointArrays);
  prober->SetPassFieldArrays(this->PassFieldArrays);
  prober->SetComputeTolerance(false);
  prober->SetTolerance(0.0);
  prober->SetSourceData(input);
  prober->SetFindCellStrategyMap(this->Internal->Strategies);
  prober->SetInputData(sampledLine);
  prober->Update();

  if (this->Controller->GetLocalProcessId() == 0 &&
    this->SamplingPattern == SAMPLE_LINE_AT_CELL_BOUNDARIES)
  {
    // We move points to the cell interfaces.
    // They were artificially moved away from the cell interfaces so probing works well.
    vtkPointSet* points = vtkPointSet::SafeDownCast(prober->GetOutputDataObject(0));
    auto pointsRange = vtk::DataArrayTupleRange<3>(points->GetPoints()->GetData());
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

  output->ShallowCopy(arcs->GetOutputDataObject(0));

  return 1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkProbeLineFilter::SampleLineUniformly() const
{
  vtkNew<vtkLineSource> lineSource;
  lineSource->SetPoint1(this->Point1);
  lineSource->SetPoint2(this->Point2);
  lineSource->SetResolution(this->LineResolution);
  lineSource->Update();
  return vtkPolyData::SafeDownCast(lineSource->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkProbeLineFilter::SampleLineAtEachCell(
  const std::vector<vtkDataSet*>& inputs) const
{
  if (vtkMathUtilities::NearlyEqual(this->Point1[0], this->Point2[0]) &&
    vtkMathUtilities::NearlyEqual(this->Point1[1], this->Point2[1]) &&
    vtkMathUtilities::NearlyEqual(this->Point1[2], this->Point2[2]))
  {
    // In this instance, we probe only Point1 and Point2.
    vtkNew<vtkLineSource> line;
    line->SetPoint1(this->Point1);
    line->SetPoint2(this->Point2);
    line->Update();
    return vtkPolyData::SafeDownCast(line->GetOutputDataObject(0));
  }

  // Initialize useful variables and result array
  const vtkVector3d p1{this->Point1};
  const vtkVector3d p2{this->Point2};
  vtkVector3d v12 = (p2 - p1).Normalized();
  const double tolerance = this->ComputeTolerance
    ? VTK_TOL * (p2 - p1).Norm()
    : this->Tolerance;
  std::vector<ProjInfo> projections;

  // Add every intersection with all blocks of the dataset on our current rank
  for (std::size_t dsId = 0; dsId < inputs.size(); ++dsId)
  {
    vtkDataSet* input = inputs[dsId];
    auto* strategy = vtkCellLocatorStrategy::SafeDownCast(this->Internal->Strategies.at(input));
    assert(strategy);
    vtkAbstractCellLocator* locator = strategy->GetCellLocator();

    vtkNew<vtkIdList> intersected;
    locator->FindCellsAlongLine(this->Point1, this->Point2, 0.0, intersected);

    // We process p1 and p2 a bit differently so in the case of their intersection with a cell
    // they are not duplicated
    auto processedProj = ::ProcessLimitPoints(p1, p2, this->SamplingPattern, input, locator, tolerance);
    for (const ProjInfo& proj : processedProj)
    {
      intersected->DeleteId(proj.CellID);
    }
    projections.insert(projections.end(), processedProj.begin(), processedProj.end());

    // Process every cell intersection once we're done with limit points
    double t, x[3], pcoords[3];
    int subId;
    for (vtkIdType i = 0; i < intersected->GetNumberOfIds(); ++i)
    {
      vtkIdType cellId = intersected->GetId(i);
      vtkCell* cell = input->GetCell(cellId);
      if (cell->GetCellDimension() == 3)
      {
        auto inOut = ::GetInOutCell3D(p1, p2, cell, tolerance);
        if (inOut.first.IsValid)
        {
          double inProj = v12.Dot((inOut.first.Position - p1) + tolerance * v12);
          double outProj = v12.Dot((inOut.second.Position - p1) - tolerance * v12);
          if (this->SamplingPattern == SamplingPattern::SAMPLE_LINE_AT_CELL_BOUNDARIES)
          {
            projections.emplace_back(ProjInfo{inProj, cellId});
            projections.emplace_back(ProjInfo{outProj, cellId});
          }
          else
          {
            projections.emplace_back(ProjInfo{(inProj + outProj) * 0.5, cellId});
          }
        }
      }
      else
      {
        cell->IntersectWithLine(p1.GetData(), p2.GetData(), tolerance, t, x, pcoords, subId);
        projections.emplace_back(ProjInfo{v12.Dot(vtkVector3d(x) - p1), cellId});
      }
    }
  }

  // Sort our array of projections so the merge across ranks is faster afterwards.
  vtkSMPTools::Sort(projections.begin(), projections.end(), &ProjInfo::SortFunction);

  // We need to gather points from every ranks to every ranks because vtkProbeFilter
  // assumes that its input is replicated in every ranks.
  using PointSetBlock = std::vector<std::vector<ProjInfo>>;
  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(this->Controller);
  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(new PointSetBlock()); },
    [](void* b) -> void { delete static_cast<PointSetBlock*>(b); });
  vtkDIYExplicitAssigner assigner(comm, 1);
  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
    /*dim*/ 1, diy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
  decomposer.decompose(comm.rank(), assigner, master);

  diy::all_to_all(
    master, assigner, [&master, &projections](PointSetBlock* block, const diy::ReduceProxy& srp) {
      int myBlockId = srp.gid();
      if (srp.round() == 0)
      {
        for (int i = 0; i < srp.out_link().size(); ++i)
        {
          const diy::BlockID& blockId = srp.out_link().target(i);
          if (blockId.gid != myBlockId)
          {
            srp.enqueue(blockId, projections);
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
            std::vector<ProjInfo> data;
            srp.dequeue(blockId, data);
            block->emplace_back(std::move(data));
          }
        }
      }
    });

  // Merge local projections with projections from all other ranks
  PointSetBlock* block = master.block<PointSetBlock>(0);
  for (const auto& distProjections : *block)
  {
    auto prevEnd = projections.insert(projections.end(), distProjections.begin(), distProjections.end());
    std::inplace_merge(projections.begin(), prevEnd, projections.end(), &ProjInfo::SortFunction);
  }

  // Duplicate points can happen on composite data set so lets remove them
  auto last = std::unique(projections.begin(), projections.end(), &ProjInfo::EqualFunction);
  auto projectionSize = std::distance(projections.begin(), last);

  // Transform projection back to coordinates
  vtkNew<vtkPoints> coordinates;
  coordinates->SetNumberOfPoints(projectionSize + 2);
  coordinates->SetPoint(0, this->Point1);
  coordinates->SetPoint(coordinates->GetNumberOfPoints() - 1, this->Point2);
  vtkSMPTools::For(0, projectionSize, [&p1, &p2, &v12, &projections, &coordinates](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType pointId = begin; pointId < end; ++pointId)
      {
        vtkVector3d p = p1 + v12 * projections[pointId].Projection;
        coordinates->SetPoint(pointId + 1, p.GetData());
      }
    });

  vtkNew<vtkPolyLineSource> polyLine;
  polyLine->SetPoints(coordinates);
  polyLine->Update();

  return vtkPolyData::SafeDownCast(polyLine->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
int vtkProbeLineFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
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
  os << indent << "PassPartialArrays: " << this->PassPartialArrays << endl;
  os << indent << "PassCellArrays: " << this->PassCellArrays << endl;
  os << indent << "PassPointArrays: " << this->PassPointArrays << endl;
  os << indent << "PassFieldArrays: " << this->PassFieldArrays << endl;
  os << indent << "ComputeTolerance: " << this->ComputeTolerance << endl;
  os << indent << "Tolerance: " << this->Tolerance << endl;
  os << indent << "Point1 = [" << this->Point1[0] << ", " << this->Point1[1] << ", "
     << this->Point1[2] << "]" << endl;
  os << indent << "Point2 = [" << this->Point2[0] << ", " << this->Point2[1] << ", "
     << this->Point2[2] << "]" << endl;
}
