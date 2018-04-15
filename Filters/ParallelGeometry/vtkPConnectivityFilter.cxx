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

#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkKdTreePointLocator.h"
#include "vtkMath.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkThreshold.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkWeakPointer.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <vector>

class vtkPConnectivityFilter::vtkInternals
{
public:
  /**
   * MPI controller for ranks with data.
   */
  vtkWeakPointer<vtkMPIController> SubController;

  /**
   * Output from the results of the local connectivity operation.
   */
  vtkWeakPointer<vtkPointSet> Output;


  vtkInternals() {}
  ~vtkInternals()
  {
  }

  /**
   * Have all ranks with data exchange their bounding boxes.
   */
  void ExchangeBounds(std::vector< double > & allBounds)
  {
    //Inflate the bounds a bit to deal with floating point precision.
    double bounds[6];
    this->Output->GetBounds(bounds);
    vtkBoundingBox myBoundingBox(bounds);
    myBoundingBox.Inflate();
    myBoundingBox.GetBounds(bounds);
    allBounds.resize(6*this->SubController->GetNumberOfProcesses());
    this->SubController->AllGather(bounds, &allBounds[0], 6);
  }

  /**
   * Determine this rank's neighbors from the bounding box information.
   */
  void FindMyNeighbors(const std::vector<double> & allBounds, std::vector<int> & myNeighbors)
  {
    myNeighbors.clear();

    double bounds[6];
    this->Output->GetBounds(bounds);
    vtkBoundingBox myBoundingBox(bounds);
    myBoundingBox.Inflate();
    myBoundingBox.GetBounds(bounds);

    // Identify neighboring ranks.
    int myRank = this->SubController->GetLocalProcessId();
    for (int p = 0; p < this->SubController->GetNumberOfProcesses(); ++p)
    {
      if (p == myRank)
      {
        continue;
      }

      vtkBoundingBox potentialNeighborBoundingBox(&allBounds[6*p]);
      if (myBoundingBox.Intersects(potentialNeighborBoundingBox))
      {
        myNeighbors.push_back(p);
      }
    }
  }

  /**
   * Get points and region ids to send to neighbors.
   */
  void GatherPointsAndRegionIds(const std::vector< double > & allBounds,
                                const std::vector< int > & regionStarts,
                                std::map< int, std::vector< double > > & pointsForMyNeighbors,
                                std::map< int, std::vector< vtkIdType > > & regionIdsForMyNeighbors)
  {
    // For all neighbors, gather up points and region IDs that they will
    // potentially need. These are local points that fall within the bounding
    // box of the neighbors.
    int myRank = this->SubController->GetLocalProcessId();
    vtkPoints* outputPoints = this->Output->GetPoints();
    vtkPointData* outputPD = this->Output->GetPointData();
    vtkIdTypeArray* pointRegionIds =
    vtkIdTypeArray::SafeDownCast(outputPD->GetArray("RegionId"));
    for (int p = 0; p < this->SubController->GetNumberOfProcesses(); ++p)
    {
      pointsForMyNeighbors[p] = std::vector<double>();
      regionIdsForMyNeighbors[p] = std::vector<vtkIdType>();
      vtkBoundingBox neighborBB(&allBounds[6*p]);
      for (vtkIdType id = 0; id < this->Output->GetNumberOfPoints(); ++id)
      {
        double pt[3];
        outputPoints->GetPoint(id, pt);

        if (neighborBB.ContainsPoint(pt))
        {
          auto & sendPoints = pointsForMyNeighbors[p];
          sendPoints.insert(sendPoints.end(), pt, pt+3);

          vtkIdType regionId = pointRegionIds->GetTypedComponent(id, 0);
          auto & sendRegionIds = regionIdsForMyNeighbors[p];
          sendRegionIds.push_back(regionId + regionStarts[myRank]);
        }
      }
    }
  }

