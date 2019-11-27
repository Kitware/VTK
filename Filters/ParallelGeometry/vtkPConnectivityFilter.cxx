/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPConnectivityFilter.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkArrayDispatch.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkKdTreePointLocator.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkThreshold.h"
#include "vtkTypeList.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkWeakPointer.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <vector>

#include "vtkDoubleArray.h"

namespace
{

typedef vtkTypeList::Unique<
  vtkTypeList::Create<vtkAOSDataArrayTemplate<int>, vtkAOSDataArrayTemplate<unsigned long>,
    vtkAOSDataArrayTemplate<char>, vtkAOSDataArrayTemplate<unsigned char>,
    vtkAOSDataArrayTemplate<float>, vtkAOSDataArrayTemplate<double> > >::Result PointArrayTypes;

struct WorkerBase
{
  WorkerBase(vtkMPIController* subController)
    : SubController(subController)
  {
  }

  /**
   * MPI controller for ranks with data.
   */
  vtkWeakPointer<vtkMPIController> SubController;
};

/**
 * Worker for all ranks with data to exchange their bounding boxes.
 */
struct ExchangeBoundsWorker : public WorkerBase
{
  ExchangeBoundsWorker(vtkMPIController* subController)
    : WorkerBase(subController)
  {
    memset(this->Bounds, 0, sizeof(this->Bounds));
  }

  bool Execute(const double bounds[6], const vtkSmartPointer<vtkDataArray>& allBoundsArray)
  {
    memcpy(this->Bounds, bounds, 6 * sizeof(double));
    this->AllBoundsArray = allBoundsArray;

    using Dispatcher = vtkArrayDispatch::DispatchByArray<PointArrayTypes>;
    return Dispatcher::Execute(this->AllBoundsArray, *this);
  }

  template <class TArray>
  void operator()(TArray* allBounds)
  {
    // Inflate the bounds a bit to deal with floating point precision.
    double bounds[6];
    vtkBoundingBox myBoundingBox(this->Bounds);
    myBoundingBox.Inflate();
    myBoundingBox.GetBounds(bounds);

    typename TArray::ValueType typedBounds[6];
    for (int i = 0; i < 6; ++i)
    {
      typedBounds[i] = static_cast<typename TArray::ValueType>(bounds[i]);
    }

    allBounds->SetNumberOfComponents(6);
    allBounds->SetNumberOfTuples(this->SubController->GetNumberOfProcesses());
    this->SubController->AllGather(
      typedBounds, reinterpret_cast<typename TArray::ValueType*>(allBounds->GetVoidPointer(0)), 6);
  }

protected:
  // Input - Local data bounds
  double Bounds[6];

  // Output - Bounds on all ranks
  vtkWeakPointer<vtkDataArray> AllBoundsArray;
};

/**
 * Determine this rank's neighbors from the bounding box information.
 */
struct FindMyNeighborsWorker : public WorkerBase
{
  FindMyNeighborsWorker(vtkMPIController* subController)
    : WorkerBase(subController)
    , MyNeighbors(nullptr)
  {
    memset(this->Bounds, 0, sizeof(this->Bounds));
  }

  bool Execute(const double bounds[6], const vtkSmartPointer<vtkDataArray>& allBoundsArray,
    std::vector<int>& myNeighbors)
  {
    memcpy(this->Bounds, bounds, 6 * sizeof(double));
    this->AllBoundsArray = allBoundsArray;

    // Output
    this->MyNeighbors = &myNeighbors;

    this->DoExecute();
    return true;
  }

  void DoExecute()
  {
    this->MyNeighbors->clear();

    vtkBoundingBox myBoundingBox(this->Bounds);
    myBoundingBox.Inflate();
    double bounds[6];
    myBoundingBox.GetBounds(bounds);

    // Identify neighboring ranks.
    int myRank = this->SubController->GetLocalProcessId();
    for (int p = 0; p < this->SubController->GetNumberOfProcesses(); ++p)
    {
      if (p == myRank)
      {
        continue;
      }

      double potentialNeighborBounds[6];
      this->AllBoundsArray->GetTuple(p, potentialNeighborBounds);

      vtkBoundingBox potentialNeighborBoundingBox(potentialNeighborBounds);
      if (myBoundingBox.Intersects(potentialNeighborBoundingBox))
      {
        this->MyNeighbors->push_back(p);
      }
    }
  }

protected:
  // Input - Local data bounds
  double Bounds[6];

