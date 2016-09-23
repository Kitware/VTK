/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOctreePointLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkOctreePointLocator.h"

#include "vtkCellArray.h"
#include "vtkCommand.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOctreePointLocatorNode.h"
#include "vtkPoints.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"

#include <list>
#include <map>
#include <queue>
#include <stack>
#include <vector>

vtkStandardNewMacro(vtkOctreePointLocator);

// helper class for ordering the points in
// vtkOctreePointLocator::FindClosestNPoints()
namespace
{
  class OrderPoints
  {
  public:
    OrderPoints(int numDesiredPoints)
    {
        this->NumDesiredPoints = numDesiredPoints;
        this->NumPoints = 0;
        this->LargestDist2 = VTK_FLOAT_MAX;
    }

    void InsertPoint(float dist2, vtkIdType id)
    {
        if(dist2 <= this->LargestDist2 ||
           this->NumPoints < this->NumDesiredPoints)
        {
          std::map<float, std::list<vtkIdType> >::iterator it=
            this->dist2ToIds.find(dist2);
          this->NumPoints++;
          if(it == this->dist2ToIds.end())
          {
            std::list<vtkIdType> idset;
            idset.push_back(id);
            this->dist2ToIds[dist2] = idset;
          }
          else
          {
            it->second.push_back(id);
          }
          if(this->NumPoints > this->NumDesiredPoints)
          {
            it=this->dist2ToIds.end();
            it--;
            if((this->NumPoints-it->second.size()) > this->NumDesiredPoints)
            {
              this->NumPoints -= it->second.size();
              std::map<float, std::list<vtkIdType> >::iterator it2 = it;
              it2--;
              this->LargestDist2 = it2->first;
              this->dist2ToIds.erase(it);
            }
          }
        }
    }
    void GetSortedIds(vtkIdList* ids)
    {
        ids->Reset();
        vtkIdType numIds = (this->NumDesiredPoints < this->NumPoints)
          ? this->NumDesiredPoints : this->NumPoints;
        ids->SetNumberOfIds(numIds);
        vtkIdType counter = 0;
        std::map<float, std::list<vtkIdType> >::iterator it=
          this->dist2ToIds.begin();
        while(counter < numIds && it!=this->dist2ToIds.end())
        {
          std::list<vtkIdType>::iterator lit=it->second.begin();
          while(counter < numIds && lit!=it->second.end())
          {
            ids->InsertId(counter, *lit);
            counter++;
            lit++;
          }
          it++;
        }
    }

    float GetLargestDist2()
    {
        return this->LargestDist2;
    }

  private:
    size_t NumDesiredPoints, NumPoints;
    float LargestDist2;
    // map from dist^2 to a list of ids
    std::map<float, std::list<vtkIdType> > dist2ToIds;
  };
}

