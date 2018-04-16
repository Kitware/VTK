/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPIUtilities.h"

// VTK includes
#include "vtkCharArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkOctreePointLocator.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

// C/C++ includes
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdio>

namespace vtkMPIUtilities
{


void Printf(vtkMPIController* comm, const char* format, ...)
{
  // Sanity checks
  assert("pre: MPI controller is nullptr!" && (comm != nullptr) );
  assert("pre: format argument is nullptr!" && (format != nullptr) );

  if( comm->GetLocalProcessId() == 0 )
  {
    va_list argptr;
    va_start(argptr,format);
    vprintf(format,argptr);
    fflush(stdout);
    va_end(argptr);
  }

  comm->Barrier();
}

//------------------------------------------------------------------------------
void SynchronizedPrintf(vtkMPIController* comm, const char* format, ...)
{
  // Sanity checks
  assert("pre: MPI controller is nullptr!" && (comm != nullptr) );
  assert("pre: format argument is nullptr!" && (format != nullptr) );

  int rank     = comm->GetLocalProcessId();
  int numRanks = comm->GetNumberOfProcesses();


  vtkMPICommunicator::Request rqst;
  int* nullmsg = nullptr;

  if(rank == 0)
  {
    // STEP 0: print message
    printf("[%d]: ", rank);
    fflush(stdout);

    va_list argptr;
    va_start(argptr,format);
    vprintf(format,argptr);
    fflush(stdout);
    va_end(argptr);

    // STEP 1: signal next process (if any) to print
    if( numRanks > 1)
    {
      comm->NoBlockSend(nullmsg,0,rank+1,0,rqst);
    } // END if
  } // END first rank
  else if( rank == numRanks-1 )
  {
    // STEP 0: Block until previous process completes
    comm->Receive(nullmsg,0,rank-1,0);

    // STEP 1: print message
    printf("[%d]: ", rank);

    va_list argptr;
    va_start(argptr,format);
    vprintf(format,argptr);
    fflush(stdout);
    va_end(argptr);
  } // END last rank
  else
  {
    // STEP 0: Block until previous process completes
    comm->Receive(nullmsg,0,rank-1,0);

    // STEP 1: print message
    printf("[%d]: ", rank);

    va_list argptr;
    va_start(argptr,format);
    vprintf(format,argptr);
    fflush(stdout);
    va_end(argptr);

    // STEP 2: signal next process to print
    comm->NoBlockSend(nullmsg,0,rank+1,0,rqst);
  }

  comm->Barrier();
}

void GetPointsInsideBounds(vtkMPIController* controller,
  vtkPointSet* input,
  vtkPointSet* output,
  const double outterBounds[6])
{
  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(controller->GetCommunicator());

  if (!com)
  {
    return;
  }

  int np = com->GetNumberOfProcesses();
  int rank = com->GetLocalProcessId();

  if (np == 1)
  {
    output->ShallowCopy(input);
    return;
  }

  double localOutterBounds[6];

  // round bounds to the nearest float value because locator use float internally.
  // Otherwise, points that are exactly on the bounds may be wrongly considered as outside
  // because of the cast.
  for (int i = 0; i < 3; i++)
  {
    localOutterBounds[2 * i] =
      std::nextafter((float)outterBounds[2 * i], (float)outterBounds[2 * i] - 1);
    localOutterBounds[2 * i + 1] =
      std::nextafter((float)outterBounds[2 * i + 1], (float)outterBounds[2 * i + 1] + 1);
  }

  bool emptyData = input->GetNumberOfPoints() == 0;

  std::vector<double> allOutterBounds(np * 6, 0);
  com->AllGather(localOutterBounds, allOutterBounds.data(), 6);

  // size in bytes of messages to be sent to other processes
  std::vector<int> messagesSize(np, 0);

  // number of points in messages to be sent to other processes
  std::vector<int> messagePointCount(np, 0);

  // array of point ids
  vtkNew<vtkIdTypeArray> idArray;
  std::vector<vtkSmartPointer<vtkCharArray> > dataToSend;
  dataToSend.resize(np);

  // we will need a locator to search points inside each processor assigned regions
  vtkNew<vtkOctreePointLocator> locator;

  if (!emptyData)
  {
    vtkNew<vtkPolyData> inputPolyData;
    inputPolyData->SetPoints(input->GetPoints());
    locator->SetDataSet(inputPolyData.Get());
    locator->BuildLocator();
  }

  // 1st step: define messages to send to each processor (including itself)
  // with polydata containing closest points to local data bounding box
  for (int partner = 0; partner < np; partner++)
  {
    idArray->SetNumberOfTuples(0);
    vtkIdType nPoints = 0;
    vtkIdType* ids = nullptr;
    if (!emptyData)
    {
      double* nbounds = &allOutterBounds[partner * 6];
      locator->FindPointsInArea(nbounds, idArray.Get());
      nPoints = idArray->GetNumberOfTuples();
      ids = idArray->GetPointer(0);
    }

    vtkNew<vtkPolyData> pointCloudToSend;
    vtkNew<vtkPoints> pointsToSend;
    pointsToSend->SetNumberOfPoints(nPoints);

    vtkPointData* pointsToSendPointData = pointCloudToSend->GetPointData();
    pointsToSendPointData->CopyAllocate(input->GetPointData(), nPoints);

    for (vtkIdType i = 0; i < nPoints; i++)
    {
      pointsToSend->SetPoint(i, input->GetPoint(ids[i]));
      pointsToSendPointData->CopyData(input->GetPointData(), ids[i], i);
    }
    pointCloudToSend->SetPoints(pointsToSend.Get());

    // flatten point data to byte array
    messagePointCount[partner] = nPoints;
    dataToSend[partner] = vtkSmartPointer<vtkCharArray>::New();
    vtkCommunicator::MarshalDataObject(pointCloudToSend.Get(), dataToSend[partner]);
    messagesSize[partner] = dataToSend[partner]->GetNumberOfTuples();
  }

  std::vector<vtkSmartPointer<vtkCharArray> > dataToReceive;
  dataToReceive.resize(np);

  std::vector<vtkMPICommunicator::Request> receiveRequests;
  receiveRequests.resize(np);

  // Calculate size of messages to receive
  std::vector<int> receiveSize(np, 0);
  std::vector<int> receivePointCount(np, 0);

  for (int i = 0; i < np; i++)
  {
    com->Gather(messagesSize.data() + i, receiveSize.data(), 1, i);
    com->Gather(messagePointCount.data() + i, receivePointCount.data(), 1, i);
  }

  // Starting asynchronous receptions
  int nReceive = 0;
  vtkIdType totalPointsToReceive = 0;
  for (int round = 0; round < np - 1; round++)
  {
    int partner = (rank + round + 1) % np;
    if (receiveSize[partner] > 0)
    {
      ++nReceive;
      dataToReceive[partner] = vtkSmartPointer<vtkCharArray>::New();
      com->NoBlockReceive(dataToReceive[partner]->WritePointer(0, receiveSize[partner]),
        receiveSize[partner],
        partner,
        0,
        receiveRequests[partner]);
      totalPointsToReceive += receivePointCount[partner];
    }
  }

  // local sending/receipt is just a pointer assignment
  dataToReceive[rank] = dataToSend[rank];
  dataToSend[rank] = nullptr;
  if (receiveSize[rank] > 0)
  {
    ++nReceive;
    totalPointsToReceive += receivePointCount[rank];
  }

  // Starting asynchronous sends
  std::vector<vtkMPICommunicator::Request> sendRequests;
  sendRequests.resize(np);
  for (int round = 0; round < np - 1; round++)
  {
    int partner = (rank + round + 1) % np;
    if (messagesSize[partner] > 0)
    {
      com->NoBlockSend(dataToSend[partner]->GetPointer(0),
        messagesSize[partner],
        partner,
        0,
        sendRequests[partner]);
    }
  }

  // sum of received points from the different processors
  vtkIdType totalPoints = 0;
  vtkPointData* outputPointData = output->GetPointData();
  outputPointData->SetNumberOfTuples(totalPointsToReceive);

  while (nReceive > 0)
  {
    for (int round = 0; round < np; round++)
    {
      int partner = (rank + round) % np;
      if ((partner == rank || receiveRequests[partner].Test() == 1) && receiveSize[partner] > 0)
      {
        vtkNew<vtkPolyData> receivedPointCloud;
        vtkCommunicator::UnMarshalDataObject(dataToReceive[partner], receivedPointCloud.Get());

        dataToReceive[partner] = nullptr;
        vtkIdType nbReceivedPoints = receivedPointCloud->GetNumberOfPoints();
        vtkPointData* receivedPointData = receivedPointCloud->GetPointData();
        vtkPoints* receivedPoints = receivedPointCloud->GetPoints();
        vtkPoints* outputPoints = output->GetPoints();
        if (!outputPoints)
        {
          vtkNew<vtkPoints> points;
          outputPoints = points.Get();
          output->SetPoints(outputPoints);
        }
        for (vtkIdType i = 0; i < nbReceivedPoints; i++)
        {
          outputPoints->InsertNextPoint(receivedPoints->GetPoint(i));
        }
        int nbArray = receivedPointData->GetNumberOfArrays();

        for (int a = 0; a < nbArray; a++)
        {
          vtkDataArray* fromArray = receivedPointData->GetArray(a);
          if (fromArray)
          {
            vtkDataArray* toArray = outputPointData->GetArray(fromArray->GetName());
            if (!toArray)
            {
              toArray = fromArray->NewInstance();
              toArray->SetName(fromArray->GetName());
              toArray->SetNumberOfComponents(fromArray->GetNumberOfComponents());
              toArray->SetNumberOfTuples(totalPointsToReceive);
              outputPointData->AddArray(toArray);
              toArray->Delete();
            }

            for (vtkIdType i = 0; i < nbReceivedPoints; i++)
            {
              toArray->SetTuple(totalPoints + i, fromArray->GetTuple(i));
            }
          }
        }
        totalPoints += nbReceivedPoints;
        --nReceive;
        receiveSize[partner] = 0;
      }
    }
  }

  // we wait for sent messages to be received before deleting them
  for (int round = 0; round < np - 1; round++)
  {
    int partner = (rank + round + 1) % np;
    if (messagesSize[partner] > 0)
    {
      sendRequests[partner].Wait();
    }
  }
}

} // END namespace vtkMPIUtilities