  /**
   * Exchange number of points going to each neighbor.
   */
  void ExchangeNumberOfPointsToSend(const std::vector< int > & myNeighbors,
                                    const std::map< int, std::vector< vtkIdType > > & regionIdsForMyNeighbors,
                                    std::map< int, int > & sendLengths,
                                    std::map< int, int > & recvLengths)
  {
    const int PCF_SIZE_EXCHANGE_TAG = 194727;
    recvLengths.clear();

    std::vector< vtkMPICommunicator::Request > recvRequests(myNeighbors.size());

    int requestIdx = 0;
    for (auto nbrIter = myNeighbors.begin(); nbrIter != myNeighbors.end(); ++nbrIter)
    {
      int fromRank = *nbrIter;
      this->SubController->NoBlockReceive(&recvLengths[fromRank], 1, fromRank,
                                 PCF_SIZE_EXCHANGE_TAG, recvRequests[requestIdx++]);
    }

    std::map< int, vtkMPICommunicator::Request > sendRequests;

    // Send number of points neighbors should expect to receive.
    for (auto nbrIter = myNeighbors.begin(); nbrIter != myNeighbors.end(); ++nbrIter)
    {
      int toRank = *nbrIter;
      sendLengths[toRank] = static_cast<int>(regionIdsForMyNeighbors.at(toRank).size());
      this->SubController->NoBlockSend(&sendLengths[toRank], 1, toRank,
                              PCF_SIZE_EXCHANGE_TAG, sendRequests[toRank]);
    }

    this->SubController->WaitAll(requestIdx, &recvRequests[0]);
  }

  /**
   * Send and receive points to/from neighbors.
   */
  void SendReceivePoints(const std::map< int, int > & sendLengths,
                         const std::map< int, std::vector< double > > pointsForMyNeighbors,
                         const std::map< int, int > & recvLengths,
                         std::map< int, std::vector< double > > & pointsFromMyNeighbors)
  {
    const int PCF_POINTS_TAG = 194728;
    pointsFromMyNeighbors.clear();

    std::map< int, vtkMPICommunicator::Request > sendRequests;
    std::vector< vtkMPICommunicator::Request > recvRequests(recvLengths.size());

    // Receive neighbors' points.
    int requestIdx = 0;
    for (auto iter = recvLengths.begin(); iter != recvLengths.end(); ++iter)
    {
      int fromRank = iter->first;
      int numFromRank = iter->second;
      if (numFromRank > 0)
      {
        pointsFromMyNeighbors[fromRank].resize(3*numFromRank);
        this->SubController->NoBlockReceive(&pointsFromMyNeighbors[fromRank][0],
                                            3*numFromRank, fromRank, PCF_POINTS_TAG,
                                            recvRequests[requestIdx++]);
      }
    }

    // Send points to neighbors
    for (auto nbrIter = sendLengths.begin(); nbrIter != sendLengths.end(); ++nbrIter)
    {
      vtkIdType toRank = nbrIter->first;
      vtkIdType numToRank = nbrIter->second;
      if (numToRank > 0)
      {
        this->SubController->NoBlockSend(&pointsForMyNeighbors.at(toRank)[0],
                                        3*numToRank, toRank,
                                        PCF_POINTS_TAG, sendRequests[toRank]);
      }
    }

    this->SubController->WaitAll(requestIdx, &recvRequests[0]);
  }

  /**
   * Send and receive RegionIds to/from neighbors.
   */
  void SendReceiveRegionIds(const std::map< int, int > & sendLengths,
                            const std::map< int, std::vector< vtkIdType > > & regionIdsForMyNeighbors,
                            const std::map< int, int > & recvLengths,
                            std::map< int, std::vector< vtkIdType > > & regionIdsFromMyNeighbors)
  {
    const int PCF_REGIONIDS_TAG = 194729;
    regionIdsFromMyNeighbors.clear();

    std::map< int, vtkMPICommunicator::Request > sendRequests;
    std::vector< vtkMPICommunicator::Request > recvRequests(recvLengths.size());

    // Receive region point IDs
    int requestIdx = 0;
    for (auto iter = recvLengths.begin(); iter != recvLengths.end(); ++iter)
    {
      int fromRank = iter->first;
      int numFromRank = iter->second;
      if (numFromRank > 0)
      {
        regionIdsFromMyNeighbors[fromRank].resize(numFromRank);
        this->SubController->NoBlockReceive(&regionIdsFromMyNeighbors[fromRank][0],
                                            numFromRank, fromRank, PCF_REGIONIDS_TAG,
                                            recvRequests[requestIdx++]);
      }
    }

    // Send region point IDs
    for (auto nbrIter = sendLengths.begin(); nbrIter != sendLengths.end(); ++nbrIter)
    {
      vtkIdType toRank = nbrIter->first;
      vtkIdType numToRank = nbrIter->second;
      if (numToRank > 0)
      {
        this->SubController->NoBlockSend(&regionIdsForMyNeighbors.at(toRank)[0],
                                         numToRank, toRank, PCF_REGIONIDS_TAG,
                                         sendRequests[toRank]);
      }
    }

    this->SubController->WaitAll(requestIdx, &recvRequests[0]);
  }
};

