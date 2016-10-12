/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEuclideanClusterExtraction.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEuclideanClusterExtraction.h"

#include "vtkPointSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkFloatArray.h"
#include "vtkAbstractPointLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkIdTypeArray.h"

vtkStandardNewMacro(vtkEuclideanClusterExtraction);
vtkCxxSetObjectMacro(vtkEuclideanClusterExtraction,Locator,vtkAbstractPointLocator);

//----------------------------------------------------------------------------
// Construct with default extraction mode to extract largest cluster.
vtkEuclideanClusterExtraction::vtkEuclideanClusterExtraction()
{
  this->ClusterSizes = vtkIdTypeArray::New();
  this->ExtractionMode = VTK_EXTRACT_LARGEST_CLUSTER;
  this->ColorClusters = false;

  this->ScalarConnectivity = false;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;

  this->ClosestPoint[0] = this->ClosestPoint[1] = this->ClosestPoint[2] = 0.0;

  this->Locator = vtkStaticPointLocator::New();

  this->NeighborScalars = vtkFloatArray::New();
  this->NeighborScalars->Allocate(64);

  this->NeighborPointIds = vtkIdList::New();
  this->NeighborPointIds->Allocate(64);

  this->Seeds = vtkIdList::New();
  this->SpecifiedClusterIds = vtkIdList::New();

  this->NewScalars = 0;

}

//----------------------------------------------------------------------------
vtkEuclideanClusterExtraction::~vtkEuclideanClusterExtraction()
{
  this->SetLocator(NULL);
  this->ClusterSizes->Delete();
  this->NeighborScalars->Delete();
  this->NeighborPointIds->Delete();
  this->Seeds->Delete();
  this->SpecifiedClusterIds->Delete();
}

