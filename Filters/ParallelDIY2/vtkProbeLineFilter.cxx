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
#include "vtkCompositeDataSet.h"
#include "vtkCutter.h"
#include "vtkDIYExplicitAssigner.h"
#include "vtkDIYUtilities.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
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
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStripper.h"
#include "vtkVectorOperators.h"

#include <algorithm>
#include <cmath>
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
struct PointSetBlock
{
  std::vector<vtkNew<vtkPoints>> ReceivedPointsMap;
};

//==============================================================================
// This worker projects points from the intersection of the input line onto the line.
// It outputs a 1D coordinate system in `LineProjection`.
struct PointProjectionWorker
{
  PointProjectionWorker(vtkDataSet* pointSoup, const double origin[3], const double end[3])
    : PointSoup(pointSoup)
    , Origin(origin)
    , End(end)
  {
    this->LineProjection.resize(this->PointSoup->GetNumberOfPoints());
    vtkMath::Subtract(this->End, this->Origin, this->LineDirection);
    this->NormalizationFactor = vtkMath::Norm(this->LineDirection);
    vtkMath::Normalize(this->LineDirection);
  }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    double p[3], diff[3];
    for (vtkIdType pointId = startId; pointId < endId; ++pointId)
    {
      this->PointSoup->GetPoint(pointId, p);
      vtkMath::Subtract(p, this->Origin, diff);
      this->LineProjection[pointId] = std::make_pair(
        vtkMath::Dot(diff, this->LineDirection) / this->NormalizationFactor, pointId);
    }
  }

  vtkDataSet* PointSoup;
  const double* Origin;
  const double* End;
  bool SplitPoints;
  // LineProjection maps each point to a 1D coordinate (the double) and its index in
  // the data set consisting of the intersection between the input data set of the filter
  // and the input line.
  std::vector<std::pair<double, vtkIdType>> LineProjection;
  double LineDirection[3];
  double NormalizationFactor;
};

//==============================================================================
// This worker generates the line profile given an input of sorted points along the line.
// Its behavior depends on SamplingPattern: it duplicates the points, moving them slightly
// in each direction of the line, in the instance of `SAMPLE_LINE_AT_CELL_BOUNDARIES`.
struct ProbingPointGeneratorWorker
{
  ProbingPointGeneratorWorker(vtkDataSet* pointSoup,
    const std::vector<std::pair<double, vtkIdType>>& lineProjection, vtkIdType firstPointId,
    vtkIdType lastPointId, const double* point1, const double* point2, bool splitPoints,
    const double lineDirection[3])
    : PointSoup(pointSoup)
    , LineProjection(lineProjection)
    , FirstPointId(firstPointId)
    , Point1(point1)
    , Point2(point2)
    , LineDirection(lineDirection)
    , SplitPoints(splitPoints)
  {
    // We add 2 points to add Point1 and Point2 to the list of probed points
    if (this->SplitPoints)
    {
      this->SortedPoints->SetNumberOfPoints(2 * (lastPointId - firstPointId + 2));
    }
    else
    {
      this->SortedPoints->SetNumberOfPoints(lastPointId - firstPointId + 2);
    }
    this->LineDirectionEpsilon[0] = lineDirection[0] * 2.0 * VTK_TOL;
    this->LineDirectionEpsilon[1] = lineDirection[1] * 2.0 * VTK_TOL;
    this->LineDirectionEpsilon[2] = lineDirection[2] * 2.0 * VTK_TOL;
  }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    double point1Epsilon = VTK_DBL_EPSILON *
      std::max({ std::abs(this->Point1[0]), std::abs(this->Point1[1]), std::abs(this->Point1[2]) });
    double point2Epsilon = VTK_DBL_EPSILON *
      std::max({ std::abs(this->Point2[0]), std::abs(this->Point2[1]), std::abs(this->Point2[2]) });
    double p[3];
    for (vtkIdType pointId = startId; pointId < endId; ++pointId)
    {
      this->PointSoup->GetPoint(this->LineProjection[pointId].second, p);
      if (this->SplitPoints)
      {
        // We make sure that we do not add any point before Point1 or after Point2.
        // If this happens, we replace the point by Point1 or Point2 when appropriate.
        // In such instances, there will unnecessary duplicate points in the probing line.
        // It is not a problem for the rest of the filter, and keeping consistently
        // the rule that the first and last points are the end points of the input line
        // makes it more trivial to get rid of them if they are not required later
        // in the pipeline (getting rid of the 2 first samples and the 2 last samples
        // is sufficient).
        double pBefore[3], pAfter[3], tmp[3];

        vtkMath::Subtract(p, this->LineDirectionEpsilon, pBefore);
        vtkMath::Subtract(pBefore, this->Point1, tmp);
        if (vtkMath::Dot(this->LineDirection, tmp) < point1Epsilon)
        {
          this->SortedPoints->SetPoint((pointId - this->FirstPointId + 1) * 2, this->Point1);
        }
        else
        {
          this->SortedPoints->SetPoint((pointId - this->FirstPointId + 1) * 2, pBefore);
        }

        vtkMath::Add(p, this->LineDirectionEpsilon, pAfter);
        vtkMath::Subtract(this->Point2, pAfter, tmp);
        if (vtkMath::Dot(this->LineDirection, tmp) < point2Epsilon)
        {
          this->SortedPoints->SetPoint((pointId - this->FirstPointId + 1) * 2 + 1, this->Point2);
        }
        else
        {
          this->SortedPoints->SetPoint((pointId - this->FirstPointId + 1) * 2 + 1, pAfter);
        }
      }
      else
      {
        this->SortedPoints->SetPoint(pointId - this->FirstPointId + 1, p);
      }
    }
  }

  vtkDataSet* PointSoup;
  const std::vector<std::pair<double, vtkIdType>>& LineProjection;
  vtkIdType FirstPointId;
  const double* Point1;
  const double* Point2;
  vtkNew<vtkPoints> SortedPoints;
  const double* LineDirection;
  bool SplitPoints;
  double LineDirectionEpsilon[3];
};