  // Input - Bounds on all ranks
  vtkWeakPointer<vtkDataArray> AllBoundsArray;

  // Output - List of this rank's neighbors.
  std::vector<int>* MyNeighbors;
};

/**
 * Worker to gather up points and region ids to send to neighbors.
 */
struct AssemblePointsAndRegionIdsWorker : public WorkerBase
{
  AssemblePointsAndRegionIdsWorker(vtkMPIController* subController)
    : WorkerBase(subController)
    , RegionStarts(nullptr)
    , PointsForMyNeighbors(nullptr)
    , RegionIdsForMyNeighbors(nullptr)
  {
  }

  bool Execute(const std::vector<int>& regionStarts,
    const vtkSmartPointer<vtkDataArray>& allBoundsArray,
    const vtkSmartPointer<vtkPointSet>& localResult,
    std::map<int, vtkSmartPointer<vtkDataArray> >& pointsForMyNeighbors,
    std::map<int, vtkSmartPointer<vtkIdTypeArray> >& regionIdsForMyNeighbors)
  {
    this->RegionStarts = &regionStarts;
    this->LocalResult = localResult;

    // Output
    this->PointsForMyNeighbors = &pointsForMyNeighbors;
    this->RegionIdsForMyNeighbors = &regionIdsForMyNeighbors;

    using Dispatcher = vtkArrayDispatch::DispatchByArray<PointArrayTypes>;
    return Dispatcher::Execute(allBoundsArray, *this);
  }

  template <class TArray>
  void operator()(TArray* allBounds)
  {
    // For all neighbors, gather up points and region IDs that they will
    // potentially need. These are local points that fall within the bounding
    // box of the neighbors.
    this->PointsForMyNeighbors->clear();
    this->RegionIdsForMyNeighbors->clear();

    int myRank = this->SubController->GetLocalProcessId();
    vtkPoints* outputPoints = this->LocalResult->GetPoints();
    TArray* pointArray = TArray::SafeDownCast(outputPoints->GetData());
    vtkPointData* outputPD = this->LocalResult->GetPointData();
    vtkIdTypeArray* pointRegionIds = vtkIdTypeArray::SafeDownCast(outputPD->GetArray("RegionId"));
    for (int p = 0; p < this->SubController->GetNumberOfProcesses(); ++p)
    {
      if (myRank == p)
      {
        continue;
      }

      TArray* typedPointsForMyNeighbor = TArray::New();
      typedPointsForMyNeighbor->SetNumberOfComponents(3);
      (*this->PointsForMyNeighbors)[p].TakeReference(typedPointsForMyNeighbor);
      (*this->RegionIdsForMyNeighbors)[p] = vtkSmartPointer<vtkIdTypeArray>::New();

      typename TArray::ValueType bb[6];
      allBounds->GetTypedTuple(p, bb);

      vtkBoundingBox neighborBB(bb[0], bb[1], bb[2], bb[3], bb[4], bb[5]);
      for (vtkIdType id = 0; id < this->LocalResult->GetNumberOfPoints(); ++id)
      {
        typename TArray::ValueType pt[3];
        pointArray->GetTypedTuple(id, pt);
        double doublePt[3] = { static_cast<double>(pt[0]), static_cast<double>(pt[1]),
          static_cast<double>(pt[2]) };

        if (neighborBB.ContainsPoint(doublePt))
        {
          typedPointsForMyNeighbor->InsertNextTypedTuple(pt);

          vtkIdType regionId =
            pointRegionIds->GetTypedComponent(id, 0) + (*this->RegionStarts)[myRank];
          (*this->RegionIdsForMyNeighbors)[p]->InsertNextTypedTuple(&regionId);
        }
      }
    }
  }

protected:
  // Input - Starting index of the first region on each rank.
  const std::vector<int>* RegionStarts;

  // Input - Output from the local connectivity operation.
  vtkWeakPointer<vtkPointSet> LocalResult;

  // Output
  std::map<int, vtkSmartPointer<vtkDataArray> >* PointsForMyNeighbors;

  // Output
  std::map<int, vtkSmartPointer<vtkIdTypeArray> >* RegionIdsForMyNeighbors;
};

/**
 * Send and receive points to/from neighbors.
 */
struct SendReceivePointsWorker : public WorkerBase
{