//----------------------------------------------------------------------------
vtkOctreePointLocator::vtkOctreePointLocator()
{
  this->FudgeFactor = 0;
  this->MaxWidth = 0.0;
  this->MaxLevel  = 20;
  this->MaximumPointsPerRegion = 100;
  this->Level    = 0;
  this->Top      = NULL;
  this->NumberOfLocatorPoints = 0;
  this->LocatorPoints = NULL;
  this->LocatorIds = NULL;
  this->LeafNodeList = NULL;
  this->CreateCubicOctants = 1;
  this->NumberOfLeafNodes = 0;
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::DeleteAllDescendants(
  vtkOctreePointLocatorNode *octant)
{
  if(octant->GetChild(0))
  {
    for(int i=0;i<8;i++)
    {
      vtkOctreePointLocatorNode* child = octant->GetChild(i);
      vtkOctreePointLocator::DeleteAllDescendants(child);
    }
    octant->DeleteChildNodes();
  }
}

//----------------------------------------------------------------------------
vtkOctreePointLocator::~vtkOctreePointLocator()
{
  this->FreeSearchStructure();

  delete []this->LocatorPoints;
  this->LocatorPoints = 0;

  delete []this->LocatorIds;
  this->LocatorIds = 0;

  delete []this->LeafNodeList;
  this->LeafNodeList = 0;
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::GetBounds(double *bounds)
{
  if (this->Top)
  {
    this->Top->GetBounds(bounds);
  }
}

//----------------------------------------------------------------------------
double* vtkOctreePointLocator::GetBounds()
{
  double* bounds = vtkAbstractPointLocator::GetBounds();
  if (this->Top)
  {
    this->Top->GetBounds(bounds);
  }
  return bounds;
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::GetRegionBounds(int leafNodeId,
                                            double bounds[6])
{
  if ( (leafNodeId < 0) || (leafNodeId >= this->NumberOfLeafNodes))
  {
    vtkErrorMacro("Invalid region.");
    return;
  }

  vtkOctreePointLocatorNode *node = this->LeafNodeList[leafNodeId];

  node->GetBounds(bounds);
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::GetRegionDataBounds(int leafNodeId,
                                                double bounds[6])
{
  if ( (leafNodeId < 0) || (leafNodeId >= this->NumberOfLeafNodes))
  {
    vtkErrorMacro("Invalid region.");
    return;
  }

  vtkOctreePointLocatorNode *node = this->LeafNodeList[leafNodeId];

  node->GetDataBounds(bounds);
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::SetDataBoundsToSpatialBounds(
  vtkOctreePointLocatorNode *octant)
{
  octant->SetMinDataBounds(octant->GetMinBounds());
  octant->SetMaxDataBounds(octant->GetMaxBounds());

  if (octant->GetChild(0))
  {
    for(int i=0;i<8;i++)
    {
      vtkOctreePointLocator::SetDataBoundsToSpatialBounds(octant->GetChild(i));
    }
  }
}

//----------------------------------------------------------------------------
int vtkOctreePointLocator::DivideTest(int size, int level)
{
  if (level >= this->MaxLevel)
  {
    return 0;
  }

  if(size > this->GetMaximumPointsPerRegion())
  {
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::DivideRegion(
  vtkOctreePointLocatorNode *node, int*  ordering, int level)
{
  if(!this->DivideTest(node->GetNumberOfPoints(), level))
  {
    return;
  }
  if(level >= this->Level)
  {
    this->Level = level + 1;
  }

  node->CreateChildNodes();
  int numberOfPoints = node->GetNumberOfPoints();
  vtkDataSet* ds = this->GetDataSet();

  std::vector<int> points[7];
  int  i;
  int subOctantNumberOfPoints[8] = {0,0,0,0,0,0,0,0};
  for(i=0;i<numberOfPoints;i++)
  {
    int index = node->GetSubOctantIndex(ds->GetPoint(ordering[i]), 0);
    if(index)
    {
      points[index-1].push_back(ordering[i]);
    }
    else
    {
      ordering[subOctantNumberOfPoints[0]] = ordering[i];
    }
    subOctantNumberOfPoints[index]++;
  }
  int counter = 0;
  int sizeOfInt = sizeof(int);
  for(i=0;i<7;i++)
  {
    counter += subOctantNumberOfPoints[i];
    if(!points[i].empty())
    {
      memcpy(ordering+counter, &(points[i][0]),
             subOctantNumberOfPoints[i+1]*sizeOfInt);
    }
  }
  counter = 0;
  for(i=0;i<8;i++)
  {
    node->GetChild(i)->SetNumberOfPoints(subOctantNumberOfPoints[i]);
    this->DivideRegion(node->GetChild(i), ordering+counter, level + 1);
    counter += subOctantNumberOfPoints[i];
  }
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::BuildLocator()
{
  if(!this->GetDataSet())
  {
    vtkErrorMacro("Must set a valid data set first.");
  }

  int numPoints = this->GetDataSet()->GetNumberOfPoints();

  if (numPoints < 1)
  {
    vtkErrorMacro(<< "No points to build from.");
    return;
  }

  if (numPoints >= VTK_INT_MAX)
  {
     // When point IDs are stored in an "int" instead of a vtkIdType,
     // performance doubles.  So we store point IDs in an "int" during
     // the calculation.  This will need to be rewritten if true 64 bit
     // IDs are required.

    vtkErrorMacro("Intentional 64 bit error - time to rewrite code.");
     return;
  }

  vtkDebugMacro( << "Creating octree" );

  if ( (this->BuildTime > this->MTime)
       && (this->BuildTime > this->DataSet->GetMTime()) )
  {
    return;
  }
  this->FreeSearchStructure();

  // Fix bounds - (1) push out a little if flat
  // (2) pull back the x, y and z lower bounds a little bit so that
  // points are clearly "inside" the spatial region.  Point p is
  // "inside" region r = [r1, r2] if r1 < p <= r2.

  double bounds[6], diff[3];

  this->GetDataSet()->GetBounds(bounds);

  this->MaxWidth = 0.0;
  int i;
  for (i=0; i<3; i++)
  {
    diff[i] = bounds[2*i+1] - bounds[2*i];
    this->MaxWidth = static_cast<float>
      ((diff[i] > this->MaxWidth) ? diff[i] : this->MaxWidth);
  }

  if(this->CreateCubicOctants)
  {
    // make the bounding box have equal length sides so that all octants
    // will also have equal length sides
    for(i=0;i<3;i++)
    {
      if(diff[i] != this->MaxWidth)
      {
        double delta = this->MaxWidth - diff[i];
        bounds[2*i] -= .5*delta;
        bounds[2*i+1] += .5*delta;
        diff[i] =  this->MaxWidth;
      }
    }
  }

  this->FudgeFactor = this->MaxWidth * 10e-6;

  double aLittle = this->MaxWidth * 10e-2;

  for (i=0; i<3; i++)
  {
    if (diff[i] < aLittle)         // case (1) above
    {
      double temp = bounds[2*i];
      bounds[2*i]   = bounds[2*i+1] - aLittle;
      bounds[2*i+1] = temp + aLittle;
    }
    else                           // case (2) above
    {
      bounds[2*i] -= this->FudgeFactor;
    }
  }

  // root node of octree - it's the whole space

  vtkOctreePointLocatorNode *node = this->Top = vtkOctreePointLocatorNode::New();

  node->SetBounds(bounds[0], bounds[1],
                  bounds[2], bounds[3],
                  bounds[4], bounds[5]);

  node->SetNumberOfPoints(numPoints);

  node->SetDataBounds(bounds[0], bounds[1],
                      bounds[2], bounds[3],
                      bounds[4], bounds[5]);


  this->LocatorIds = new int [numPoints];
  this->LocatorPoints = new float [3 * numPoints];

  if ( !this->LocatorPoints || !this->LocatorIds)
  {
    this->FreeSearchStructure();
    vtkErrorMacro("vtkOctreePointLocator::BuildLocatorFromPoints - memory allocation");
    return;
  }

  for(i=0;i<numPoints;i++)
  {
    this->LocatorIds[i] = i;
  }
  this->DivideRegion(node, this->LocatorIds, 0);
  // TODO: may want to directly check if there exists a point array that
  // is of type float and directly copy that instead of dealing with
  // all of the casts
  vtkDataSet* ds = this->GetDataSet();
  for(i=0;i<numPoints;i++)
  {
    double *pt = ds->GetPoint(this->LocatorIds[i]);

    this->LocatorPoints[i*3] = static_cast<float>(pt[0]);
    this->LocatorPoints[i*3+1] = static_cast<float>(pt[1]);
    this->LocatorPoints[i*3+2] = static_cast<float>(pt[2]);
  }

  int nextLeafNodeId = 0;
  int nextMinId = 0;
  this->Top->ComputeOctreeNodeInformation(this->Top, nextLeafNodeId,
                                          nextMinId, this->LocatorPoints);

  this->NumberOfLeafNodes = nextLeafNodeId;
  int index = 0;
  this->LeafNodeList = new vtkOctreePointLocatorNode*[this->NumberOfLeafNodes];
  this->BuildLeafNodeList(this->Top, index);
  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::BuildLeafNodeList(vtkOctreePointLocatorNode* node,
                                              int & index)
{
  if(node->GetChild(0))
  {
    for(int i=0;i<8;i++)
    {
      this->BuildLeafNodeList(node->GetChild(i), index);
    }
  }
  else
  {
    this->LeafNodeList[index] = node;
    index++;
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkOctreePointLocator::FindClosestPoint(const double x[3])
{
  double dist2(0);
  return this->FindClosestPoint(x[0], x[1], x[2], dist2);
}

//----------------------------------------------------------------------------
vtkIdType vtkOctreePointLocator::FindClosestPoint(
  double x, double y, double z, double &dist2)
{
  this->BuildLocator();

  int closeId=-1;
  vtkIdType newCloseId=-1;
  double newDistance2 = 4 * this->MaxWidth * this->MaxWidth;

  int regionId = this->GetRegionContainingPoint(x, y, z);
  vtkIdType closePointId = -1;
  if (regionId < 0)
  {
    // This point is not inside the space divided by the octree.
    // Find the point on the boundary that is closest to it.

    double pt[3];
    this->Top->GetDistance2ToBoundary(x, y, z, pt, this->Top, 1);

    double *min = this->Top->GetMinBounds();
    double *max = this->Top->GetMaxBounds();

    // GetDistance2ToBoundary will sometimes return a point *just*
    // *barely* outside the bounds of the region.  Move that point to
    // just barely *inside* instead.

    if (pt[0] <= min[0])
    {
      pt[0] = min[0] + this->FudgeFactor;
    }
    if (pt[1] <= min[1])
    {
      pt[1] = min[1] + this->FudgeFactor;
    }
    if (pt[2] <= min[2])
    {
      pt[2] = min[2] + this->FudgeFactor;
    }
    if (pt[0] >= max[0])
    {
      pt[0] = max[0] - this->FudgeFactor;
    }
    if (pt[1] >= max[1])
    {
      pt[1] = max[1] - this->FudgeFactor;
    }
    if (pt[2] >= max[2])
    {
      pt[2] = max[2] - this->FudgeFactor;
    }

    regionId = this->GetRegionContainingPoint(pt[0], pt[1], pt[2]);

    closeId = this->_FindClosestPointInRegion(regionId, x, y, z, dist2);

    closePointId = static_cast<vtkIdType>(this->LocatorIds[closeId]);

    // Check to see if neighboring regions have a closer point
    newCloseId =  this->FindClosestPointInSphere(x, y, z,
                                                 sqrt(dist2),        // radius
                                                 regionId,     // skip this region
                                                 newDistance2);// distance to closest point
    if(newDistance2 < dist2)
    {
      dist2 = newDistance2;
      closePointId = newCloseId;
    }
  }
  else     // Point is inside an octree region
  {
    closeId = this->_FindClosestPointInRegion(regionId, x, y, z, dist2);
    closePointId = static_cast<vtkIdType>(this->LocatorIds[closeId]);

    if (dist2 > 0.0)
    {
      float dist2ToBoundary = static_cast<float>(
        this->LeafNodeList[regionId]->GetDistance2ToInnerBoundary(x, y, z, this->Top));

      if (dist2ToBoundary < dist2)
      {
        // The closest point may be in a neighboring region
        newCloseId = this->FindClosestPointInSphere(x, y, z,
                                                    sqrt(dist2),        // radius
                                                    regionId,     // skip this region
                                                    newDistance2);
        if(newDistance2 < dist2)
        {
          dist2 = newDistance2;
          closePointId = newCloseId;
        }
      }
    }
  }

  return closePointId;
}

//----------------------------------------------------------------------------
vtkIdType vtkOctreePointLocator::FindClosestPointWithinRadius(
  double radius, const double x[3], double& dist2)
{
  return this->FindClosestPointInSphere(x[0], x[1], x[2], radius, -2, dist2);
}


//----------------------------------------------------------------------------
vtkIdType vtkOctreePointLocator::FindClosestPointInRegion(
  int regionId, double *x, double &dist2)
{
  return this->FindClosestPointInRegion(regionId, x[0], x[1], x[2], dist2);
}

//----------------------------------------------------------------------------
vtkIdType vtkOctreePointLocator::FindClosestPointInRegion(
  int regionId, double x, double y, double z, double &dist2)
{
  if (!this->LocatorPoints)
  {
    // if the locator hasn't been built yet the regionId is garbage!
    vtkErrorMacro("vtkOctreePointLocator::FindClosestPointInRegion - must build locator first");
    return -1;
  }
  int localId = this->_FindClosestPointInRegion(regionId, x, y, z, dist2);

  vtkIdType originalId = -1;

  if (localId >= 0)
  {
    originalId = static_cast<vtkIdType>(this->LocatorIds[localId]);
  }

  return originalId;
}

//----------------------------------------------------------------------------
int vtkOctreePointLocator::_FindClosestPointInRegion(
  int leafNodeId, double x, double y, double z, double &dist2)
{
  int minId=0;

  float fx = static_cast<float>(x);
  float fy = static_cast<float>(y);
  float fz = static_cast<float>(z);

  float minDistance2 = 4 * this->MaxWidth * this->MaxWidth;

  int idx = this->LeafNodeList[leafNodeId]->GetMinID();

  float *candidate = this->LocatorPoints + (idx * 3);

  int numPoints = this->LeafNodeList[leafNodeId]->GetNumberOfPoints();
  for (int i=0; i < numPoints; i++)
  {
    float diffx = fx-candidate[0];
    float diffy = fy-candidate[1];
    float diffz = fz-candidate[2];
    float dxyz = diffx*diffx + diffy*diffy + diffz*diffz;
    if(dxyz < minDistance2)
    {
      minId = idx + i;
      minDistance2 = dxyz;
      if(dxyz == 0.0)
      {
        break;
      }
    }

    candidate += 3;
  }

  dist2 = minDistance2;

  return minId;
}

//----------------------------------------------------------------------------
int vtkOctreePointLocator::FindClosestPointInSphere(
  double x, double y, double z, double radius, int skipRegion, double &dist2)
{
  this->BuildLocator();

  dist2 = radius * radius * 1.0001;
  int localCloseId = -1;

  std::stack<vtkOctreePointLocatorNode*> regions;
  regions.push(this->Top);
  while(!regions.empty())
  {
    vtkOctreePointLocatorNode* region = regions.top();
    regions.pop();
    if(region->GetChild(0))
    {
      for(int i=0;i<8;i++)
      {
        vtkOctreePointLocatorNode* child = region->GetChild(i);
        // must check for leaf nodes here in case skipRegion == -1
        // since all non-leaf nodes have Id = -1.
        if(child->GetID() != skipRegion &&
           (child->GetDistance2ToBoundary(x, y, z, this->Top, 1) < dist2 ||
            child->ContainsPoint(x, y, z, 0)))
        {
          regions.push(child);
        }
      }
    }
    else
    {
      double tempDist2 = dist2;
      int tempId =
        this->_FindClosestPointInRegion(region->GetID(), x, y, z, tempDist2);

      if (tempDist2 < dist2)
      {
        dist2 = tempDist2;
        localCloseId = tempId;
      }
    }
  }

  vtkIdType originalId = -1;
  if(localCloseId >= 0 && dist2 <= radius*radius)
  {
    originalId = static_cast<vtkIdType>(this->LocatorIds[localCloseId]);
  }
  return originalId;
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::FindPointsWithinRadius(
  double radius, const double x[3], vtkIdList* result)
{
  result->Reset();
  this->BuildLocator();
  // don't forget to square the radius
  this->FindPointsWithinRadius(this->Top, radius*radius, x, result);
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::FindPointsWithinRadius(
  vtkOctreePointLocatorNode* node, double radiusSquared, const double x[3],
  vtkIdList* result)
{
  double b[6];
  node->GetBounds(b);

  double mindist2 = 0; // distance to closest vertex of BB
  double maxdist2 = 0; // distance to furthest vertex of BB
  // x-dir
  if(x[0] < b[0])
  {
    mindist2 = (b[0]-x[0])*(b[0]-x[0]);
    maxdist2 = (b[1]-x[0])*(b[1]-x[0]);
  }
  else if(x[0] > b[1])
  {
    mindist2 = (b[1]-x[0])*(b[1]-x[0]);
    maxdist2 = (b[0]-x[0])*(b[0]-x[0]);
  }
  else if((b[1]-x[0]) > (x[0]-b[0]))
  {
    maxdist2 = (b[1]-x[0])*(b[1]-x[0]);
  }
  else
  {
    maxdist2 = (b[0]-x[0])*(b[0]-x[0]);
  }
  // y-dir
  if(x[1] < b[2])
  {
    mindist2 += (b[2]-x[1])*(b[2]-x[1]);
    maxdist2 += (b[3]-x[1])*(b[3]-x[1]);
  }
  else if(x[1] > b[3])
  {
    mindist2 += (b[3]-x[1])*(b[3]-x[1]);
    maxdist2 += (b[2]-x[1])*(b[2]-x[1]);
  }
  else if((b[3]-x[1]) > (x[1]-b[2]))
  {
    maxdist2 += (b[3]-x[1])*(b[3]-x[1]);
  }
  else
  {
    maxdist2 += (b[2]-x[1])*(b[2]-x[1]);
  }
  // z-dir
  if(x[2] < b[4])
  {
    mindist2 += (b[4]-x[2])*(b[4]-x[2]);
    maxdist2 += (b[5]-x[2])*(b[5]-x[2]);
  }
  else if(x[2] > b[5])
  {
    mindist2 += (b[5]-x[2])*(b[5]-x[2]);
    maxdist2 += (b[4]-x[2])*(b[4]-x[2]);
  }
  else if((b[5]-x[2]) > (x[2]-b[4]))
  {
    maxdist2 += (b[5]-x[2])*(b[5]-x[2]);
  }
  else
  {
    maxdist2 += (x[2]-b[4])*(x[2]-b[4]);
  }

  if(mindist2 > radiusSquared)
  {
    // non-intersecting
    return;
  }

  if(maxdist2 <= radiusSquared)
  {
    // sphere contains BB
    this->AddAllPointsInRegion(node, result);
    return;
  }

  // partial intersection of sphere & BB
  if (node->GetChild(0) == NULL)
  {
    //int regionID = node->GetID();
    int regionLoc = node->GetMinID();
    float* pt = this->LocatorPoints + (regionLoc * 3);
    vtkIdType numPoints = node->GetNumberOfPoints();
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      double dist2 = (pt[0]-x[0])*(pt[0]-x[0])+
        (pt[1]-x[1])*(pt[1]-x[1])+(pt[2]-x[2])*(pt[2]-x[2]);
      if(dist2 <= radiusSquared)
      {
        vtkIdType ptId =
          static_cast<vtkIdType>(this->LocatorIds[regionLoc + i]);
        result->InsertNextId(ptId);
      }
      pt += 3;
    }
  }
  else
  {
    for(int i=0;i<8;i++)
    {
      this->FindPointsWithinRadius(
        node->GetChild(i), radiusSquared, x, result);
    }
  }
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::FindClosestNPoints(int N, const double x[3],
                                               vtkIdList* result)
{
  result->Reset();
  if(N<=0)
  {
    return;
  }
  this->BuildLocator();

  int numTotalPoints = this->Top->GetNumberOfPoints();
  if(numTotalPoints < N)
  {
    vtkWarningMacro("Number of requested points is greater than total number of points in OctreePointLocator");
    N = numTotalPoints;
  }
  result->SetNumberOfIds(N);

  // now we want to go about finding a region that contains at least N points
  // but not many more -- hopefully the region contains X as well but we
  // can't depend on that
  vtkOctreePointLocatorNode* node = this->Top;
  vtkOctreePointLocatorNode* startingNode = 0;
  if(!node->ContainsPoint(x[0], x[1], x[2], 0))
  {
    // point is not in the region
    int numPoints = node->GetNumberOfPoints();
    vtkOctreePointLocatorNode* prevNode = node;
    while(node->GetChild(0) && numPoints > N)
    {
      prevNode = node;
      vtkOctreePointLocatorNode* nextNode = node->GetChild(0);
      double minDist2 =
        nextNode->GetDistance2ToBoundary(x[0], x[1], x[2], this->Top, 1);
      for(int i=1;i<8;i++)
      {
        double dist2 = node->GetChild(i)->GetDistance2ToBoundary(
          x[0], x[1], x[2], this->Top, 1);
        if(dist2 < minDist2)
        {
          nextNode = node->GetChild(i);
          minDist2 = dist2;
        }
      }
      node = nextNode;
      numPoints = node->GetNumberOfPoints();
    }
    if(numPoints < N)
    {
      startingNode = prevNode;
    }
    else
    {
      startingNode = node;
    }
  }
  else
  {
    int numPoints = node->GetNumberOfPoints();
    vtkOctreePointLocatorNode* prevNode = node;
    while(node->GetChild(0) && numPoints > N)
    {
      prevNode = node;
      int i;
      for(i=0;i<8;i++)
      {
        if(node->GetChild(i)->ContainsPoint(x[0], x[1], x[2], 0))
        {
          node = node->GetChild(i);
          break;
        }
      }
      numPoints = node->GetNumberOfPoints();
    }
    if(numPoints < N)
    {
      startingNode = prevNode;
    }
    else
    {
      startingNode = node;
    }
  }

  // now that we have a starting region, go through its points
  // and order them
  int regionId = startingNode->GetID();
  int numPoints = startingNode->GetNumberOfPoints();
  int where = startingNode->GetMinID();
  if(regionId < 0) // if not a leaf node
  {
    vtkOctreePointLocatorNode* parentOfNext = startingNode->GetChild(0);
    vtkOctreePointLocatorNode* next = parentOfNext->GetChild(0);
    while(next)
    {
      parentOfNext = next;
      next = next->GetChild(0);
    }
    where = parentOfNext->GetMinID();
  }
  int *ids = this->LocatorIds + where;
  float* pt = this->LocatorPoints + (where*3);
  float xfloat[3] = {static_cast<float>(x[0]), static_cast<float>(x[1]),
                     static_cast<float>(x[2])};
  OrderPoints orderedPoints(N);
  for (int i=0; i<numPoints; i++)
  {
    float dist2 = vtkMath::Distance2BetweenPoints(xfloat, pt);
    orderedPoints.InsertPoint(dist2, ids[i]);
    pt += 3;
  }

  // to finish up we have to check other regions for
  // closer points
  float largestDist2 = orderedPoints.GetLargestDist2();
  double bounds[6];
  node = this->Top;
  std::queue<vtkOctreePointLocatorNode*> nodesToBeSearched;
  nodesToBeSearched.push(node);
  while(!nodesToBeSearched.empty())
  {
    node = nodesToBeSearched.front();
    nodesToBeSearched.pop();
    if(node == startingNode)
    {
      continue;
    }
    if(node->GetChild(0))
    {
      for(int j=0;j<8;j++)
      {
        vtkOctreePointLocatorNode* child = node->GetChild(j);
        child->GetDataBounds(bounds);
        double delta[3] = {0,0,0};
        if(vtkMath::PointIsWithinBounds(const_cast<double*>(x), bounds, delta) == 1 ||
           child->GetDistance2ToBoundary(x[0], x[1], x[2], 0, 1) < largestDist2)
        {
          nodesToBeSearched.push(child);
        }
      }
    }
    else if(node->GetDistance2ToBoundary(x[0], x[1], x[2], this->Top, 1) < largestDist2)
    {
      numPoints = node->GetNumberOfPoints();
      where = node->GetMinID();
      ids = this->LocatorIds + where;
      pt = this->LocatorPoints + (where*3);
      for (int i=0; i<numPoints; i++)
      {
        float dist2 = vtkMath::Distance2BetweenPoints(xfloat, pt);
        orderedPoints.InsertPoint(dist2, ids[i]);
        pt += 3;
      }
      largestDist2 = orderedPoints.GetLargestDist2();
    }
  }
  orderedPoints.GetSortedIds(result);
}

//----------------------------------------------------------------------------
vtkIdTypeArray *vtkOctreePointLocator::GetPointsInRegion(int leafNodeId)
{
  if ( (leafNodeId < 0) || (leafNodeId >= this->NumberOfLeafNodes))
  {
    vtkErrorMacro("vtkOctreePointLocator::GetPointsInRegion invalid leaf node ID");
    return NULL;
  }

  if (!this->LocatorIds)
  {
    // don't build locator since leafNodeId is probably garbage anyways
    vtkErrorMacro("vtkOctreePointLocator::GetPointsInRegion build locator first");
    return NULL;
  }

  int numPoints = this->LeafNodeList[leafNodeId]->GetNumberOfPoints();
  int where = this->LeafNodeList[leafNodeId]->GetMinID();

  vtkIdTypeArray *ptIds = vtkIdTypeArray::New();
  ptIds->SetNumberOfValues(numPoints);

  int *ids = this->LocatorIds + where;

  for (int i=0; i<numPoints; i++)
  {
    ptIds->SetValue(i, ids[i]);
  }

  return ptIds;
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::FreeSearchStructure()
{
  if (this->Top)
  {
    vtkOctreePointLocator::DeleteAllDescendants(this->Top);
    this->Top->Delete();
    this->Top = NULL;
  }
  delete [] this->LeafNodeList;
  this->LeafNodeList = NULL;

  this->NumberOfLeafNodes = 0;

  delete [] this->LocatorPoints;
  this->LocatorPoints = NULL;

  delete [] this->LocatorIds;
  this->LocatorIds = NULL;
}

//----------------------------------------------------------------------------
// build PolyData representation of all spacial regions------------
//
void vtkOctreePointLocator::GenerateRepresentation(int level,
                                                   vtkPolyData *pd)
{
  if ( this->Top == NULL )
  {
    vtkErrorMacro("vtkOctreePointLocator::GenerateRepresentation no tree");
    return;
  }

  std::list<vtkOctreePointLocatorNode*> nodesAtLevel;
  // queue of nodes to be examined and what level each one is at
  std::queue<std::pair<vtkOctreePointLocatorNode*, int> > testNodes;
  testNodes.push(std::make_pair(this->Top, 0));
  while(!testNodes.empty())
  {
    vtkOctreePointLocatorNode* node = testNodes.front().first;
    int nodeLevel = testNodes.front().second;
    testNodes.pop();
    if(nodeLevel == level)
    {
      nodesAtLevel.push_back(node);
    }
    else if(node->GetChild(0))
    {
      for(int i=0;i<8;i++)
      {
        testNodes.push(std::make_pair(node->GetChild(i), nodeLevel+1));
      }
    }
  }

  int npoints = 8 * static_cast<int>(nodesAtLevel.size());
  int npolys  = 6 * static_cast<int>(nodesAtLevel.size());

  vtkPoints* pts = vtkPoints::New();
  pts->Allocate(npoints);
  vtkCellArray* polys = vtkCellArray::New();
  polys->Allocate(npolys);

  for(std::list<vtkOctreePointLocatorNode*>::iterator it=nodesAtLevel.begin();
      it!=nodesAtLevel.end();it++)
  {
    vtkOctreePointLocator::AddPolys(*it, pts, polys);
  }

  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::AddPolys(vtkOctreePointLocatorNode* node,
                                     vtkPoints* pts, vtkCellArray * polys)
{
  vtkIdType ids[8];
  vtkIdType idList[4];
  double     x[3];

  double* min = node->GetMinBounds();
  double* max = node->GetMaxBounds();

  x[0]  = min[0]; x[1]  = max[1]; x[2]  = min[2];
  ids[0] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = max[1]; x[2]  = min[2];
  ids[1] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = max[1]; x[2]  = max[2];
  ids[2] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = max[1]; x[2]  = max[2];
  ids[3] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = min[1]; x[2]  = min[2];
  ids[4] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = min[1]; x[2]  = min[2];
  ids[5] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = min[1]; x[2]  = max[2];
  ids[6] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = min[1]; x[2]  = max[2];
  ids[7] = pts->InsertNextPoint(x);

  idList[0] = ids[0]; idList[1] = ids[1]; idList[2] = ids[2]; idList[3] = ids[3];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[1]; idList[1] = ids[5]; idList[2] = ids[6]; idList[3] = ids[2];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[5]; idList[1] = ids[4]; idList[2] = ids[7]; idList[3] = ids[6];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[4]; idList[1] = ids[0]; idList[2] = ids[3]; idList[3] = ids[7];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[3]; idList[1] = ids[2]; idList[2] = ids[6]; idList[3] = ids[7];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[1]; idList[1] = ids[0]; idList[2] = ids[4]; idList[3] = ids[5];
  polys->InsertNextCell(4, idList);
}

//----------------------------------------------------------------------------
int vtkOctreePointLocator::FindRegion(vtkOctreePointLocatorNode *node, float x,
                                      float y, float z)
{
  return vtkOctreePointLocator::FindRegion(
    node, static_cast<double>(x),static_cast<double>(y),static_cast<double>(z));
}

//----------------------------------------------------------------------------
int vtkOctreePointLocator::FindRegion(vtkOctreePointLocatorNode *node, double x,
                                      double y, double z)
{
  if (!node->ContainsPoint(x, y, z, 0))
  {
    return -1; // no region is found
  }

  if (node->GetChild(0) == NULL)
  {
    return node->GetID();
  }

  int regionId = -1;
  for(int i=0;i<8;i++)
  {
    regionId =
      vtkOctreePointLocator::FindRegion(node->GetChild(i), x, y, z);
    if(regionId >=0 )
    {
      return regionId;
    }
  }

  return -1; // no region is found
}

//----------------------------------------------------------------------------
int vtkOctreePointLocator::GetRegionContainingPoint(double x, double y,
                                                    double z)
{
  return vtkOctreePointLocator::FindRegion(this->Top, x, y, z);
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::FindPointsInArea(
  double* area, vtkIdTypeArray* ids, bool clearArray)
{
  if (clearArray)
  {
    ids->Reset();
  }
  this->BuildLocator();
  this->FindPointsInArea(this->Top, area, ids);
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::FindPointsInArea(
  vtkOctreePointLocatorNode* node, double* area, vtkIdTypeArray* ids)
{
  double b[6];
  node->GetBounds(b);

  if (b[0] > area[1] || b[1] < area[0] ||
    b[2] > area[3] || b[3] < area[2] ||
    b[4] > area[5] || b[5] < area[4])
  {
    return; // no intersection
  }

  bool contains = false;
  if (area[0] <= b[0] && b[1] <= area[1] &&
    area[2] <= b[2] && b[3] <= area[3] &&
    area[4] <= b[4] && b[5] <= area[5])
  {
    contains = true;
  }

  if (contains)
  {
    this->AddAllPointsInRegion(node, ids);
  }
  else // intersects
  {
    if (node->GetChild(0) == NULL)
    {
      //int regionID = node->GetID();
      int regionLoc = node->GetMinID();
      float* pt = this->LocatorPoints + (regionLoc * 3);
      vtkIdType numPoints = node->GetNumberOfPoints();
      for (vtkIdType i = 0; i < numPoints; i++)
      {
        if (area[0] <= pt[0] && pt[0] <= area[1] &&
          area[2] <= pt[1] && pt[1] <= area[3] &&
          area[4] <= pt[2] && pt[2] <= area[5])
        {
          vtkIdType ptId =
            static_cast<vtkIdType>(this->LocatorIds[regionLoc + i]);
          ids->InsertNextValue(ptId);
        }
        pt += 3;
      }
    }
    else
    {
      for(int i=0;i<8;i++)
      {
        this->FindPointsInArea(node->GetChild(i), area, ids);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::AddAllPointsInRegion(vtkOctreePointLocatorNode* node,
                                                 vtkIdTypeArray* ids)
{
  int regionLoc = node->GetMinID();
  vtkIdType numPoints = node->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numPoints; i++)
  {
    vtkIdType ptId = static_cast<vtkIdType>(this->LocatorIds[regionLoc + i]);
    ids->InsertNextValue(ptId);
  }
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::AddAllPointsInRegion(vtkOctreePointLocatorNode* node,
                                                 vtkIdList* ids)
{
  int regionLoc = node->GetMinID();
  vtkIdType numPoints = node->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numPoints; i++)
  {
    vtkIdType ptId = static_cast<vtkIdType>(this->LocatorIds[regionLoc + i]);
    ids->InsertNextId(ptId);
  }
}

//----------------------------------------------------------------------------
void vtkOctreePointLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MaximumPointsPerRegion: " <<
    this->MaximumPointsPerRegion << endl;
  os << indent << "NumberOfLeafNodes: " << this->NumberOfLeafNodes << endl;
  os << indent << "Top: " << this->Top << endl;
  os << indent << "LeafNodeList: " << this->LeafNodeList << endl;
  os << indent << "LocatorPoints: " << this->LocatorPoints << endl;
  os << indent << "NumberOfLocatorPoints: "
     << this->NumberOfLocatorPoints << endl;
  os << indent << "LocatorIds: " << this->LocatorIds << endl;
  os << indent << "FudgeFactor: " << this->FudgeFactor << endl;
  os << indent << "MaxWidth: " << this->MaxWidth << endl;
  os << indent << "CreateCubicOctants: " << this->CreateCubicOctants << endl;
}
