/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkDistributedPointCloudFilter.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDistributedPointCloudFilter.h"

#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkMPIController.h"
#include "vtkMPIUtilities.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <algorithm>

// Histogram precision to divide space in two
static const int HISTOGRAM_SIZE = 1024;

vtkStandardNewMacro(vtkDistributedPointCloudFilter);
vtkSetObjectImplementationMacro(vtkDistributedPointCloudFilter, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkDistributedPointCloudFilter::vtkDistributedPointCloudFilter()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkDistributedPointCloudFilter::~vtkDistributedPointCloudFilter()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int vtkDistributedPointCloudFilter::FillOutputPortInformation(int port, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkDistributedPointCloudFilter::RequestData(vtkInformation *,
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  vtkPointSet *input = vtkPointSet::GetData(inputVector[0]);
  if (!input)
  {
    vtkErrorMacro("No valid input!");
    return 0;
  }

  vtkPolyData *output = vtkPolyData::GetData(outputVector);
  if (!output)
  {
    vtkErrorMacro("No output object!");
    return 0;
  }

  vtkMPIController* controller = vtkMPIController::SafeDownCast(this->Controller);
  if (!controller)
  {
    // No MPI controller? Just pass points & point data
    output->SetPoints(input->GetPoints());
    output->GetPointData()->ShallowCopy(input->GetPointData());
    return 1;
  }

  std::vector<KdTreeBuildRound> kdTreeRounds;
  if (this->InitializeKdTree(kdTreeRounds))
  {
    double bounds[6];
    this->OptimizeBoundingBox(kdTreeRounds, input, bounds);
    vtkMPIUtilities::GetPointsInsideBounds(controller, input, output, bounds);
  }
  else
  {
    vtkErrorMacro("SubCommunicators are not correctly initialized, no distribution performed");
    return 0;
  }
  // Destroy the kdtree controllers
  for (const auto &round : kdTreeRounds)
  {
    round.controller->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
bool vtkDistributedPointCloudFilter::InitializeKdTree(std::vector<KdTreeBuildRound> &kdTreeRounds)
{
  this->Controller->Register(this);

  kdTreeRounds.push_back({ vtkMPIController::SafeDownCast(this->Controller),
    this->Controller->GetNumberOfProcesses(),
    this->Controller->GetLocalProcessId() });

  int index = 0;
  while (kdTreeRounds[index].np > 2)
  {
    int split = kdTreeRounds[index].np / 2;
    int color, key;

    if (kdTreeRounds[index].rank < split)
    {
      color = 0;
      key = kdTreeRounds[index].rank;
    }
    else
    {
      color = 1;
      key = kdTreeRounds[index].rank - split;
    }

    vtkMPIController* ctrl =
      kdTreeRounds[index].controller->PartitionController(color, key);
    if (ctrl == nullptr)
    {
      break;
    }
    ++index;
    kdTreeRounds.push_back(
      { ctrl, ctrl->GetNumberOfProcesses(), ctrl->GetLocalProcessId() });
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkDistributedPointCloudFilter::OptimizeBoundingBox(
  std::vector<KdTreeBuildRound> &kdTreeRounds, vtkPointSet* pointCloud, double regionBounds[6])
{
  if (kdTreeRounds.empty())
  {
    return false;
  }

  // ****************************************
  // Compute the domains bounds

  // Separate lower and upper because Allreduce should minimize lower and maximize upper
  double localLowerBound[3];
  double localUpperBound[3];
  double currentGroupLowerBound[3];
  double currentGroupUpperBound[3];

  vtkIdType numPts = pointCloud->GetNumberOfPoints();
  if (numPts > 0)
  {
    double* bounds = pointCloud->GetBounds();
    for (int i = 0; i < 3; ++i)
    {
      localLowerBound[i] = bounds[2 * i + 0];
      localUpperBound[i] = bounds[2 * i + 1];
    }
  }
  else
  {
    for (int i = 0; i < 3; ++i)
    {
      localLowerBound[i] = +std::numeric_limits<double>::max();
      localUpperBound[i] = -std::numeric_limits<double>::max();
    }
  }

  // ****************************************
  // Main Loop: transfer points between process.
  // Point cloud is recursively splitted in two, among MPI groups.
  // Algorithm:
  // - 1. choose an axis (the longest)
  // - 2. build the local histogram of number of points along the given axis
  // - 3. compute the global histogram
  // - 4. process 0 only: find the median and the corresponding cut position and share it to others
  // - 5. split points in two groups : to keep on this node or to send
  // - 6. exchange points: each MPI process find a partner in the other MPI group.
  // - 7. update current MPI group bounds
  // Each round concerns a MPI subgroup of previous round group

  const int histSize = HISTOGRAM_SIZE;
  std::vector<int> histogram(histSize, 0);
  std::vector<int> histsum(histSize, 0);
  std::vector<vtkIdType> pointExchangeCount(kdTreeRounds[0].np, 0);
  std::vector<double> pts;

  if (numPts > 0)
  {
    pts.resize(3 * numPts);
    for (vtkIdType i = 0; i < numPts; i++)
    {
      pointCloud->GetPoint(i, &pts[i * 3]);
    }
  }

  for (size_t round = 0; round < kdTreeRounds.size(); round++)
  {
    if (kdTreeRounds[round].np == 1)
    {
      continue;
    }

    kdTreeRounds[round].controller->GetCommunicator()->AllReduceVoidArray(
      localLowerBound, currentGroupLowerBound, 3, VTK_DOUBLE, vtkCommunicator::MIN_OP);
    kdTreeRounds[round].controller->GetCommunicator()->AllReduceVoidArray(
      localUpperBound, currentGroupUpperBound, 3, VTK_DOUBLE, vtkCommunicator::MAX_OP);

    // ----------------------------------------
    // 1. choose a cut axis
    int cutaxis = 0;
    double length = currentGroupUpperBound[0] - currentGroupLowerBound[0];
    if (length < (currentGroupUpperBound[1] - currentGroupLowerBound[1]))
    {
      cutaxis = 1;
      length = currentGroupUpperBound[1] - currentGroupLowerBound[1];
    }
    if (length < (currentGroupUpperBound[2] - currentGroupLowerBound[2]))
    {
      cutaxis = 2;
      length = currentGroupUpperBound[2] - currentGroupLowerBound[2];
    }

    // ----------------------------------------
    // 2. build local histogram along chosen axis
    double offset = -currentGroupLowerBound[cutaxis];

    for (vtkIdType i = 0; i < numPts; i++)
    {
      int sampledPosition = static_cast<int>((pts[3 * i + cutaxis] + offset) / length * histSize);
      sampledPosition = std::max(std::min(sampledPosition, histSize - 1), 0);
      ++histogram[sampledPosition];
    }

    // ----------------------------------------
    // 3. reduction across round participants to get global histogram
    kdTreeRounds[round].controller->GetCommunicator()->ReduceVoidArray(
      histogram.data(), histsum.data(), histSize, VTK_INT, vtkCommunicator::SUM_OP, 0);

    vtkIdType totalNumpts = numPts;
    kdTreeRounds[round].controller->GetCommunicator()->ReduceVoidArray(
      &numPts, &totalNumpts, 1, VTK_ID_TYPE, vtkCommunicator::SUM_OP, 0);

    // ----------------------------------------
    // 4. process 0 of sub group computes cut position and broadcast it to others
    int cutpos = 0;
    if (kdTreeRounds[round].rank == 0)
    {
      // ratio of left part over total. it won't be 2 for un-even number of participant processors
      double ratio = static_cast<double>(kdTreeRounds[round].np) /
        static_cast<double>(kdTreeRounds[round].np / 2);
      int i = 1;
      for (; i < histSize; i++)
      {
        histsum[i] += histsum[i - 1];
        if (histsum[i] >= (totalNumpts / ratio))
        {
          break;
        }
      }
      cutpos = i;
    }
    kdTreeRounds[round].controller->GetCommunicator()->BroadcastVoidArray(
      &cutpos, 1, VTK_INT, 0);

    // ----------------------------------------
    // 5. classify points
    // which half of the group i belongs to. true = left, false = right
    bool side = kdTreeRounds[round].rank < (kdTreeRounds[round].np / 2);
    std::vector<double> partnerPts(3 * numPts);
    vtkIdType newNumPts = 0;
    vtkIdType partnerNumPts = 0;
    for (vtkIdType i = 0; i < numPts; i++)
    {
      int position = static_cast<int>((pts[3 * i + cutaxis] + offset) / length * histSize);
      bool where = position <= cutpos;
      if ((where && side) || (!where && !side))
      {
        pts[newNumPts * 3 + 0] = pts[i * 3 + 0];
        pts[newNumPts * 3 + 1] = pts[i * 3 + 1];
        pts[newNumPts * 3 + 2] = pts[i * 3 + 2];
        newNumPts++;
      }
      else
      {
        partnerPts[partnerNumPts * 3 + 0] = pts[i * 3 + 0];
        partnerPts[partnerNumPts * 3 + 1] = pts[i * 3 + 1];
        partnerPts[partnerNumPts * 3 + 2] = pts[i * 3 + 2];
        partnerNumPts++;
      }
    }

    kdTreeRounds[round].controller->GetCommunicator()->AllGatherVoidArray(
      &partnerNumPts, pointExchangeCount.data(), 1, VTK_ID_TYPE);

    // ----------------------------------------
    // 6. exchange points
    // find a partner
    int partner;
    if (side)
    {
      partner = kdTreeRounds[round].rank + (kdTreeRounds[round].np / 2);
    }
    else
    {
      partner = kdTreeRounds[round].rank - (kdTreeRounds[round].np / 2);
    }

    vtkIdType toReceive = pointExchangeCount[partner];

    bool even = (kdTreeRounds[round].np % 2) == 0;
    // If non even number of processes: last one send to 0 and receive nothing.
    if (!even && kdTreeRounds[round].rank == kdTreeRounds[round].np - 1)
    {
      partner = 0;
      toReceive = 0;
    }

    pts.resize(3 * (newNumPts + toReceive));

    vtkMPICommunicator *com =
      vtkMPICommunicator::SafeDownCast(kdTreeRounds[round].controller->GetCommunicator());
    vtkMPICommunicator::Request request;
    const int EXCHANGE_POINT_TAG = 524821;

    if (partnerNumPts > 0)
    {
      com->NoBlockSend(partnerPts.data(), static_cast<int>(3 * partnerNumPts), partner, EXCHANGE_POINT_TAG, request);
    }
    if (toReceive > 0)
    {
      com->ReceiveVoidArray(&pts[3 * newNumPts], static_cast<int>(3 * toReceive), VTK_DOUBLE, partner, EXCHANGE_POINT_TAG);
    }

    // If non even number of processes: 0 receive from the last one.
    if (!even && kdTreeRounds[round].rank == 0)
    {
      newNumPts += toReceive;
      partner = kdTreeRounds[round].np - 1;
      toReceive = pointExchangeCount[partner];
      pts.resize(3 * (newNumPts + toReceive));
      if (toReceive > 0)
      {
        com->ReceiveVoidArray(&pts[3 * newNumPts], 3 * toReceive, VTK_DOUBLE, partner, partner);
      }
    }

    numPts = newNumPts + toReceive;

    // ----------------------------------------
    // 7. update bounds
    for (int i = 0; i < 3; ++i)
    {
      localLowerBound[i] = std::numeric_limits<double>::max();
      localUpperBound[i] = -std::numeric_limits<double>::max();
    }

    for (vtkIdType i = 0; i < numPts; ++i)
    {
      for (int j = 0; j < 3; j++)
      {
        localLowerBound[j] = std::min(localLowerBound[j], pts[3 * i + j]);
        localUpperBound[j] = std::max(localUpperBound[j], pts[3 * i + j]);
      }
    }

    if (partnerNumPts > 0)
    {
      request.Wait();
    }

    histogram.assign(histSize, 0);
  }

  // update bounding box of the current processor assigned region
  regionBounds[0] = localLowerBound[0];
  regionBounds[2] = localLowerBound[1];
  regionBounds[4] = localLowerBound[2];
  regionBounds[1] = localUpperBound[0];
  regionBounds[3] = localUpperBound[1];
  regionBounds[5] = localUpperBound[2];

  return true;
}

//----------------------------------------------------------------------------
void vtkDistributedPointCloudFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