  SendReceivePointsWorker(vtkMPIController* subController)
    : WorkerBase(subController)
    , PointsFromMyNeighbors(nullptr)
    , RegionIdsFromMyNeighbors(nullptr)
  {
  }

  bool Execute(const vtkSmartPointer<vtkDataArray>& allBoundsArray,
    const std::map<int, int>& sendLengths, const std::map<int, int>& recvLengths,
    const std::map<int, vtkSmartPointer<vtkDataArray> >& pointsForMyNeighbors,
    const std::map<int, vtkSmartPointer<vtkIdTypeArray> >& regionIdsForMyNeighbors,
    std::map<int, vtkSmartPointer<vtkDataArray> >& pointsFromMyNeighbors,
    std::map<int, vtkSmartPointer<vtkIdTypeArray> >& regionIdsFromMyNeighbors)
  {
    this->SendLengths = sendLengths;
    this->RecvLengths = recvLengths;
    this->PointsForMyNeighbors = pointsForMyNeighbors;
    this->RegionIdsForMyNeighbors = regionIdsForMyNeighbors;

    // Output
    this->PointsFromMyNeighbors = &pointsFromMyNeighbors;
    this->RegionIdsFromMyNeighbors = &regionIdsFromMyNeighbors;

    using Dispatcher = vtkArrayDispatch::DispatchByArray<PointArrayTypes>;
    return Dispatcher::Execute(allBoundsArray, *this);
  }

  template <class TArray>
  void operator()(TArray* vtkNotUsed(array))
  {
    const int PCF_POINTS_TAG = 194728;
    const int PCF_REGIONIDS_TAG = 194729;
    this->PointsFromMyNeighbors->clear();
    this->RegionIdsFromMyNeighbors->clear();

    std::map<int, vtkMPICommunicator::Request> sendRequestsPoints;
    std::map<int, vtkMPICommunicator::Request> sendRequestsRegionIds;
    std::vector<vtkMPICommunicator::Request> recvRequestsPoints(this->RecvLengths.size());
    std::vector<vtkMPICommunicator::Request> recvRequestsRegionIds(this->RecvLengths.size());

    // Receive neighbors' points.
    int requestIdx = 0;
    for (auto iter = this->RecvLengths.begin(); iter != this->RecvLengths.end(); ++iter)
    {
      int fromRank = iter->first;
      int numFromRank = iter->second;
      if (numFromRank > 0)
      {
        TArray* pfmn = TArray::New();
        pfmn->SetNumberOfComponents(3);
        pfmn->SetNumberOfTuples(numFromRank);
        (*this->PointsFromMyNeighbors)[fromRank].TakeReference(pfmn);
        this->SubController->NoBlockReceive(pfmn->GetPointer(0), 3 * numFromRank, fromRank,
          PCF_POINTS_TAG, recvRequestsPoints[requestIdx]);

        vtkIdTypeArray* idArray = vtkIdTypeArray::New();
        idArray->SetNumberOfComponents(1);
        idArray->SetNumberOfTuples(numFromRank);
        (*this->RegionIdsFromMyNeighbors)[fromRank].TakeReference(idArray);
        this->SubController->NoBlockReceive(idArray->GetPointer(0), numFromRank, fromRank,
          PCF_REGIONIDS_TAG, recvRequestsRegionIds[requestIdx++]);
      }
    }

    // Send points to neighbors
    for (auto nbrIter = this->SendLengths.begin(); nbrIter != this->SendLengths.end(); ++nbrIter)
    {
      vtkIdType toRank = nbrIter->first;
      vtkIdType numToRank = nbrIter->second;
      if (numToRank > 0)
      {
        TArray* pfmn = TArray::SafeDownCast(this->PointsForMyNeighbors.at(toRank));
        this->SubController->NoBlockSend(
          pfmn->GetPointer(0), 3 * numToRank, toRank, PCF_POINTS_TAG, sendRequestsPoints[toRank]);

        vtkIdTypeArray* idArray = this->RegionIdsForMyNeighbors.at(toRank);
        this->SubController->NoBlockSend(idArray->GetPointer(0), numToRank, toRank,
          PCF_REGIONIDS_TAG, sendRequestsRegionIds[toRank]);
      }
    }

    this->SubController->WaitAll(requestIdx, &recvRequestsPoints[0]);
    this->SubController->WaitAll(requestIdx, &recvRequestsRegionIds[0]);
  }

protected:
  // Input
  std::map<int, int> SendLengths;
  std::map<int, int> RecvLengths;
  std::map<int, vtkSmartPointer<vtkDataArray> > PointsForMyNeighbors;
  std::map<int, vtkSmartPointer<vtkIdTypeArray> > RegionIdsForMyNeighbors;

