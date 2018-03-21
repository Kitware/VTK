/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkYoungsMaterialInterfaceCEA.h,v $

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
#include "vtkInformationVector.h"
#include "vtkMPIController.h"
#include "vtkMPIUtilities.h"
#include "vtkMath.h"
#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"

#include <algorithm>

// Histogram precision to divide space in two
static const int HISTOGRAM_SIZE = 1024;

vtkStandardNewMacro(vtkDistributedPointCloudFilter);

//----------------------------------------------------------------------------
vtkDistributedPointCloudFilter::vtkDistributedPointCloudFilter()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
}

//----------------------------------------------------------------------------
int vtkDistributedPointCloudFilter::RequestData(vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    vtkErrorMacro("No valid input!");
    return 0;
  }

  vtkPointSet* output = vtkPointSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    vtkErrorMacro("No output object!");
    return 0;
  }

  if (this->InitializeKdTree())
  {
    double bounds[6];
    this->OptimizeBoundingBox(input, bounds);
    vtkMPIController* mpiController = vtkMPIController::SafeDownCast(this->Controller);
    vtkMPIUtilities::GetPointsInsideBounds(mpiController, input, output, bounds);
  }
  else
  {
    output->ShallowCopy(input);
    vtkErrorMacro("SubCommunicators are not correctly initialized, no distribution performed");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
bool vtkDistributedPointCloudFilter::InitializeKdTree()
{
  // Remove existing communicators
  for (size_t i = 1; i < this->KdTreeRound.size(); i++)
  {
    this->KdTreeRound[i].controller->Delete();
  }
  this->KdTreeRound.clear();

  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());
  if (!com)
  {
    vtkErrorMacro("No MPI communicator!");
    this->Controller = nullptr;
    return false;
  }

  int index = 0;
  this->KdTreeRound.push_back({ this->Controller,
    this->Controller->GetNumberOfProcesses(),
    this->Controller->GetLocalProcessId() });

  while (this->KdTreeRound[index].np > 2)
  {
    int split = this->KdTreeRound[index].np / 2;
    int color = -1;
    int key = -1;

    if (this->KdTreeRound[index].rank < split)
    {
      color = 0;
      key = this->KdTreeRound[index].rank;
    }
    else
    {
      color = 1;
      key = this->KdTreeRound[index].rank - split;
    }

    vtkMultiProcessController* ctrl =
      this->KdTreeRound[index].controller->PartitionController(color, key);
    if (ctrl != nullptr)
    {
      ++index;
      this->KdTreeRound.push_back(
        { ctrl, ctrl->GetNumberOfProcesses(), ctrl->GetLocalProcessId() });
    }
    else
    {
      break;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkDistributedPointCloudFilter::OptimizeBoundingBox(vtkPointSet* pointCloud,
  double regionBounds[6])
{
  if (!pointCloud)
  {
    vtkErrorMacro("No input point set!");
    return false;
  }

  if (this->KdTreeRound.empty())
  {
    vtkErrorMacro("No communicator found!");
    return false;
  }

  vtkIdType numPts = pointCloud->GetNumberOfPoints();

  // ****************************************
  // Compute the domains bounds

  // Separate lower and upper because Allreduce should minimize lower and maximize upper
  double localLowerBound[3] = { 0, 0, 0 };
  double localUpperBound[3] = { 0, 0, 0 };
  double currentGroupLowerBound[3] = { 0, 0, 0 };
  double currentGroupUpperBound[3] = { 0, 0, 0 };

  if (numPts > 0)
  {
    double* bounds = pointCloud->GetBounds();
    for (int i = 0; i < 3; ++i)
    {
      localLowerBound[i] = bounds[2 * i];
      localUpperBound[i] = bounds[2 * i + 1];
    }
  }
  else
  {
    for (int i = 0; i < 3; ++i)
    {
      localLowerBound[i] = std::numeric_limits<double>::max();
      localUpperBound[i] = -std::numeric_limits<double>::max();
    }
  }

  // ****************************************
  // Main Loop: transfer points between process.
  // Point cloud is recursively splitted in two, among MPI groups.
  // Algorithm:
  // - 1. choose an axis. (the longest)
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
  std::vector<vtkIdType> pointExchangeCount(this->KdTreeRound[0].np, 0);

  // put 0 in vector to ensure there is data allocation.
  // method pts.data() should not return nullptr because it is used to receive data later.
  std::vector<double> pts{0};

  if (numPts > 0)
  {
    pts.resize(3 * numPts);
    vtkDataArray* ptsArray = pointCloud->GetPoints()->GetData();
    for (vtkIdType i = 0; i < numPts; i++)
    {
      double tuple[3];
      ptsArray->GetTuple(i, tuple);
      pts[i * 3 + 0] = tuple[0];
      pts[i * 3 + 1] = tuple[1];
      pts[i * 3 + 2] = tuple[2];
    }
  }

  size_t round = 0;
  while (round < this->KdTreeRound.size())
  {
    if (this->KdTreeRound[round].np == 1)
    {
      ++round;
      continue;
    }

    this->KdTreeRound[round].controller->GetCommunicator()->AllReduceVoidArray(
      localLowerBound, currentGroupLowerBound, 3, VTK_DOUBLE, vtkCommunicator::MIN_OP);
    this->KdTreeRound[round].controller->GetCommunicator()->AllReduceVoidArray(
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
      if (sampledPosition < 0)
      {
        sampledPosition = 0;
      }
      if (sampledPosition >= histSize)
      {
        sampledPosition = histSize - 1;
      }
      ++histogram[sampledPosition];
    }

    // 3. reduction across round participants to get global histogram
    this->KdTreeRound[round].controller->GetCommunicator()->ReduceVoidArray(
      histogram.data(), histsum.data(), histSize, VTK_INT, vtkCommunicator::SUM_OP, 0);

    vtkIdType totalNumpts = numPts;
    this->KdTreeRound[round].controller->GetCommunicator()->ReduceVoidArray(
      &numPts, &totalNumpts, 1, VTK_ID_TYPE, vtkCommunicator::SUM_OP, 0);

    // 4. process 0 of sub group computes cut position and broadcast it to others
    int cutpos = 0;
    if (this->KdTreeRound[round].rank == 0)
    {
      // ratio of left part over total. it won't be 2 for un-even number of participant processors
      double ratio = static_cast<double>(this->KdTreeRound[round].np) /
        static_cast<double>(this->KdTreeRound[round].np / 2);
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
    this->KdTreeRound[round].controller->GetCommunicator()->BroadcastVoidArray(
      &cutpos, 1, VTK_INT, 0);

    // ----------------------------------------
    // 5. classify points

    // which half of the group i belongs to. true = left, false = right
    bool side = this->KdTreeRound[round].rank < (this->KdTreeRound[round].np / 2);
    std::vector<double> partnerPts{0};
    vtkIdType newNumPts = 0;
    vtkIdType partnerNumPts = 0;
    if (numPts > 0)
    {
      partnerPts.resize(3 * numPts);
      for (vtkIdType i = 0; i < numPts; i++)
      {
        int position = static_cast<int>((pts[3 * i + cutaxis] + offset) / length * histSize);
        bool where = position <= cutpos;
        if ((where && side) || (!where && !side))
        {
          pts[newNumPts * 3 + 0] = pts[i * 3 + 0];
          pts[newNumPts * 3 + 1] = pts[i * 3 + 1];
          pts[newNumPts * 3 + 2] = pts[i * 3 + 2];
          ++newNumPts;
        }
        else
        {
          partnerPts[partnerNumPts * 3 + 0] = pts[i * 3 + 0];
          partnerPts[partnerNumPts * 3 + 1] = pts[i * 3 + 1];
          partnerPts[partnerNumPts * 3 + 2] = pts[i * 3 + 2];
          ++partnerNumPts;
        }
      }
    }

    this->KdTreeRound[round].controller->GetCommunicator()->AllGatherVoidArray(
      &partnerNumPts, pointExchangeCount.data(), 1, VTK_ID_TYPE);

    // ----------------------------------------
    // 6. exchange points

    // find a partner
    int partner = -1;
    if (side)
    {
      partner = (this->KdTreeRound[round].rank + (this->KdTreeRound[round].np / 2));
    }
    else
    {
      partner = this->KdTreeRound[round].rank - (this->KdTreeRound[round].np / 2);
    }

    vtkIdType toReceive = pointExchangeCount[partner];

    bool even = (this->KdTreeRound[round].np % 2) == 0;
    // If non even number of processes, last one send to 0 and receive nothing.
    if (!even && this->KdTreeRound[round].rank == this->KdTreeRound[round].np - 1)
    {
      partner = 0;
      toReceive = 0;
    }

    pts.resize(3 * (newNumPts + toReceive) );

    vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(
      this->KdTreeRound[round].controller->GetCommunicator());
    vtkMPICommunicator::Request request;
    if (partnerNumPts > 0)
    {
      com->NoBlockSend(partnerPts.data(), 3 * partnerNumPts, partner, this->KdTreeRound[round].rank, request);
    }
    if (toReceive > 0)
    {
      com->ReceiveVoidArray(&pts[3 * newNumPts], 3 * toReceive, VTK_DOUBLE, partner, partner);
    }

    // non even number of processes: 0 receive from the last one.
    if (!even && this->KdTreeRound[round].rank == 0)
    {
      newNumPts += toReceive;
      partner = this->KdTreeRound[round].np - 1;
      toReceive = pointExchangeCount[partner];
      pts.resize(3 * (newNumPts + toReceive));
      if (toReceive > 0)
      {
        com->ReceiveVoidArray(&pts[3 * newNumPts], 3 * toReceive, VTK_DOUBLE, partner, partner);
      }
    }

    numPts = newNumPts + toReceive;

    // 7. update bounds
    for (int i = 0; i < 3; ++i)
    {
      localLowerBound[i] = std::numeric_limits<double>::max();
      localUpperBound[i] = -std::numeric_limits<double>::max();
    }

    for (int i = 0; i < numPts; ++i)
    {
      for (int j = 0; j<3; j++)
      {
        localLowerBound[j] = std::min(localLowerBound[j], pts[3 * i + j]);
        localUpperBound[j] = std::max(localUpperBound[j], pts[3 * i + j]);
      }
    }

    histogram.assign(histSize, 0);

    ++round;
  }

  // update bounding box of the current processor assigned region
  regionBounds[0] = localLowerBound[0];
  regionBounds[2] = localLowerBound[1];
  regionBounds[4] = localLowerBound[2];
  regionBounds[1] = localUpperBound[0];
  regionBounds[3] = localUpperBound[1];
  regionBounds[5] = localUpperBound[2];

  for (size_t i = 1; i < this->KdTreeRound.size(); i++)
  {
    this->KdTreeRound[i].controller->Delete();
  }
  this->KdTreeRound.clear();

  return true;
}

//----------------------------------------------------------------------------
void vtkDistributedPointCloudFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of MPI groups: " << this->KdTreeRound.size() << "\n";
}