vtkStandardNewMacro(vtkPConnectivityFilter);

vtkPConnectivityFilter::vtkPConnectivityFilter()
{
  this->Internals = new vtkInternals;
}

vtkPConnectivityFilter::~vtkPConnectivityFilter()
{
  delete this->Internals;
}

int vtkPConnectivityFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_REGIONS ||
      this->ExtractionMode == VTK_EXTRACT_CELL_SEEDED_REGIONS ||
      this->ExtractionMode == VTK_EXTRACT_SPECIFIED_REGIONS)
  {
    vtkErrorMacro("ExtractionMode " << this->GetExtractionModeAsString()
      << " is not supported in " << this->GetClassName());
    return 1;
  }

  // Get the input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataSet* input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkMultiProcessController* globalController =
    vtkMultiProcessController::GetGlobalController();

  // Check how many ranks have data. If it is only one, running the superclass
  // RequestData is sufficient as no global data exchange and RegionId
  // relabeling is needed. It is worth checking to avoid issuing an
  // unnecessary warning when a dataset resides entirely on one process.
  int hasCells = input->GetNumberOfCells() > 0 ? 1 : 0;
  int ranksWithCells = 0;
  globalController->AllReduce(&hasCells, &ranksWithCells, 1, vtkCommunicator::SUM_OP);

  int numRanks = globalController->GetNumberOfProcesses();
  int myRank = globalController->GetLocalProcessId();

  // Compute local connectivity. If we are running in parallel, we need the full
  // connectivity first, and will handle the extraction mode later.
  int success = 1;
  if (numRanks > 1 && ranksWithCells > 1)
  {
    int saveScalarConnectivity = this->ScalarConnectivity;
    int saveExtractionMode = this->ExtractionMode;
    int saveColorRegions = this->ColorRegions;

    // Overwrite custom member variables temporarily.
    this->ScalarConnectivity = 0;
    this->ExtractionMode = VTK_EXTRACT_ALL_REGIONS;
    this->ColorRegions = 1;

    // Invoke the connectivity algorithm in the superclass.
    success = this->Superclass::RequestData(request, inputVector, outputVector);

    this->ScalarConnectivity = saveScalarConnectivity;
    this->ExtractionMode = saveExtractionMode;
    this->ColorRegions = saveColorRegions;
  }
  else
  {
    // Only 1 process, just invoke the superclass and return.
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  // Creates a SubController.
  vtkSmartPointer<vtkMPIController> subController;
  subController.TakeReference(
    vtkMPIController::SafeDownCast(globalController)->PartitionController(hasCells, 0));

  this->Internals->SubController = subController;

  // From here on we deal only with the SubController
  numRanks = subController->GetNumberOfProcesses();
  myRank = subController->GetLocalProcessId();

  // Get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the output
  vtkPointSet *output = vtkPointSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->Internals->Output = output;

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
  std::partial_sum(regionCounts.begin(), regionCounts.end(),
    regionStarts.begin() + 1);

  vtkPointData* outputPD = output->GetPointData();
  vtkIdTypeArray* pointRegionIds =
    vtkIdTypeArray::SafeDownCast(outputPD->GetArray("RegionId"));

  // Exchange bounding boxes of the data on each rank. These are used to
  // determine neighboring ranks and to minimize the number of points sent
  // to neighboring processors.
  std::vector<double> allBounds;
  this->Internals->ExchangeBounds(allBounds);

  // Identify neighboring ranks.
  std::vector<int> myNeighbors;
  this->Internals->FindMyNeighbors(allBounds, myNeighbors);

  // Create a map from neighbor to data set boundary points and region IDs
  std::map< int, std::vector< double > > pointsForMyNeighbors;
  std::map< int, std::vector< vtkIdType > > regionIdsForMyNeighbors;
  this->Internals->GatherPointsAndRegionIds(allBounds, regionStarts,
                                            pointsForMyNeighbors,
                                            regionIdsForMyNeighbors);

  std::map< int, int > sendLengths;
  std::map< int, int > recvLengths;
  this->Internals->ExchangeNumberOfPointsToSend(myNeighbors, regionIdsForMyNeighbors,
                                                sendLengths, recvLengths);

  std::map< int, std::vector< double > > pointsFromMyNeighbors;
  this->Internals->SendReceivePoints(sendLengths, pointsForMyNeighbors,
                                     recvLengths, pointsFromMyNeighbors);

  std::map< int, std::vector< vtkIdType > > regionIdsFromMyNeighbors;
  this->Internals->SendReceiveRegionIds(sendLengths, regionIdsForMyNeighbors,
                                        recvLengths, regionIdsFromMyNeighbors);

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  // Links from local region ids to remote region ids. Vector index is local
  // region id, and the set contains linked remote ids.
  typedef std::vector< std::set< vtkIdType > > RegionLinksType;
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
      if (rank == myRank)
      {
        continue;
      }

      for (size_t ptId = 0; ptId < pointsFromMyNeighbors[rank].size()/3; ++ptId)
      {
        double x[3];
        x[0] = pointsFromMyNeighbors[rank][3*ptId + 0];
        x[1] = pointsFromMyNeighbors[rank][3*ptId + 1];
        x[2] = pointsFromMyNeighbors[rank][3*ptId + 2];

        vtkIdType localId = localPointLocator->FindClosestPoint(x);
        // Skip local ghost points as we do not need ghost-ghost links.
        vtkUnsignedCharArray* pointGhostArray = output->GetPointGhostArray();
        if (pointGhostArray && pointGhostArray->GetTypedComponent(localId, 0) &
            vtkDataSetAttributes::DUPLICATEPOINT)
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
        vtkIdType localRegionId = localRegionIds->GetTypedComponent(localId, 0) +
          regionStarts[myRank];

        vtkIdType remoteRegionId = regionIdsFromMyNeighbors[rank][ptId];

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
  std::vector< vtkIdType > localLinks;
  for (size_t i = 0; i < links.size(); ++i)
  {
    for (std::set< vtkIdType >::iterator iter = std::begin(links[i]);
         iter != std::end(links[i]); ++iter)
    {
      localLinks.push_back(static_cast<vtkIdType>(i));
      localLinks.push_back(*iter);
    }
  }

  // Gather all the links on each rank. This is possibly suboptimal, but it
  // avoids needing a connected components algorithm on a distributed graph.
  vtkIdType localNumLinks = static_cast<vtkIdType>(localLinks.size());
  std::vector< vtkIdType > linkCounts(numRanks, -1);
  std::vector< vtkIdType > linkStarts(numRanks + 1, 0);
  subController->AllGather(&localNumLinks, &linkCounts[0], 1);

  // Compute starting region IDs on each rank
  for (int i = 0; i < numRanks; ++i)
  {
    linkStarts[i + 1] = linkCounts[i] + linkStarts[i];
  }

  std::vector< vtkIdType > allLinks(linkStarts[numRanks]);

  subController->AllGatherV(&localLinks[0], &allLinks[0],
                         static_cast<vtkIdType>(localLinks.size()),
                         &linkCounts[0], &linkStarts[0]);

  // Set up a graph of all the region-to-region links.
  typedef struct _RegionNode {
    // Stored for relabeling step
    vtkIdType OriginalRegionId;

    // Current local region id
    vtkIdType CurrentRegionId;

    std::vector< vtkIdType > Links;
  } RegionNode;

  size_t linkIdx = 0;
  std::vector< RegionNode > regionNodes(regionStarts[numRanks]);
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
    for (std::vector< RegionNode >::iterator nodeIter = regionNodes.begin();
         nodeIter != regionNodes.end(); ++nodeIter)
    {
      for (std::vector< vtkIdType >::iterator linkIter = nodeIter->Links.begin();
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
  std::set< vtkIdType > currentRegionIds;
  for (std::vector< RegionNode >::iterator nodeIter = regionNodes.begin();
       nodeIter != regionNodes.end(); ++nodeIter)
  {
    currentRegionIds.insert(nodeIter->CurrentRegionId);
  }

  // Create a map from current region id after relabeling to a new, contiguous
  // label. Maps current region id -> relabeled array.
  std::map< vtkIdType, vtkIdType > relabeledRegionMap;
  vtkIdType contiguousLabel = 0;
  for (std::set< vtkIdType >::iterator setIter = currentRegionIds.begin();
       setIter != currentRegionIds.end(); ++setIter)
  {
    relabeledRegionMap[*setIter] = contiguousLabel++;
  }

  // Now do the relabing to the contiguous region id.
  std::vector< vtkIdType > regionIdMap(regionNodes.size(), -1);
  for (std::vector< RegionNode >::iterator nodeIter = regionNodes.begin();
       nodeIter != regionNodes.end(); ++nodeIter)
  {
    nodeIter->CurrentRegionId = relabeledRegionMap[nodeIter->CurrentRegionId];
  }

  // Relabel the points and cells according to the contiguous renumbering.
  vtkCellData* outputCD = output->GetCellData();
  vtkIdTypeArray* cellRegionIds =
    vtkIdTypeArray::SafeDownCast(outputCD->GetArray("RegionId"));
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
  std::vector< vtkIdType > localRegionSizes(numContiguousLabels, 0);
  if (cellRegionIds)
  {
    // Iterate over cells and count how many are in different regions.
    for (vtkIdType i = 0; i < cellRegionIds->GetNumberOfValues(); ++i)
    {
      localRegionSizes[cellRegionIds->GetValue(i)]++;
    }
  }

  // AllReduce to sum up the number of cells in each region on each process.
  std::vector< vtkIdType > globalRegionSizes(numContiguousLabels, 0);
  subController->AllReduce(&localRegionSizes[0], &globalRegionSizes[0],
    numContiguousLabels, vtkCommunicator::SUM_OP);

  // Store the region sizes
  this->RegionSizes->Reset();
  this->RegionSizes->SetNumberOfComponents(1);
  this->RegionSizes->SetNumberOfTuples(numContiguousLabels);
  for (vtkIdType i = 0; i < numContiguousLabels; ++i)
  {
    this->RegionSizes->SetTypedTuple(i, &globalRegionSizes[i]);
  }

  if (this->ExtractionMode == VTK_EXTRACT_LARGEST_REGION ||
      this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_REGION)
  {
    double threshold = 0.0;
    if (this->ExtractionMode == VTK_EXTRACT_LARGEST_REGION)
    {
      vtkIdType largestRegionId =
        std::distance(globalRegionSizes.begin(),
          std::max_element(globalRegionSizes.begin(), globalRegionSizes.end()));
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
      subController->AllReduce(&minDist2, &globalMinDist2, 1,
        vtkCommunicator::MIN_OP);

      int minDist2Rank = 0;
      vtkIdType minDist2Region = 0;
      if (fabs(minDist2 - globalMinDist2) < 1e-9)
      {
        minDist2Rank = myRank;
        minDist2Region = pointRegionIds->GetValue(minId);
      }

      // Broadcast the rank of who has the minimum distance
      int globalMinDist2Rank = 0;
      subController->AllReduce(&minDist2Rank, &globalMinDist2Rank, 1,
        vtkCommunicator::MAX_OP);

      // Get the id of the region nearest the point and use that in the
      // threshold filter below.
      subController->Broadcast(&minDist2Region, 1, globalMinDist2Rank);
      threshold = minDist2Region;
    }

    // Now extract only the cells that have the desired id.
    vtkNew<vtkThreshold> thresholder;
    thresholder->SetInputData(output);
    thresholder->ThresholdBetween(threshold, threshold);
    thresholder->SetInputArrayToProcess(0, 0, 0,
      vtkDataObject::FIELD_ASSOCIATION_CELLS, "RegionId");
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
  this->Superclass::PrintSelf(os,indent);
}
