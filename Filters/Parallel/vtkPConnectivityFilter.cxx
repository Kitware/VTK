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

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkKdTreePointLocator.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkThreshold.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <vector>

vtkStandardNewMacro(vtkPConnectivityFilter);

vtkPConnectivityFilter::vtkPConnectivityFilter()
{
}

vtkPConnectivityFilter::~vtkPConnectivityFilter()
{
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

  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  int numRanks = controller->GetNumberOfProcesses();
  int myRank = controller->GetLocalProcessId();

  // Get the input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkPointSet* input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check how many ranks have data. If it is only one, running the superclass
  // RequestData is sufficient as no global data exchange and RegionId
  // relabeling is needed. It is worth checking to avoid issuing an
  // unnecessary warning when a dataset resides entirely on one process.
  int hasCells = input->GetNumberOfCells() > 0 ? 1 : 0;
  int ranksWithCells = 0;
  controller->AllReduce(&hasCells, &ranksWithCells, 1, vtkCommunicator::SUM_OP);

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

  // Get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the output
  vtkPointSet *output = vtkPointSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check that we have at least one ghost level. If we don't have any ghost
  // points, we say that we have ghost points so other ranks can proceed.
  bool hasGhostPoints = output->GetNumberOfPoints() == 0 ||
    (output->GetNumberOfPoints() > 0 && output->GetPointGhostArray());

  // Check that all ranks succeeded in local connectivity and
  // have ghost cells.
  int globalSuccess = 0;
  controller->AllReduce(&success, &globalSuccess, 1, vtkCommunicator::MIN_OP);

  if (!globalSuccess)
  {
    vtkErrorMacro("An error occurred on at least one process.");
    return 0;
  }

  if (!hasGhostPoints)
  {
    vtkWarningMacro("At least one ghost level is required to run this filter in "
      "parallel, but no ghost cells are available. Results may not be correct.");
  }

  // Exchange number of regions. We assume the RegionIDs are contiguous.
  int numRegions = this->GetNumberOfExtractedRegions();
  std::vector<int> regionCounts(numRanks, 0);
  std::vector<int> regionStarts(numRanks + 1, 0);
  controller->AllGather(&numRegions, &regionCounts[0], 1);

  // Compute starting region Ids on each rank
  std::partial_sum(regionCounts.begin(), regionCounts.end(),
    regionStarts.begin() + 1);

  vtkPoints* outputPoints = output->GetPoints();
  vtkPointData* outputPD = output->GetPointData();
  vtkCellData* outputCD = output->GetCellData();
  vtkIdTypeArray* cellRegionIds =
    vtkIdTypeArray::SafeDownCast(outputCD->GetArray("RegionId"));
  vtkIdTypeArray* pointRegionIds =
    vtkIdTypeArray::SafeDownCast(outputPD->GetArray("RegionId"));

  // Gather up ghost points in the dataset and send them to all the other ranks.
  vtkUnsignedCharArray* pointGhostArray = output->GetPointGhostArray();

  // Extract ghost points and their regions.
  vtkSmartPointer<vtkDataArray> ghostPoints;
  ghostPoints.TakeReference(outputPoints->GetData()->NewInstance());
  ghostPoints->SetNumberOfComponents(outputPoints->GetData()->GetNumberOfComponents());
  vtkSmartPointer<vtkIdTypeArray> ghostRegionIds =
    vtkSmartPointer<vtkIdTypeArray>::New();
  for (vtkIdType i = 0; i < output->GetNumberOfPoints(); ++i)
  {
    if (pointGhostArray && pointGhostArray->GetTypedComponent(i, 0) &
        vtkDataSetAttributes::DUPLICATEPOINT)
    {
      ghostPoints->InsertNextTuple(i, outputPoints->GetData());

      // Add the region id starting value for this rank so it is in terms of the
      // global region numbering.
      vtkIdType regionId = pointRegionIds->GetTypedComponent(i, 0);
      ghostRegionIds->InsertNextValue(regionId + regionStarts[myRank]);
    }
  }

  // Gather up number of point tuples on each rank
  std::vector<vtkIdType> remotePointLengths(numRanks, 0);
  vtkIdType localPointsLength = ghostPoints->GetNumberOfValues();
  controller->AllGather(&localPointsLength, &remotePointLengths[0], 1);

  std::vector<vtkIdType> remotePointOffsets(numRanks+1, 0);
  std::partial_sum(remotePointLengths.begin(), remotePointLengths.end(),
    remotePointOffsets.begin() + 1);

  // Gather points
  vtkSmartPointer<vtkDataArray> remotePointData;
  remotePointData.TakeReference(ghostPoints->NewInstance());
  remotePointData->SetNumberOfComponents(3);
  remotePointData->SetNumberOfTuples(remotePointOffsets[numRanks] / 3);

  controller->AllGatherV(ghostPoints, remotePointData,
                         &remotePointLengths[0], &remotePointOffsets[0]);

  // The remote point lengths and offsets account for the point data being
  // stored as 3-tuples. Divide by 3 in preparation for distributing 1-tuple
  // point region ids.
  for (int i = 0; i < numRanks; ++i)
  {
    remotePointLengths[i] /= 3;
    remotePointOffsets[i+1] /= 3;
  }

  // Gather the RegionIds associated with the ghost points.
  vtkSmartPointer<vtkIdTypeArray> remoteRegionIds =
    vtkSmartPointer<vtkIdTypeArray>::New();
  remoteRegionIds->SetNumberOfComponents(1);
  remoteRegionIds->SetNumberOfTuples(remotePointOffsets[numRanks]);
  remoteRegionIds->FillValue(-1); // Invalid region id

  controller->AllGatherV(ghostRegionIds, remoteRegionIds,
                         &remotePointLengths[0], &remotePointOffsets[0]);

  ghostRegionIds = nullptr;

  // Links from local region ids to remote region ids. Vector index is local
  // region id, and the set contains linked remote ids.
  typedef std::vector< std::set< vtkIdType > > RegionLinksType;
  RegionLinksType links(regionStarts[numRanks]);

  if (output->GetNumberOfPoints() > 0)
  {
    // Now resolve the remote ghost points to local points if possible.
    // TODO - use a surface filter on the output to reduce the number of points
    // in the point locator.
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

      for (vtkIdType remoteIdx = remotePointOffsets[rank];
           remoteIdx < remotePointOffsets[rank+1]; ++remoteIdx)
      {
        double x[3];
        remotePointData->GetTuple(remoteIdx, x);

        vtkIdType localId = localPointLocator->FindClosestPoint(x);
        // Skip local ghost points as we do not need ghost-ghost links.
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

        vtkIdType remoteRegionId = remoteRegionIds->GetTypedComponent(remoteIdx, 0);

        if (links[localRegionId].count(remoteRegionId) == 0)
        {
          // Link not already established. Add it here.
          links[localRegionId].insert(remoteRegionId);
        }
      }
    }
  }

  remoteRegionIds = nullptr;

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
  controller->AllGather(&localNumLinks, &linkCounts[0], 1);

  // Compute starting region IDs on each rank
  for (int i = 0; i < numRanks; ++i)
  {
    linkStarts[i + 1] = linkCounts[i] + linkStarts[i];
  }

  std::vector< vtkIdType > allLinks(linkStarts[numRanks]);

  controller->AllGatherV(&localLinks[0], &allLinks[0],
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
  controller->AllReduce(&localRegionSizes[0], &globalRegionSizes[0],
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
      controller->AllReduce(&minDist2, &globalMinDist2, 1,
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
      controller->AllReduce(&minDist2Rank, &globalMinDist2Rank, 1,
        vtkCommunicator::MAX_OP);

      // Get the id of the region nearest the point and use that in the
      // threshold filter below.
      controller->Broadcast(&minDist2Region, 1, globalMinDist2Rank);
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