//----------------------------------------------------------------------------
int vtkEuclideanClusterExtraction::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts, i, ptId;
  vtkPoints *newPts;
  int maxPointsInCluster, clusterId, largestClusterId=0;
  vtkPointData *pd=input->GetPointData(), *outputPD=output->GetPointData();

  vtkDebugMacro(<<"Executing point clustering filter.");

  //  Check input/allocate storage
  //
  if ( (numPts=input->GetNumberOfPoints()) < 1 )
  {
    vtkDebugMacro(<<"No data to cluster!");
    return 1;
  }
  vtkPoints *inPts = input->GetPoints();

  // Need to build a locator
  if ( !this->Locator )
  {
    vtkErrorMacro(<<"Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // See whether to consider scalar connectivity
  //
  this->InScalars = input->GetPointData()->GetScalars();
  if ( !this->ScalarConnectivity )
  {
    this->InScalars = NULL;
  }
  else
  {
    this->NeighborScalars->SetNumberOfComponents(this->InScalars->GetNumberOfComponents());
    if ( this->ScalarRange[1] < this->ScalarRange[0] )
    {
      this->ScalarRange[1] = this->ScalarRange[0];
    }
  }

  // Initialize.  Keep track of the points visited.
  //
  this->Visited = new char [numPts];
  std::fill_n(this->Visited, numPts, static_cast<char>(0));

  this->ClusterSizes->Reset();
  this->PointMap = new vtkIdType[numPts];
  std::fill_n(this->PointMap, numPts, static_cast<vtkIdType>(-1));

  this->NewScalars = vtkIdTypeArray::New();
  this->NewScalars->SetName("ClusterId");
  this->NewScalars->SetNumberOfTuples(numPts);

  newPts = vtkPoints::New();
  newPts->SetDataType(input->GetPoints()->GetDataType());
  newPts->Allocate(numPts);

  // Traverse all points marking those visited.  Each new search
  // starts a new connected cluster. Connected clusters grow
  // using a connected wave propagation.
  //
  this->Wave = vtkIdList::New();
  this->Wave->Allocate(numPts/4+1,numPts);
  this->Wave2 = vtkIdList::New();
  this->Wave2->Allocate(numPts/4+1,numPts);

  this->PointNumber = 0;
  this->ClusterNumber = 0;
  maxPointsInCluster = 0;

  this->PointIds = vtkIdList::New();
  this->PointIds->Allocate(8, VTK_CELL_SIZE);

  if ( this->ExtractionMode != VTK_EXTRACT_POINT_SEEDED_CLUSTERS &&
  this->ExtractionMode != VTK_EXTRACT_CLOSEST_POINT_CLUSTER )
  { //visit all points assigning cluster number
    for (ptId=0; ptId < numPts; ptId++)
    {
      if ( ptId && !(ptId % 10000) )
      {
        this->UpdateProgress (0.1 + 0.8*ptId/numPts);
      }

      if ( ! this->Visited[ptId] )
      {
        this->NumPointsInCluster = 0;
        this->InsertIntoWave(this->Wave,ptId);
        this->TraverseAndMark (inPts);

        if ( this->NumPointsInCluster > maxPointsInCluster )
        {
          maxPointsInCluster = this->NumPointsInCluster;
          largestClusterId = this->ClusterNumber;
        }

        if ( this->NumPointsInCluster > 0 )
        {
          this->ClusterSizes->InsertValue(this->ClusterNumber++,
                                          this->NumPointsInCluster);
        }
        this->Wave->Reset();
        this->Wave2->Reset();
      }
    }
  }
  else // clusters have been seeded, everything considered in same cluster
  {
    this->NumPointsInCluster = 0;

    if ( this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_CLUSTERS )
    {
      this->NumPointsInCluster = 0;
      for (i=0; i < this->Seeds->GetNumberOfIds(); i++)
      {
        ptId = this->Seeds->GetId(i);
        if ( ptId >= 0 )
        {
          this->InsertIntoWave(this->Wave,ptId);
        }
      }
    }
    else if ( this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_CLUSTER )
    {//loop over points, find closest one
      ptId = this->Locator->FindClosestPoint(this->ClosestPoint);
      this->InsertIntoWave(this->Wave,ptId);
    }
    this->UpdateProgress (0.5);

    //mark all seeded clusters
    this->TraverseAndMark (inPts);
    this->ClusterSizes->InsertValue(this->ClusterNumber,this->NumPointsInCluster);
    this->UpdateProgress (0.9);
  }

  vtkDebugMacro (<<"Extracted " << this->ClusterNumber << " cluster(s)");
  this->Wave->Delete();
  this->Wave2->Delete();
  delete [] this->Visited;

  // Now that points have been marked, traverse the PointMap pulling
  // everything that has been visited and is selected for output.
  outputPD->CopyAllocate(pd);
  if ( this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_CLUSTERS ||
  this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_CLUSTER ||
  this->ExtractionMode == VTK_EXTRACT_ALL_CLUSTERS)
  { // extract any point that's been visited
    for (ptId=0; ptId < numPts; ptId++)
    {
      if ( this->PointMap[ptId] >= 0 )
      {
        newPts->InsertPoint(this->PointMap[ptId],inPts->GetPoint(ptId));
        outputPD->CopyData(pd,ptId,this->PointMap[ptId]);
      }
    }
  }
  else if ( this->ExtractionMode == VTK_EXTRACT_SPECIFIED_CLUSTERS )
  {
    bool inCluster;
    for (ptId=0; ptId < numPts; ptId++)
    {
      if ( this->PointMap[ptId] >= 0 )
      {
        clusterId = this->NewScalars->GetValue(this->PointMap[ptId]);
        for (inCluster=false,i=0; i<this->SpecifiedClusterIds->GetNumberOfIds(); i++)
        {
          if ( clusterId == this->SpecifiedClusterIds->GetId(i) )
          {
            inCluster = true;
            break;
          }
        }
        if ( inCluster )
        {
          newPts->InsertPoint(this->PointMap[ptId],inPts->GetPoint(ptId));
          outputPD->CopyData(pd,ptId,this->PointMap[ptId]);
        }
      }
    }
  }
  else //extract largest cluster
  {
    for (ptId=0; ptId < numPts; ptId++)
    {
      if ( this->PointMap[ptId] >= 0 )
      {
        clusterId = this->NewScalars->GetValue(this->PointMap[ptId]);
        if ( clusterId == largestClusterId )
        {
          newPts->InsertPoint(this->PointMap[ptId],inPts->GetPoint(ptId));
          outputPD->CopyData(pd,ptId,this->PointMap[ptId]);
        }
      }
    }
  }

  // if coloring clusters; send down new scalar data
  if ( this->ColorClusters )
  {
    int idx = outputPD->AddArray(this->NewScalars);
    outputPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
  }
  this->NewScalars->Delete();

  newPts->Squeeze();
  output->SetPoints(newPts);

  delete [] this->PointMap;
  this->PointIds->Delete();

  // print out some debugging information
  int num = this->GetNumberOfExtractedClusters();
  int count = 0;

  for (int ii = 0; ii < num; ii++)
  {
    count += this->ClusterSizes->GetValue (ii);
  }
  vtkDebugMacro (<< "Total # of points accounted for: " << count);
  vtkDebugMacro (<< "Extracted " << newPts->GetNumberOfPoints() << " points");
  newPts->Delete();

  return 1;
}


//----------------------------------------------------------------------------
// Insert point into connected wave. Check to make sure it satisfies connectivity
// criterion (if enabled).
void vtkEuclideanClusterExtraction::
InsertIntoWave(vtkIdList *wave, vtkIdType ptId)
{
  this->Visited[ptId] = 1;
  if ( this->InScalars ) //is scalar connectivity enabled?
  {
    double s = this->InScalars->GetTuple1(ptId);
    if ( s >= this->ScalarRange[0] && s <= this->ScalarRange[1] )
    {
      wave->InsertNextId(ptId);
    }
  }
  else
  {
    wave->InsertNextId(ptId);
  }
}


//----------------------------------------------------------------------------
// Update current point information including updating cluster number.  Note:
// traversal occurs across proximally located points, possibly limited by
// scalar connectivty.
//
void vtkEuclideanClusterExtraction::TraverseAndMark (vtkPoints *inPts)
{
  vtkIdType i, j, numPts, numIds, ptId;
  vtkIdList *tmpWave;
  double x[3];

  while ( (numIds=this->Wave->GetNumberOfIds()) > 0 )
  {
    for ( i=0; i < numIds; i++ ) //for all points in this wave
    {
      ptId = this->Wave->GetId(i);
      this->PointMap[ptId] = this->PointNumber++;
      this->NewScalars->SetValue(this->PointMap[ptId],this->ClusterNumber);
      this->NumPointsInCluster++;

      inPts->GetPoint(ptId,x);
      this->Locator->FindPointsWithinRadius(this->Radius,x,this->NeighborPointIds);

      numPts = this->NeighborPointIds->GetNumberOfIds();
      for (j=0; j < numPts; ++j)
      {
        ptId = this->NeighborPointIds->GetId(j);
        if ( ! this->Visited[ptId] )
        {
          this->InsertIntoWave(this->Wave2,ptId);
        }//if point not yet visited
      }//for all neighbors
    }//for all cells in this connected wave

    tmpWave = this->Wave;
    this->Wave = this->Wave2;
    this->Wave2 = tmpWave;
    tmpWave->Reset();
  } //while wave is not empty

  return;
}

//----------------------------------------------------------------------------
// Obtain the number of connected clusters.
int vtkEuclideanClusterExtraction::GetNumberOfExtractedClusters()
{
  return this->ClusterSizes->GetMaxId() + 1;
}

//----------------------------------------------------------------------------
// Initialize list of point ids used to seed clusters.
void vtkEuclideanClusterExtraction::InitializeSeedList()
{
  this->Modified();
  this->Seeds->Reset();
}

//----------------------------------------------------------------------------
// Add a seed id. Note: ids are 0-offset.
void vtkEuclideanClusterExtraction::AddSeed(vtkIdType id)
{
  this->Modified();
  this->Seeds->InsertNextId(id);
}

//----------------------------------------------------------------------------
// Delete a seed id. Note: ids are 0-offset.
void vtkEuclideanClusterExtraction::DeleteSeed(vtkIdType id)
{
  this->Modified();
  this->Seeds->DeleteId(id);
}

//----------------------------------------------------------------------------
// Initialize list of cluster ids to extract.
void vtkEuclideanClusterExtraction::InitializeSpecifiedClusterList()
{
  this->Modified();
  this->SpecifiedClusterIds->Reset();
}

//----------------------------------------------------------------------------
// Add a cluster id to extract. Note: ids are 0-offset.
void vtkEuclideanClusterExtraction::AddSpecifiedCluster(int id)
{
  this->Modified();
  this->SpecifiedClusterIds->InsertNextId(id);
}

//----------------------------------------------------------------------------
// Delete a cluster id to extract. Note: ids are 0-offset.
void vtkEuclideanClusterExtraction::DeleteSpecifiedCluster(int id)
{
  this->Modified();
  this->SpecifiedClusterIds->DeleteId(id);
}

//----------------------------------------------------------------------------
int vtkEuclideanClusterExtraction::
FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkEuclideanClusterExtraction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Extraction Mode: ";
  os << this->GetExtractionModeAsString() << "\n";

  os << indent << "Closest Point: (" << this->ClosestPoint[0] << ", "
     << this->ClosestPoint[1] << ", " << this->ClosestPoint[2] << ")\n";

  os << indent << "Color Clusters: " << (this->ColorClusters ? "On\n" : "Off\n");

  os << indent << "Scalar Connectivity: "
     << (this->ScalarConnectivity ? "On\n" : "Off\n");

  double *range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";

  os << indent << "Locator: " << this->Locator << "\n";
}