//------------------------------------------------------------------------------
void RemoveDuplicate(std::vector<std::pair<double, vtkIdType>>& lineProjection)
{
  if (!lineProjection.empty())
  {
    for (std::size_t id = 0; id < lineProjection.size() - 1; ++id)
    {
      auto& pt1 = lineProjection[id];
      auto& pt2 = lineProjection[id + 1];
      if (vtkMathUtilities::NearlyEqual(pt1.first, pt2.first))
      {
        lineProjection.erase(lineProjection.begin() + id);
      }
    }
  }
}
} // anonymous namespace

//------------------------------------------------------------------------------
vtkProbeLineFilter::vtkProbeLineFilter()
  : Controller(nullptr)
  , SamplingPattern(SAMPLE_LINE_AT_CELL_BOUNDARIES)
  , LineResolution(1000)
  , Point1{ -0.5, 0.0, 0.0 }
  , Point2{ 0.5, 0.0, 0.0 }
  , ComputeTolerance(true)
  , Tolerance(1.0)
{
  this->SetNumberOfInputPorts(1);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkProbeLineFilter::~vtkProbeLineFilter()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
int vtkProbeLineFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!outInfo || !inInfo)
  {
    vtkErrorMacro("No input or output information");
  }

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
    double diff[3];
    for (vtkIdType pointId = 0; pointId < pointsRange.size(); pointId += 2)
    {
      PointRef p1 = pointsRange[pointId];
      PointRef p2 = pointsRange[pointId + 1];

      vtkMath::Subtract(this->Point1, p1, diff);
      if (vtkMathUtilities::NearlyEqual<double>(p1[0], this->Point1[0]) &&
        vtkMathUtilities::NearlyEqual<double>(p1[1], this->Point1[1]) &&
        vtkMathUtilities::NearlyEqual<double>(p1[2], this->Point1[2]))
      {
        p1[0] = p2[0] = this->Point1[0];
        p1[1] = p2[1] = this->Point1[1];
        p1[2] = p2[2] = this->Point1[2];
        continue;
      }

      vtkMath::Subtract(this->Point2, p2, diff);
      if (vtkMathUtilities::NearlyEqual<double>(p2[0], this->Point2[0]) &&
        vtkMathUtilities::NearlyEqual<double>(p2[1], this->Point2[1]) &&
        vtkMathUtilities::NearlyEqual<double>(p2[2], this->Point2[2]))
      {
        p1[0] = p2[0] = this->Point2[0];
        p1[1] = p2[1] = this->Point2[1];
        p1[2] = p2[2] = this->Point2[2];
        continue;
      }

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
  vtkVector3d p12 = vtkVector3d(this->Point2) - vtkVector3d(this->Point1);

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

  p12.Normalize();

  double v[3] = { 0.0, 1.0, 0.0 };
  if (std::abs(p12[1] - 1.0) > 0.1)
  {
    // We swap coordinates of v if it is colinear with p12
    std::swap(v[0], v[1]);
  }

  double n1[3], n2[3];
  vtkMath::Cross(p12.GetData(), v, n1);
  vtkMath::Normalize(n1);
  vtkMath::Cross(p12.GetData(), n1, n2);

  vtkNew<vtkPlane> plane1;
  plane1->SetOrigin(this->Point1);

  vtkNew<vtkPlane> plane2;
  plane2->SetOrigin(this->Point1);

  // Strategy: extract the intersection between the input line and the data set
  // by concatenating 2 slicing planes.
  // Doing that on 2D inputs is likely not working and may output a data set with zero
  // points. This is taken cared of by using the first slice plane rather than the second
  // when the second slice plane outputs nothing.

  vtkNew<vtkAppendDataSets> appender;
  bool emptyInputInAppender = true;

  for (std::size_t dsId = 0; dsId < inputs.size(); ++dsId)
  {
    vtkDataSet* input = inputs[dsId];

    vtkBoundingBox bb(input->GetBounds());
    // This test is important, because if we slice outside of a data set inside a composite data
    // set, we can end up with a 2D plane output instead of a 1D line when we swap cut plane
    // normals later in the loop.
    if (!bb.IntersectsLine(this->Point1, this->Point2))
    {
      continue;
    }

    plane1->SetNormal(n1);
    plane2->SetNormal(n2);

    vtkNew<vtkCutter> slice2D;
    slice2D->SetCutFunction(plane1);
    slice2D->SetInputData(input);
    slice2D->GenerateTrianglesOff();

    slice2D->Update();
    if (!vtkDataSet::SafeDownCast(slice2D->GetOutputDataObject(0))->GetNumberOfPoints())
    {
      // This happens if the slice plane is coplanar with a 2D input dataset.
      // We swap normals in this case.
      plane1->SetNormal(n2);
      plane2->SetNormal(n1);
    }

    vtkNew<vtkCutter> slice1D;
    slice1D->SetCutFunction(plane2);
    slice1D->SetInputConnection(slice2D->GetOutputPort());
    slice1D->GenerateTrianglesOff();

    slice1D->Update();
    vtkSmartPointer<vtkDataSet> pointSoup(
      vtkDataSet::SafeDownCast(slice1D->GetOutputDataObject(0)));
    if (!pointSoup->GetNumberOfPoints())
    {
      // This only happens when the input is a 2D data set. We want to use the first
      // slice plane and ignore the second one.
      pointSoup = vtkDataSet::SafeDownCast(slice2D->GetOutputDataObject(0));
    }

    if (this->SamplingPattern == SAMPLE_LINE_AT_SEGMENT_CENTERS)
    {
      vtkNew<vtkCellCenters> cellCenters;
      cellCenters->SetInputData(pointSoup);
      cellCenters->Update();
      pointSoup = vtkDataSet::SafeDownCast(cellCenters->GetOutputDataObject(0));
    }

    appender->AddInputData(pointSoup);
    emptyInputInAppender = false;
  }

  if (emptyInputInAppender)
  {
    vtkNew<vtkLineSource> line;
    line->SetPoint1(this->Point1);
    line->SetPoint2(this->Point2);
    line->Update();
    return vtkPolyData::SafeDownCast(line->GetOutputDataObject(0));
  }

  appender->Update();
  vtkPointSet* pointSoup = vtkPointSet::SafeDownCast(appender->GetOutputDataObject(0));

  // We need to gather points from every ranks to everyranks because vtkProbeFilter
  // assumes that its input is replicated in every ranks.

  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(this->Controller);

  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(new PointSetBlock()); },
    [](void* b) -> void { delete static_cast<PointSetBlock*>(b); });

  vtkDIYExplicitAssigner assigner(comm, 1);

  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
    /*dim*/ 1, diy::interval(0, assigner.nblocks() - 1), assigner.nblocks());

  decomposer.decompose(comm.rank(), assigner, master);

  diy::all_to_all(
    master, assigner, [&master, pointSoup](PointSetBlock* block, const diy::ReduceProxy& srp) {
      int myBlockId = srp.gid();
      if (srp.round() == 0)
      {
        for (int i = 0; i < srp.out_link().size(); ++i)
        {
          const diy::BlockID& blockId = srp.out_link().target(i);
          if (blockId.gid != myBlockId)
          {
            srp.enqueue(blockId, pointSoup->GetPoints()->GetData());
          }
        }
      }
      else
      {
        block->ReceivedPointsMap.reserve(srp.in_link().size());
        for (int i = 0; i < static_cast<int>(srp.in_link().size()); ++i)
        {
          const diy::BlockID& blockId = srp.in_link().target(i);
          if (blockId.gid != myBlockId)
          {
            vtkDataArray* array = nullptr;
            srp.dequeue(blockId, array);
            vtkNew<vtkPoints> points;
            points->SetData(array);
            block->ReceivedPointsMap.emplace_back(std::move(points));
            array->FastDelete();
          }
        }
      }
    });

  vtkNew<vtkPointSet> reducedPointSoup;
  reducedPointSoup->DeepCopy(pointSoup);

  PointSetBlock* block = master.block<PointSetBlock>(0);
  vtkPoints* points = reducedPointSoup->GetPoints();
  for (vtkPoints* source : block->ReceivedPointsMap)
  {
    points->InsertPoints(points->GetNumberOfPoints(), source->GetNumberOfPoints(), 0, source);
  }

  PointProjectionWorker pointProjectionWorker(reducedPointSoup, this->Point1, this->Point2);
  vtkSMPTools::For(0, reducedPointSoup->GetNumberOfPoints(), pointProjectionWorker);

  auto& lineProjection = pointProjectionWorker.LineProjection;
  std::sort(lineProjection.begin(), lineProjection.end());

  // Duplicate points can happen on composite data set.
  RemoveDuplicate(lineProjection);

  vtkIdType firstPointIdInSegment = 0;
  while (firstPointIdInSegment < static_cast<vtkIdType>(lineProjection.size()))
  {
    if (lineProjection[firstPointIdInSegment].first < -VTK_DBL_EPSILON)
    {
      ++firstPointIdInSegment;
    }
    else
    {
      break;
    }
  }
  vtkIdType lastPointIdInSegment = firstPointIdInSegment;
  while (lastPointIdInSegment < static_cast<vtkIdType>(lineProjection.size()))
  {
    if (lineProjection[lastPointIdInSegment].first < (1.0 + VTK_DBL_EPSILON))
    {
      ++lastPointIdInSegment;
    }
    else
    {
      break;
    }
  }

  bool splitPoints = this->SamplingPattern == SAMPLE_LINE_AT_CELL_BOUNDARIES;

  ProbingPointGeneratorWorker probingPointGeneratorWorker(reducedPointSoup, lineProjection,
    firstPointIdInSegment, lastPointIdInSegment, this->Point1, this->Point2, splitPoints,
    pointProjectionWorker.LineDirection);
  vtkSMPTools::For(firstPointIdInSegment, lastPointIdInSegment, probingPointGeneratorWorker);

  vtkPoints* sortedPoints = probingPointGeneratorWorker.SortedPoints;

  sortedPoints->SetPoint(0, this->Point1);

  if (this->SamplingPattern == SAMPLE_LINE_AT_CELL_BOUNDARIES)
  {
    double tmp[3];
    // This tmp adding at each end of the line is essential.
    // If Point1 or Point2 lies inside a cell boundary, cell data probing might not output a step
    // function.
    vtkMath::Add(this->Point1, probingPointGeneratorWorker.LineDirectionEpsilon, tmp);
    sortedPoints->SetPoint(1, tmp);
  }

  sortedPoints->SetPoint(sortedPoints->GetNumberOfPoints() - 1, this->Point2);

  if (this->SamplingPattern == SAMPLE_LINE_AT_CELL_BOUNDARIES)
  {
    double tmp[3];
    vtkMath::Subtract(this->Point2, probingPointGeneratorWorker.LineDirectionEpsilon, tmp);
    sortedPoints->SetPoint(sortedPoints->GetNumberOfPoints() - 2, tmp);
  }

  vtkNew<vtkPolyLineSource> polyLine;
  polyLine->SetPoints(sortedPoints);
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