  // Output
  std::map<int, vtkSmartPointer<vtkDataArray> >* PointsFromMyNeighbors;
  std::map<int, vtkSmartPointer<vtkIdTypeArray> >* RegionIdsFromMyNeighbors;
};

/**
 * Exchange number of points going to each neighbor. No dispatch is needed for this function.
 */
void ExchangeNumberOfPointsToSend(vtkMPIController* subController,
  const std::vector<int>& myNeighbors,
  const std::map<int, vtkSmartPointer<vtkIdTypeArray> >& regionIdsForMyNeighbors,
  std::map<int, int>& sendLengths, std::map<int, int>& recvLengths)
{
  const int PCF_SIZE_EXCHANGE_TAG = 194727;
  recvLengths.clear();
  std::vector<vtkMPICommunicator::Request> recvRequests(myNeighbors.size());
  int requestIdx = 0;
  for (auto nbrIter = myNeighbors.begin(); nbrIter != myNeighbors.end(); ++nbrIter)
  {
    int fromRank = *nbrIter;
    subController->NoBlockReceive(
      &recvLengths[fromRank], 1, fromRank, PCF_SIZE_EXCHANGE_TAG, recvRequests[requestIdx++]);
  }
  std::map<int, vtkMPICommunicator::Request> sendRequests;
  // Send number of points neighbors should expect to receive.
  for (auto nbrIter = myNeighbors.begin(); nbrIter != myNeighbors.end(); ++nbrIter)
  {
    int toRank = *nbrIter;
    sendLengths[toRank] = static_cast<int>(regionIdsForMyNeighbors.at(toRank)->GetNumberOfValues());
    subController->NoBlockSend(
      &sendLengths[toRank], 1, toRank, PCF_SIZE_EXCHANGE_TAG, sendRequests[toRank]);
  }
  subController->WaitAll(requestIdx, &recvRequests[0]);
}

} // end anonymous namespace

vtkStandardNewMacro(vtkPConnectivityFilter);

vtkPConnectivityFilter::vtkPConnectivityFilter() {}

vtkPConnectivityFilter::~vtkPConnectivityFilter() {}

int vtkPConnectivityFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkMultiProcessController* globalController = vtkMultiProcessController::GetGlobalController();

  // Check how many ranks have data. If it is only one, running the superclass
  // RequestData is sufficient as no global data exchange and RegionId
  // relabeling is needed. It is worth checking to avoid issuing an
  // unnecessary warning when a dataset resides entirely on one process.
  int numRanks = 1;
  int myRank = 0;
  int ranksWithCells = 0;
  int hasCells = input->GetNumberOfCells() > 0 ? 1 : 0;

  if (globalController)
  {
    globalController->AllReduce(&hasCells, &ranksWithCells, 1, vtkCommunicator::SUM_OP);
    numRanks = globalController->GetNumberOfProcesses();
    myRank = globalController->GetLocalProcessId();
  }

  // Compute local connectivity. If we are running in parallel, we need the full
  // connectivity first, and will handle the extraction mode later.
  int success = 1;
  if (numRanks > 1 && ranksWithCells > 1)
  {
    if (this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_REGIONS ||
      this->ExtractionMode == VTK_EXTRACT_CELL_SEEDED_REGIONS ||
      this->ExtractionMode == VTK_EXTRACT_SPECIFIED_REGIONS)
    {
      vtkErrorMacro("ExtractionMode " << this->GetExtractionModeAsString()
                                      << " is not supported in " << this->GetClassName()
                                      << " when the number of ranks with data is greater than 1.");
      return 1;
    }

    int saveScalarConnectivity = this->ScalarConnectivity;
    int saveExtractionMode = this->ExtractionMode;
    int saveColorRegions = this->ColorRegions;
    int saveRegionIdAssignmentMode = this->RegionIdAssignmentMode;

    // Overwrite custom member variables temporarily.
    this->ScalarConnectivity = 0;
    this->ExtractionMode = VTK_EXTRACT_ALL_REGIONS;
    this->ColorRegions = 1;
    this->RegionIdAssignmentMode = UNSPECIFIED;

    // Invoke the connectivity algorithm in the superclass.
    success = this->Superclass::RequestData(request, inputVector, outputVector);

    this->ScalarConnectivity = saveScalarConnectivity;
    this->ExtractionMode = saveExtractionMode;
    this->ColorRegions = saveColorRegions;
    this->RegionIdAssignmentMode = saveRegionIdAssignmentMode;
  }
  else
  {
    // Only 1 process, just invoke the superclass and return.
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  // Create a SubController.
  vtkSmartPointer<vtkMPIController> subController;
  subController.TakeReference(
    vtkMPIController::SafeDownCast(globalController)->PartitionController(hasCells, 0));

  // From here on we deal only with the SubController
  numRanks = subController->GetNumberOfProcesses();
  myRank = subController->GetLocalProcessId();

  // Get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the output
  vtkPointSet* output = vtkPointSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output->GetPoints())
  {
    vtkErrorMacro("No points in data set");
    success = 0;
  }

  // Check that all ranks succeeded in local connectivity.
  int globalSuccess = 0;
  subController->AllReduce(&success, &globalSuccess, 1, vtkCommunicator::MIN_OP);

  if (!globalSuccess)
  {
    vtkErrorMacro("An error occurred on at least one process.");
    return 0;
  }

  // Exchange number of regions. We assume the RegionIDs are contiguous.
  int numRegions = this->GetNumberOfExtractedRegions();
  std::vector<int> regionCounts(numRanks, 0);
  std::vector<int> regionStarts(numRanks + 1, 0);
  subController->AllGather(&numRegions, &regionCounts[0], 1);

  // Compute starting region Ids on each rank
  std::partial_sum(regionCounts.begin(), regionCounts.end(), regionStarts.begin() + 1);

  vtkPointData* outputPD = output->GetPointData();
  vtkIdTypeArray* pointRegionIds = vtkIdTypeArray::SafeDownCast(outputPD->GetArray("RegionId"));

  // Exchange bounding boxes of the data on each rank. These are used to
  // determine neighboring ranks and to minimize the number of points sent
  // to neighboring processors.
  vtkSmartPointer<vtkDataArray> allBoundsArray;
  allBoundsArray.TakeReference(output->GetPoints()->GetData()->NewInstance());

  ExchangeBoundsWorker exchangeBounds(subController);
  if (!exchangeBounds.Execute(output->GetBounds(), allBoundsArray))
  {
    vtkErrorMacro("Unsupported points array type encountered when exchanging bounds.");
    return 0;
  }

  // Identify neighboring ranks.
  FindMyNeighborsWorker findMyNeighbors(subController);
  std::vector<int> myNeighbors;
  if (!findMyNeighbors.Execute(output->GetBounds(), allBoundsArray, myNeighbors))
  {
    vtkErrorMacro("Unsupported points array type encountered when finding neighbors.");
    return 0;
  }

  AssemblePointsAndRegionIdsWorker assemblePointsAndRegionIds(subController);
  std::map<int, vtkSmartPointer<vtkDataArray> > pointsForMyNeighbors;
  std::map<int, vtkSmartPointer<vtkIdTypeArray> > regionIdsForMyNeighbors;
  if (!assemblePointsAndRegionIds.Execute(
        regionStarts, allBoundsArray, output, pointsForMyNeighbors, regionIdsForMyNeighbors))
  {
    vtkErrorMacro(
      "Unsupported points array type encountered when assembling points and region ids.");
    return 0;
  }

  std::map<int, int> sendLengths;
  std::map<int, int> recvLengths;
  ExchangeNumberOfPointsToSend(
    subController, myNeighbors, regionIdsForMyNeighbors, sendLengths, recvLengths);

  SendReceivePointsWorker sendReceivePoints(subController);
  std::map<int, vtkSmartPointer<vtkDataArray> > pointsFromMyNeighbors;
  std::map<int, vtkSmartPointer<vtkIdTypeArray> > regionIdsFromMyNeighbors;
  if (!sendReceivePoints.Execute(allBoundsArray, sendLengths, recvLengths, pointsForMyNeighbors,
        regionIdsForMyNeighbors, pointsFromMyNeighbors, regionIdsFromMyNeighbors))
  {
    vtkErrorMacro("Unsupported points array type encountered when sending and receiving points.");
    return 0;
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  // Links from local region ids to remote region ids. Vector index is local
  // region id, and the set contains linked remote ids.
  typedef std::vector<std::set<vtkIdType> > RegionLinksType;
  RegionLinksType links(regionStarts[numRanks]);

  if (output->GetNumberOfPoints() > 0)
  {
    // Now resolve the points from our neighbors to local points if possible.
    vtkNew<vtkKdTreePointLocator> localPointLocator;
    localPointLocator->SetDataSet(output);
    localPointLocator->BuildLocator();

    // Map the local and remote ids
    for (int rank = 0; rank < numRanks; ++rank)
    {
      if (rank == myRank || pointsFromMyNeighbors.count(rank) == 0)
      {
        continue;
      }

      for (vtkIdType ptId = 0; ptId < pointsFromMyNeighbors[rank]->GetNumberOfTuples(); ++ptId)
      {
        double x[3];
        pointsFromMyNeighbors[rank]->GetTuple(ptId, x);

        vtkIdType localId = localPointLocator->FindClosestPoint(x);
        // Skip local ghost points as we do not need ghost-ghost links.
        vtkUnsignedCharArray* pointGhostArray = output->GetPointGhostArray();
        if (pointGhostArray &&
          pointGhostArray->GetTypedComponent(localId, 0) & vtkDataSetAttributes::DUPLICATEPOINT)
        {
          continue;
        }
        double y[3];
        output->GetPoints()->GetPoint(localId, y);
        double dist2 = vtkMath::Distance2BetweenPoints(x, y);
        if (dist2 > 1e-6)
        {
          // Nearest point is too far away, move on.
          continue;
        }

        // Save association between local and remote ids
        vtkIdTypeArray* localRegionIds =
          vtkIdTypeArray::SafeDownCast(outputPD->GetArray("RegionId"));
        vtkIdType localRegionId =
          localRegionIds->GetTypedComponent(localId, 0) + regionStarts[myRank];

        vtkIdType remoteRegionId = regionIdsFromMyNeighbors[rank]->GetTypedComponent(ptId, 0);

        if (links[localRegionId].count(remoteRegionId) == 0)
        {
          // Link not already established. Add it here.
          links[localRegionId].insert(remoteRegionId);
        }
      }
    }
  }

  // Set up storage for gathering all links from all processors. This is an
  // interleaved vector containing one regionId and its connected regionId.
  std::vector<vtkIdType> localLinks;
  for (size_t i = 0; i < links.size(); ++i)
  {
    for (std::set<vtkIdType>::iterator iter = std::begin(links[i]); iter != std::end(links[i]);
         ++iter)
    {
      localLinks.push_back(static_cast<vtkIdType>(i));
      localLinks.push_back(*iter);
    }
  }

  // Gather all the links on each rank. This is possibly suboptimal, but it
  // avoids needing a connected components algorithm on a distributed graph.
  vtkIdType localNumLinks = static_cast<vtkIdType>(localLinks.size());
  std::vector<vtkIdType> linkCounts(numRanks, -1);
  std::vector<vtkIdType> linkStarts(numRanks + 1, 0);
  subController->AllGather(&localNumLinks, &linkCounts[0], 1);

  // Compute starting region IDs on each rank
  for (int i = 0; i < numRanks; ++i)
  {
    linkStarts[i + 1] = linkCounts[i] + linkStarts[i];
  }

  std::vector<vtkIdType> allLinks(linkStarts[numRanks]);

  subController->AllGatherV(&localLinks[0], &allLinks[0], static_cast<vtkIdType>(localLinks.size()),
    &linkCounts[0], &linkStarts[0]);

  // Set up a graph of all the region-to-region links.
  typedef struct _RegionNode
  {
    // Stored for relabeling step
    vtkIdType OriginalRegionId;

    // Current local region id
    vtkIdType CurrentRegionId;

    std::vector<vtkIdType> Links;
  } RegionNode;

  size_t linkIdx = 0;
  std::vector<RegionNode> regionNodes(regionStarts[numRanks]);
  for (vtkIdType regionId = 0; regionId < regionStarts[numRanks]; ++regionId)
  {
    regionNodes[regionId].OriginalRegionId = regionId;
    regionNodes[regionId].CurrentRegionId = regionId;

    while (linkIdx < allLinks.size() && allLinks[linkIdx] == regionId)
    {
      regionNodes[regionId].Links.push_back(allLinks[linkIdx + 1]);
      linkIdx += 2;
    }
  }

  // Now run connected components on this graph. The algorithm labels all
  // connected nodes in the graph with the lowest region id in the connected
  // component. This is a breadth-first algorithm. I'm not 100% sure that the
  // multiple passes in the do-while loop are required, but I suspect there may
  // be graph configurations where a single pass is not sufficient for the
  // relabeling to converge.
  bool componentChanged = false;
  do
  {
    componentChanged = false;
    for (std::vector<RegionNode>::iterator nodeIter = regionNodes.begin();
         nodeIter != regionNodes.end(); ++nodeIter)
    {
      for (std::vector<vtkIdType>::iterator linkIter = nodeIter->Links.begin();
           linkIter != nodeIter->Links.end(); ++linkIter)
      {
        vtkIdType linkedRegionId = *linkIter;
        if (nodeIter->CurrentRegionId < regionNodes[linkedRegionId].CurrentRegionId)
        {
          regionNodes[linkedRegionId].CurrentRegionId = nodeIter->CurrentRegionId;
          componentChanged = true;
        }
      }
    }
  } while (componentChanged);

  // Collect all the current ids remaining after the connected components
  // algorithm.
  std::set<vtkIdType> currentRegionIds;
  for (std::vector<RegionNode>::iterator nodeIter = regionNodes.begin();
       nodeIter != regionNodes.end(); ++nodeIter)
  {
    currentRegionIds.insert(nodeIter->CurrentRegionId);
  }

  // Create a map from current region id after relabeling to a new, contiguous
  // label. Maps current region id -> relabeled array.
  std::map<vtkIdType, vtkIdType> relabeledRegionMap;
  vtkIdType contiguousLabel = 0;
  for (std::set<vtkIdType>::iterator setIter = currentRegionIds.begin();
       setIter != currentRegionIds.end(); ++setIter)
  {
    relabeledRegionMap[*setIter] = contiguousLabel++;
  }

  // Now do the relabing to the contiguous region id.
  std::vector<vtkIdType> regionIdMap(regionNodes.size(), -1);
  for (std::vector<RegionNode>::iterator nodeIter = regionNodes.begin();
       nodeIter != regionNodes.end(); ++nodeIter)
  {
    nodeIter->CurrentRegionId = relabeledRegionMap[nodeIter->CurrentRegionId];
  }

  // Relabel the points and cells according to the contiguous renumbering.
  vtkCellData* outputCD = output->GetCellData();
  vtkIdTypeArray* cellRegionIds = vtkIdTypeArray::SafeDownCast(outputCD->GetArray("RegionId"));
  for (vtkIdType i = 0; i < output->GetNumberOfCells(); ++i)
  {
    // Offset the cellRegionId by the starting region id on this rank.
    vtkIdType cellRegionId = cellRegionIds->GetValue(i) + regionStarts[myRank];
    cellRegionIds->SetValue(i, regionNodes[cellRegionId].CurrentRegionId);
  }

  for (vtkIdType i = 0; i < output->GetNumberOfPoints(); ++i)
  {
    // Offset the pointRegionId by the starting region id on this rank.
    vtkIdType pointRegionId = pointRegionIds->GetValue(i) + regionStarts[myRank];
    pointRegionIds->SetValue(i, regionNodes[pointRegionId].CurrentRegionId);
  }

  // Sum up number of cells in each region.
  vtkIdType numContiguousLabels = contiguousLabel;
  std::vector<vtkIdType> localRegionSizes(numContiguousLabels, 0);
  if (cellRegionIds)
  {
    // Iterate over cells and count how many are in different regions. Count only non-ghost cells.
    vtkUnsignedCharArray* cellGhostArray = output->GetCellGhostArray();
    for (vtkIdType i = 0; i < cellRegionIds->GetNumberOfValues(); ++i)
    {
      if (cellGhostArray &&
        cellGhostArray->GetTypedComponent(i, 0) & vtkDataSetAttributes::DUPLICATECELL)
      {
        continue;
      }
      localRegionSizes[cellRegionIds->GetValue(i)]++;
    }
  }

  // AllReduce to sum up the number of cells in each region on each process.
  std::vector<vtkIdType> globalRegionSizes(numContiguousLabels, 0);
  subController->AllReduce(
    &localRegionSizes[0], &globalRegionSizes[0], numContiguousLabels, vtkCommunicator::SUM_OP);

  // Store the region sizes
  this->RegionSizes->Reset();
  this->RegionSizes->SetNumberOfComponents(1);
  this->RegionSizes->SetNumberOfTuples(numContiguousLabels);
  for (vtkIdType i = 0; i < numContiguousLabels; ++i)
  {
    this->RegionSizes->SetTypedTuple(i, &globalRegionSizes[i]);
  }

  // Potentially reorder RegionIds in the output arrays.
  this->OrderRegionIds(pointRegionIds, cellRegionIds);

  if (this->ExtractionMode == VTK_EXTRACT_LARGEST_REGION ||
    this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_REGION)
  {
    double threshold = 0.0;
    if (this->ExtractionMode == VTK_EXTRACT_LARGEST_REGION)
    {
      vtkIdType largestRegionCount = 0;
      vtkIdType largestRegionId = 0;
      for (vtkIdType i = 0; i < this->RegionSizes->GetNumberOfTuples(); ++i)
      {
        vtkIdType candidateCount = this->RegionSizes->GetValue(i);
        if (candidateCount > largestRegionCount)
        {
          largestRegionCount = candidateCount;
          largestRegionId = i;
        }
      }
      threshold = largestRegionId;
    }
    else if (this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_REGION)
    {
      // Find point closest to the desired point
      double minDist2 = VTK_DOUBLE_MAX;
      vtkIdType minId = 0;
      for (vtkIdType i = 0; i < output->GetNumberOfPoints(); ++i)
      {
        double x[3];
        output->GetPoint(i, x);
        double dist2 = vtkMath::Distance2BetweenPoints(x, this->ClosestPoint);
        if (dist2 < minDist2)
        {
          minDist2 = dist2;
          minId = i;
        }
      }

      // AllReduce to find the global minDist2.
      double globalMinDist2 = VTK_DOUBLE_MAX;
      subController->AllReduce(&minDist2, &globalMinDist2, 1, vtkCommunicator::MIN_OP);

      int minDist2Rank = 0;
      vtkIdType minDist2Region = 0;
      if (fabs(minDist2 - globalMinDist2) < 1e-9)
      {
        minDist2Rank = myRank;
        minDist2Region = pointRegionIds->GetValue(minId);
      }

      // Broadcast the rank of who has the minimum distance
      int globalMinDist2Rank = 0;
      subController->AllReduce(&minDist2Rank, &globalMinDist2Rank, 1, vtkCommunicator::MAX_OP);

      // Get the id of the region nearest the point and use that in the
      // threshold filter below.
      subController->Broadcast(&minDist2Region, 1, globalMinDist2Rank);
      threshold = minDist2Region;
    }

    // Now extract only the cells that have the desired id.
    vtkNew<vtkThreshold> thresholder;
    thresholder->SetInputData(output);
    thresholder->ThresholdBetween(threshold, threshold);
    thresholder->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "RegionId");
    thresholder->Update();

    if (output->IsA("vtkPolyData"))
    {
      // It's too bad we have to do this, but vtkThreshold produces
      // vtkUnstructuredGrid output.
      vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
      surfaceFilter->SetInputConnection(thresholder->GetOutputPort());
      surfaceFilter->UseStripsOff();
      surfaceFilter->PassThroughCellIdsOff();
      surfaceFilter->PassThroughPointIdsOff();
      surfaceFilter->Update();
      output->ShallowCopy(surfaceFilter->GetOutput());
    }
    else
    {
      // Output is an unstructured grid
      output->DeepCopy(thresholder->GetOutput());
    }
  }

  if (!this->ColorRegions)
  {
    // No coloring desired. Remove the RegionId arrays.
    outputPD->RemoveArray("RegionId");
    outputCD->RemoveArray("RegionId");
  }

  return 1;
}

void vtkPConnectivityFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
