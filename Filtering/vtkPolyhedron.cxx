/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyhedron.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyhedron.h"

#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedTriangulator.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkQuad.h"
#include "vtkPolygon.h"
#include "vtkLine.h"
#include "vtkEdgeTable.h"
#include "vtkPolyData.h"
#include "vtkCellLocator.h"
#include "vtkGenericCell.h"
#include "vtkPointLocator.h"
#include "vtkMeanValueCoordinatesInterpolator.h"
#include "vtkSmartPointer.h"
#include "vtkMergePoints.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"

#include <vtkstd/set>
#include <vtkstd/list>
#include <limits>

vtkStandardNewMacro(vtkPolyhedron);

// Special typedef for point id map
struct vtkPointIdMap : public vtkstd::map<vtkIdType,vtkIdType> {};
typedef vtkstd::map<vtkIdType,vtkIdType*>::iterator PointIdMapIterator;

// Special class for iterating through polyhedron faces
//----------------------------------------------------------------------------
class vtkPolyhedronFaceIterator
{
public:
  vtkIdType CurrentPolygonSize;
  vtkIdType *Polygon;
  vtkIdType *Current;
  vtkIdType NumberOfPolygons;
  vtkIdType Id;

  vtkPolyhedronFaceIterator(vtkIdType numFaces, vtkIdType *t)
    {
      this->CurrentPolygonSize = t[0];
      this->Polygon = t;
      this->Current = t+1;
      this->NumberOfPolygons = numFaces;
      this->Id = 0;
    }
  vtkIdType* operator++()
    {
      this->Current += this->CurrentPolygonSize + 1;
      this->Polygon = this->Current - 1;
      this->CurrentPolygonSize = *(Polygon);
      this->Id++;
      return this->Current;
    }
};


// Special class for iterating through vertices on a polygon face
//----------------------------------------------------------------------------
class vtkPolygonVertexIterator
{
public:
  vtkIdType *Current;
  vtkIdType NumberOfVertices;
  vtkIdType Id;
  // 1 or 0 for iterating along its original direction or reverse
  vtkIdType IterDirection; 

  vtkPolygonVertexIterator(vtkIdType numVertices, vtkIdType startVertex, 
                           vtkIdType *startVertexPointer, vtkIdType nextVertex)
    {
    this->Current = startVertexPointer;
    this->NumberOfVertices = numVertices;
    this->Id = startVertex;
    this->IterDirection = 1;
    vtkIdType nextId = this->Id + 1;
    vtkIdType *next = this->Current + 1;
    if (nextId == this->NumberOfVertices)
      {
      next -= this->NumberOfVertices;
      }
    if (*next != nextVertex)
      {
      this->IterDirection = 0;
      }
    }

  vtkIdType* operator++()
    {
    if (this->IterDirection)
      {
      this->Id++;
      this->Current++;
      if (this->Id == this->NumberOfVertices)
        {
        this->Id = 0;
        this->Current -= this->NumberOfVertices;
        }
      }
    else
      {
      this->Id--;
      this->Current--;
      if (this->Id == -1)
        {
        this->Id = this->NumberOfVertices - 1;
        this->Current += this->NumberOfVertices;
        }
      }
    return this->Current;
    }
};

//----------------------------------------------------------------------------
namespace
{
// insert new id element in between two existing adjacent id elements.
// this is a convenient function. no check whether the input elements 
// exist in the vector. no check for element adjacency.
int InsertNewIdToIdVector(vtkPolyhedron::IdVectorType & idVector, 
                          vtkIdType id, vtkIdType id0, vtkIdType id1)
{
  if (idVector.size() < 2)
    {
    return 0;
    }
  
  size_t num = idVector.size();
  if ((idVector[0] == id0 && idVector[num-1] == id1)
    ||(idVector[0] == id1 && idVector[num-1] == id0))
    {
    idVector.push_back(id);
    return 1;
    }
  
  vtkPolyhedron::IdVectorType::iterator iter = idVector.begin();
  for (; iter != idVector.end(); ++iter)
    {
    if (*iter == id0 || *iter == id1)
      {
      ++iter;
      idVector.insert(iter, id);
      return 1;
      }
    }
  
  return 0;
}

// Convinient function used by clip. The id is the vector index of the positive 
// point, id0 is the vector index of the start point, and id1 is the vector index
// of the end point.
//----------------------------------------------------------------------------
int EraseSegmentFromIdVector(vtkPolyhedron::IdVectorType & idVector, 
                             vtkIdType id, vtkIdType id0, vtkIdType id1)
{
  // three possible cases
  // first case: 0 -- id0 -- id -- id1 -- size-1
  if (id0 < id && id < id1)
    {
    idVector.erase(idVector.begin() + id0 + 1, idVector.begin() + id1);
    }
  // second case: 0 -- id1 -- id0 -- id -- size-1
  // third case: 0 -- id -- id1 -- id0 -- size-1
  else if (id1 < id0 && (id0 < id || id < id1))
    {
    idVector.erase(idVector.begin() + id0 + 1, idVector.end());
    idVector.erase(idVector.begin(), idVector.begin() + id1);
    }
  else
    {
    // we should never get here.
    return 0;
    }
  return 1;
}

// convert the point ids from map.first to map.second
//----------------------------------------------------------------------------
int ConvertPointIds(vtkIdType npts, vtkIdType * pts, 
                    vtkPolyhedron::IdToIdMapType & map,
                    vtkIdType reverse = 0)
{
  for (vtkIdType i = 0; i < npts; i++)
    {
    vtkIdType id = reverse ? npts-1-i : i;
    vtkPolyhedron::IdToIdMapType::iterator iter = map.find(pts[id]);
    if (iter == map.end())
      {
      return 0;
      }
    pts[id] = iter->second;
    }
  return 1;
}

//----------------------------------------------------------------------------
// The connected contour points are found by (1) locating the current
// contour point in the face loop, (2) looping through face point: 
//  meet a positive point, keep going. 
//  meet a contour point, store it and stop marching in this direction.
//  meet a negative point, stop marching in this direction.
//  meet the same point from both directions, stop. 
// This loop may find zero, one or two connected contour points.
void FindConnectedContourPointsOnFace(vtkPolyhedron::IdVectorType & facePtsVector, 
                                      vtkPolyhedron::IdVectorType & faceContourPtsVec,
                                      vtkIdType currContourPoint,
                                      vtkPolyhedron::IdVectorType & pointLabelVec,
                                      vtkPolyhedron::IdSetType & connectedContourPtsSet,
                                      vtkPolyhedron::IdSetType & unConnectedContourPtsSet)
{
  vtkIdType numFacePoints = static_cast<vtkIdType>(facePtsVector.size());
  if (numFacePoints < 3)
    {
    return;
    }
  if (faceContourPtsVec.size() < 2)
    {
    return;
    }
  // locate the id of the startContourPt inside the face loop
  vtkIdType startPt = -1;
  for (vtkIdType i = 0; i < numFacePoints; i++)
    {
    if (currContourPoint == facePtsVector[i])
      {
      startPt = i;
      break;
      }
    }

  if (startPt < 0 || startPt >= numFacePoints)
    {
    return;
    }

  vtkIdType leftEndPt = -1; // face loop index
  vtkIdType rightEndPt = -1; // face loop index
  vtkIdType leftEndPoint = -1; // point id
  vtkIdType rightEndPoint = -1; // point id
  vtkIdType leftEndPassPositivePoint = 0;
  vtkIdType rightEndPassPositivePoint = 0;
  // search in one direction.
  vtkIdType endPt = startPt - 1;
  for (; endPt != startPt; endPt--)
    {
    if (endPt < 0)
      {
      endPt = numFacePoints - 1;
      if (endPt == startPt)
        {
        break;
        }
      }
    if (pointLabelVec[facePtsVector[endPt]] == -1)//negative point reached. stop
      {
      break;
      }
    else if (pointLabelVec[facePtsVector[endPt]] == 0)//contour pt reached. stop
      {
      leftEndPt = endPt;
      leftEndPoint = facePtsVector[endPt];
      break;
      }
    else
      {
      leftEndPassPositivePoint = 1;
      }
    // positive pt reached. continue.
    }
  
  // check if already loop through the entire face
  if (endPt != startPt)
    {
    vtkIdType prevEndPt = endPt;
    
    // search in the other direction
    for (endPt = startPt + 1; endPt != prevEndPt; endPt++)
      {
      if (endPt > numFacePoints - 1)
        {
        endPt = 0;
        if (endPt == prevEndPt)
          {
          break;
          }
        if (endPt == startPt)
          {
          break;
          }
        }
      if (pointLabelVec[facePtsVector[endPt]] == -1)//negative point reached. stop
        {
        break;
        }
      else if (pointLabelVec[facePtsVector[endPt]] == 0)//contour pt reached. stop
        {
        rightEndPt = endPt;
        rightEndPoint = facePtsVector[endPt];
        break;
        }
      else
        {
        rightEndPassPositivePoint = 1;
        }
      }
    }

  // need to check a special case where startPt, leftEndPoint and rightEndPoint
  // are directly connected or connected by a series of other contour points, 
  // and startPt is at one end of the contour strip. We can check this situation
  // using leftEndPassPositivePoint and leftEndPassPositivePoint. If both are
  // 1, then the three points are not on a contour strip. If both are 0, then
  // startPt is not at one end of the contour strip.
  if (leftEndPoint >= 0 && rightEndPoint >=0 && leftEndPoint != rightEndPoint)
    {
    if (leftEndPassPositivePoint != rightEndPassPositivePoint)
      {
      bool foundNonContourPoint = false;
      for (endPt = leftEndPt - 1; endPt != rightEndPt; endPt--)
        {
        if (endPt < 0)
          {
          endPt = numFacePoints - 1;
          if (endPt == rightEndPt)
            {
            break;
            }
          }
        if (pointLabelVec[facePtsVector[endPt]] != 0)
          {
          foundNonContourPoint = true;
          break;
          }
        }
      if (!foundNonContourPoint)// startPt on one end of the contour strip
        {
        if (leftEndPassPositivePoint)
          {
          leftEndPoint = -1;
          }
        else
          {
          rightEndPoint = -1;
          }
        }
      }
    }
  
  if (leftEndPoint >= 0)
    {
    connectedContourPtsSet.insert(leftEndPoint);
    }
  if (rightEndPoint >= 0)
    {
    connectedContourPtsSet.insert(rightEndPoint);
    }
  for (size_t i = 0; i < faceContourPtsVec.size(); i++)
    {
    if (faceContourPtsVec[i] != leftEndPoint && 
        faceContourPtsVec[i] != rightEndPoint &&
        faceContourPtsVec[i] != currContourPoint)
      {
      unConnectedContourPtsSet.insert(faceContourPtsVec[i]);
      }
    }
}

//----------------------------------------------------------------------------
void RemoveIdFromIdToIdVectorMap(vtkPolyhedron::IdToIdVectorMapType & map, 
                                 vtkIdType id)
{
  vtkPolyhedron::IdToIdVectorMapIteratorType mit = map.begin();
  for (; mit != map.end(); ++mit)
    {
    vtkPolyhedron::IdVectorType::iterator vit = mit->second.begin();
    for (; vit != mit->second.end(); ++vit)
      {
      if ((*vit) == id)
        {
        mit->second.erase(vit);
        break;
        }
      }
    }
}

//----------------------------------------------------------------------------
// For each contour point, extract its adjacent faces, then extract other 
// contour points on the same face that can be connected to the current
// points.
// The connected contour points are found by (1) locating the current
// contour point in the face loop, (2) looping through face point: 
//  meet a positive point, keep going. 
//  meet a contour point, store it and stop marching in this direction.
//  meet a negative point, stop marching in this direction.
//  meet the same point from both directions, stop. 
// This loop may find zero, one or two connected contour points.
int  ExtractContourConnectivities(
                 vtkPolyhedron::IdToIdVectorMapType & ceMap,
                 vtkPolyhedron::IdVectorType & isBranchPointVector,
                 vtkPolyhedron::IdSetType & cpSet, 
                 vtkPolyhedron::IdVectorType & pointLabelVector, 
                 vtkPolyhedron::IdToIdVectorMapType & pointToFacesMap, 
                 vtkPolyhedron::IdToIdVectorMapType & faceToPointsMap, 
                 vtkPolyhedron::IdToIdVectorMapType & faceToContourPointsMap)
{
  int maxConnectivity = 0;
  if (cpSet.empty())
    {
    return 0;
    }
  
  vtkPolyhedron::IdSetType contourBranchesSet;
  vtkPolyhedron::IdSetType nonContourBranchesSet;
  vtkPolyhedron::IdVectorType contourBranchesVector;
  vtkPolyhedron::IdSetType::iterator cpSetIt;
  vtkPolyhedron::IdToIdVectorMapType::iterator fcpMapIt, fvMapIt, ceMapIt, ceMapIt1;
  for (cpSetIt = cpSet.begin(); cpSetIt != cpSet.end(); /*manual increment*/)
    {
    contourBranchesSet.clear();
    nonContourBranchesSet.clear();
    contourBranchesVector.clear();
    vtkIdType pid = *cpSetIt;
    vtkPolyhedron::IdVectorType fVector = pointToFacesMap.find(pid)->second;
    for (size_t i = 0; i < fVector.size(); i++)
      {
      // find adjacent faces that contain contour points
      fcpMapIt = faceToContourPointsMap.find(fVector[i]);
      if (fcpMapIt == faceToContourPointsMap.end())
        {
        continue;
        }
      fvMapIt = faceToPointsMap.find(fVector[i]);
      if (fvMapIt == faceToPointsMap.end())
        {
        std::cout << "Cannot find point ids of a face. We should never get "
                     "here. Contouring aborted." << std::endl;;
        return 0;
        }


      // find connected contour points and store them in the set. Notice that
      // some weird topology will classify a point as a connected contour point
      // in one face and a non-connected contour point in some other face. we
      // will extract the union.
      FindConnectedContourPointsOnFace(fvMapIt->second, fcpMapIt->second, pid, 
                  pointLabelVector, contourBranchesSet, nonContourBranchesSet);
      }

    if (!contourBranchesSet.empty())
      {
      vtkPolyhedron::IdSetType::iterator ccpSetIt = contourBranchesSet.begin();
      for (; ccpSetIt != contourBranchesSet.end(); ++ccpSetIt)
        {
        if (nonContourBranchesSet.find(*ccpSetIt) == nonContourBranchesSet.end())
          {
          contourBranchesVector.push_back(*ccpSetIt);
          }
        }
      }

    if (contourBranchesVector.size() >= 2)
      {
      ceMap.insert(
        vtkPolyhedron::IdToIdVectorPairType(pid, contourBranchesVector));
      ++cpSetIt;
      }
    else // throw away point contour or edge contour.
      {
      if (cpSetIt != cpSet.begin())
        {
        vtkPolyhedron::IdSetType::iterator tempIt = cpSetIt;
        --cpSetIt;
        cpSet.erase(tempIt);
        ++cpSetIt;
        }
      else
        {
        cpSet.erase(cpSetIt);
        cpSetIt = cpSet.begin();
        }
      }
    }
  
  // sanity check, all edges should be listed twice
  for (ceMapIt = ceMap.begin(); ceMapIt != ceMap.end(); ++ceMapIt)
    {
    vtkPolyhedron::IdVectorType edges = ceMapIt->second;
    for (size_t i = 0; i < edges.size(); i++)
      {
      bool foundMatch = false;
      ceMapIt1 = ceMap.find(edges[i]);
      if (ceMapIt1 != ceMap.end())
        {
        for (size_t j = 0; j < ceMapIt1->second.size(); j++)
          {
          if (ceMapIt->first == ceMapIt1->second[j])
            {
            foundMatch = true;
            break;
            }
          }
        }
      if (!foundMatch)
        {
        edges.erase(edges.begin()+i);
        i--;
        }
      }
    ceMapIt->second = edges;
    }

  // clean 0 or 1-connected contour from ceMap
  for (ceMapIt = ceMap.begin(); ceMapIt != ceMap.end(); /*manual increment*/)
    {
    if (ceMapIt->second.size() >= 2)
      {
      ++ceMapIt;
      continue;
      }
      
    cpSetIt = cpSet.find(ceMapIt->first);
    if (cpSetIt != cpSet.end())
      {
      cpSet.erase(cpSetIt);
      }
    
    if (ceMapIt != ceMap.begin())
      {
      vtkPolyhedron::IdToIdVectorMapType::iterator tempIt = ceMapIt;
      --ceMapIt;
      ceMap.erase(tempIt);
      ++ceMapIt;
      }
    else
      {
      ceMap.erase(ceMapIt);
      ceMapIt = ceMap.begin();
      }
    }
  
  // set isBranchPointVector and maxConnectivity.
  for (ceMapIt = ceMap.begin(); ceMapIt != ceMap.end(); ++ceMapIt)
    {
    if (ceMapIt->second.size() > 2)
      {
      isBranchPointVector[ceMapIt->first] = 1;
      }

    if (static_cast<int>(ceMapIt->second.size()) > maxConnectivity)
      {
      maxConnectivity = ceMapIt->second.size();
      }
    }

  return maxConnectivity;
}

//----------------------------------------------------------------------------
// Use eigenvalues to determine the dimension of the input contour points.
// This chunk of code is mostly copied from vtkOBBTree::ComputeOBB()
// Function return 0 if input is a single point, 1 if co-linear, 
// 2 if co-planar, 3 if 3D
int CheckContourDimensions(vtkPoints* points, vtkIdType npts, vtkIdType * ptIds)
{
  static const double eigenvalueRatioThresh = 0.001;
  
  if (npts < 3)
    {
    return npts - 1;
    }
  
  vtkIdType i, j;
  double x[3], mean[3], xp[3], *v[3], v0[3], v1[3], v2[3];
  double *a[3], a0[3], a1[3], a2[3], eigValue[3];

  // Compute mean
  mean[0] = mean[1] = mean[2] = 0.0;
  for (i=0; i < npts; i++ )
    {
    points->GetPoint(ptIds[i], x);
    mean[0] += x[0];
    mean[1] += x[1];
    mean[2] += x[2];
    }
  for (i=0; i < 3; i++)
    {
    mean[i] /= npts;
    }

  // Compute covariance matrix
  a[0] = a0; a[1] = a1; a[2] = a2; 
  for (i=0; i < 3; i++)
    {
    a0[i] = a1[i] = a2[i] = 0.0;
    }

  for (j = 0; j < npts; j++ )
    {
    points->GetPoint(ptIds[j], x);
    xp[0] = x[0] - mean[0]; xp[1] = x[1] - mean[1]; xp[2] = x[2] - mean[2];
    for (i = 0; i < 3; i++)
      {
      a0[i] += xp[0] * xp[i];
      a1[i] += xp[1] * xp[i];
      a2[i] += xp[2] * xp[i];
      }
    }//for all points

  for (i=0; i < 3; i++)
    {
    a0[i] /= npts;
    a1[i] /= npts;
    a2[i] /= npts;
    }

  // Extract axes (i.e., eigenvectors) from covariance matrix. 
  v[0] = v0; v[1] = v1; v[2] = v2; 
  vtkMath::Jacobi(a,eigValue,v);

  int ret = 3;
  
  if ((eigValue[2] / eigValue[0]) < eigenvalueRatioThresh)
    {
    ret--;
    }
  if ((eigValue[1] / eigValue[0]) < eigenvalueRatioThresh)
    {
    ret--;
    }
  
  return ret;
}

//----------------------------------------------------------------------------
// Compute the normal (pointing to the positive side) for each contour point, 
// then sort the contour edges such that they are ordered contour-clockwise
// when viewed from normal direction. 
// Current implementation of this function assumes planar contours, we only 
// compute normal once and reuse it for all other contour points.
// TODO: for non-planar cut, need to compute normal for each contour point. We 
// then project edges onto a tangent plane and sort them.
void OrderConnectedContourPoints(
                      vtkPolyhedron::IdToIdVectorMapType & cpMap,
                      vtkPolyhedron::IdVectorType isBranchPointVector,
                      vtkPoints * points,
                      vtkPolyhedron::IdVectorType & pointLabelVector)
{
  
  double o[3], x0[3], x1[3], x2[3], e0[3], e1[3], n[3], nn[3];
  vtkPolyhedron::IdToIdVectorMapType::iterator mapIt;
  for (mapIt = cpMap.begin(); mapIt != cpMap.end(); ++mapIt)
    {
    vtkPolyhedron::IdVectorType pids = mapIt->second;
    pids.insert(pids.begin(), mapIt->first);
    if (CheckContourDimensions(points, 3, &(pids[0])) > 1)
      {
      points->GetPoint(pids[0], o);
      points->GetPoint(pids[1], x0);
      points->GetPoint(pids[2], x1);
      e0[0] = x0[0] - o[0];
      e0[1] = x0[1] - o[1];
      e0[2] = x0[2] - o[2];
      e1[0] = x1[0] - o[0];
      e1[1] = x1[1] - o[1];
      e1[2] = x1[2] - o[2];
      vtkMath::Cross(e0, e1, n);
      vtkMath::Normalize(n);
      
      // make sure n points to the positive side
      vtkIdType numPoints = static_cast<vtkIdType>(pointLabelVector.size());
      for (vtkIdType i = 0; i < numPoints; i++)
        {
        if (pointLabelVector[i] == 1)
          {
          points->GetPoint(i, x0);
          }
        e0[0] = x0[0] - o[0];
        e0[1] = x0[1] - o[1];
        e0[2] = x0[2] - o[2];
        if (vtkMath::Dot(e0, n) < 0)
          {
          n[0] = -n[0];
          n[1] = -n[1];
          n[2] = -n[2];
          }
        }
      }
    }
  
  // now loop over contour points to order edges. note: we don't need to order
  // the two edges of a 2-connected points.
  for (mapIt = cpMap.begin(); mapIt != cpMap.end(); ++mapIt)
    {
    vtkPolyhedron::IdVectorType edges = mapIt->second;

    if (edges.size() < 3)
      {
      continue;
      }

    // for branching point, there are two cases: 
    // (1) complete interior contour point with no boundary edge. 
    // (2) boundary contour point with two boundary edges.
    // for case (2), we need to find the outgoing boundary edge.
    vtkPolyhedron::IdVectorType boundaryEdges;
    vtkPolyhedron::IdVectorType nonBoundaryEdges;
    for (size_t i = 0; i < edges.size(); i++)
      {
      if (!isBranchPointVector[edges[i]])
        {
        boundaryEdges.push_back(edges[i]);
        }
      else
        {
        nonBoundaryEdges.push_back(edges[i]);
        }
      }
    if (boundaryEdges.size() != 0 && boundaryEdges.size() != 2)
      {
      std::cout << "Find a contour point with " << boundaryEdges.size() << 
        " boundary edges. This should never happen." << std::endl;
      return;
      }

    // we use the observation: normalize the two boundary edges and the selected
    // non-boundary edge. compute a new vector starting from the end of the 
    // normalized incoming boundary edge and ending at the end of the normalized
    // outgoing boundary edge. the end point of the normalized non-boundary edge
    // should be to the left of the new vector. this property holds for both
    // convex and concave cases.
    points->GetPoint(mapIt->first, o);
    if (!boundaryEdges.empty())
      {
      points->GetPoint(boundaryEdges[0], x0);
      x0[0] = x0[0] - o[0];
      x0[1] = x0[1] - o[1];
      x0[2] = x0[2] - o[2];
      vtkMath::Normalize(x0);
      points->GetPoint(boundaryEdges[1], x1);
      x1[0] = x1[0] - o[0];
      x1[1] = x1[1] - o[1];
      x1[2] = x1[2] - o[2];
      vtkMath::Normalize(x1);
      points->GetPoint(nonBoundaryEdges[0], x2);
      x2[0] = x2[0] - o[0];
      x2[1] = x2[1] - o[1];
      x2[2] = x2[2] - o[2];
      vtkMath::Normalize(x2);
      e0[0] = x1[0] - x0[0];
      e0[1] = x1[1] - x0[1];
      e0[2] = x1[2] - x0[2];
      e1[0] = x2[0] - x0[0];
      e1[1] = x2[1] - x0[1];
      e1[2] = x2[2] - x0[2];
      vtkMath::Cross(e0, e1, nn);
      if (vtkMath::Dot(n, nn) > 0)
        {
        vtkIdType temp = boundaryEdges[0];
        boundaryEdges[0] = boundaryEdges[1];
        boundaryEdges[1] = temp;
        }
      // set the outgoing and incoming boundary edges at the beginning and 
      edges.clear();
      edges.push_back(boundaryEdges[0]);
      for (size_t i = 0; i < nonBoundaryEdges.size(); i++)
        {
        edges.push_back(nonBoundaryEdges[i]);
        }
      edges.push_back(boundaryEdges[1]);
      }
    
    if (edges.size() == 3)
      {
      mapIt->second = edges;
      continue;
      }
    
    // get the first edge
    points->GetPoint(mapIt->first, o);
    points->GetPoint(edges[0], x0);
    e0[0] = x0[0] - o[0];
    e0[1] = x0[1] - o[1];
    e0[2] = x0[2] - o[2];
    vtkMath::Normalize(e0);

    // compute the angles from other edges to the first edge
    std::vector<double> angles;
    angles.push_back(0);
    const double maxDotProduct = 0.95;
    size_t endId = boundaryEdges.empty() ? edges.size() : edges.size() - 1;
    for (size_t i = 1; i < endId; i++)
      {
      points->GetPoint(edges[i], x1);
      e1[0] = x1[0] - o[0];
      e1[1] = x1[1] - o[1];
      e1[2] = x1[2] - o[2];
      vtkMath::Normalize(e1);
      double dotproduct = vtkMath::Dot(e0, e1);
      double angle = acos(dotproduct);
      if (dotproduct < maxDotProduct && dotproduct > -maxDotProduct)
        {
        vtkMath::Cross(e0, e1, nn);
        if (vtkMath::Dot(n, nn) < 0)
          {
          angle += vtkMath::Pi();
          }
        }
      else if (dotproduct > maxDotProduct)
        {
        vtkMath::Cross(e0, n, nn);
        angle = acos(vtkMath::Dot(nn, e1)) - vtkMath::Pi()/2.0;
        }
      else
        {
        vtkMath::Cross(n, e0, nn);
        angle = acos(vtkMath::Dot(nn, e1)) + vtkMath::Pi()/2.0;
        }
      angles.push_back(angle);
      }
    
    // sort connected contour points 
    for (size_t i = 1; i < endId-1; i++)
      {
      for (size_t j = i+1; j < endId; j++)
        {
        if (angles[i] > angles[j])
          {
          vtkIdType temp = edges[i];
          edges[i] = edges[j];
          edges[j] = temp;
          double angle = angles[i];
          angles[i] = angles[j];
          angles[j] = angle;
          }
        }
      }
    
    mapIt->second = edges;
    }
}

//----------------------------------------------------------------------------
// Note: the triangulation results are inserted into the input cellArray, which
// does not need to be empty.
void Triangulate3DContour(vtkIdType npts, vtkIdType * pts, 
                          vtkCellArray *cellArray)
{
  vtkIdType start = 0;
  vtkIdType end = npts-1;
  vtkIdType ids[3];
  
  while (start < end)
    {
    ids[0] = pts[start++];
    ids[1] = pts[start];
    ids[2] = pts[end];
    cellArray->InsertNextCell(3, ids);

    if (start >= end - 1)
      {
      return;
      }
    
    ids[0] = pts[end];
    ids[1] = pts[start];
    ids[2] = pts[--end];
    cellArray->InsertNextCell(3, ids);
    }
}

} //end namespace


//----------------------------------------------------------------------------
// Construct the hexahedron with eight points.
vtkPolyhedron::vtkPolyhedron()
{
  this->Line = vtkLine::New();
  this->Triangle = vtkTriangle::New();
  this->Quad = vtkQuad::New();
  this->Polygon = vtkPolygon::New();
  this->Tetra = vtkTetra::New();
  this->GlobalFaces = vtkIdTypeArray::New();
  this->FaceLocations = vtkIdTypeArray::New();
  this->PointIdMap = new vtkPointIdMap;
  
  this->EdgesGenerated = 0;
  this->EdgeTable = vtkEdgeTable::New();
  this->Edges = vtkIdTypeArray::New();
  this->Edges->SetNumberOfComponents(2);

  this->FacesGenerated = 0;
  this->Faces = vtkIdTypeArray::New();

  this->BoundsComputed = 0;
  
  this->PolyDataConstructed = 0;
  this->PolyData = vtkPolyData::New();
  this->Polys = vtkCellArray::New();
  this->PolyConnectivity = vtkIdTypeArray::New();
  this->LocatorConstructed = 0;
  this->CellLocator = vtkCellLocator::New();
  this->CellIds = vtkIdList::New();
  this->Cell = vtkGenericCell::New();
}

//----------------------------------------------------------------------------
vtkPolyhedron::~vtkPolyhedron()
{
  this->Line->Delete();
  this->Triangle->Delete();
  this->Quad->Delete();
  this->Polygon->Delete();
  this->Tetra->Delete();
  this->GlobalFaces->Delete();
  this->FaceLocations->Delete();
  delete this->PointIdMap;
  this->EdgeTable->Delete();
  this->Edges->Delete();
  this->Faces->Delete();
  this->PolyData->Delete();
  this->Polys->Delete();
  this->PolyConnectivity->Delete();
  this->CellLocator->Delete();
  this->CellIds->Delete();
  this->Cell->Delete();
}

//----------------------------------------------------------------------------
void vtkPolyhedron::ComputeBounds()
{
  if ( this->BoundsComputed )
    {
    return;
    }

  this->Superclass::GetBounds(); //stored in this->Bounds
  this->BoundsComputed = 1;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::ConstructPolyData()
{
  if (this->PolyDataConstructed)
    {
    return;
    }
  
  // Here's a trick, we're going to use the Faces array as the connectivity 
  // array. Note that the Faces have an added nfaces value at the beginning
  // of the array. Other than that,it's a vtkCellArray. So we play games 
  // with the pointers.
  this->GenerateFaces();
  
  this->PolyConnectivity->
    SetArray(this->Faces->GetPointer(1), this->Faces->GetSize()-1, 1);
  this->Polys->
    SetCells(*(this->Faces->GetPointer(0)), this->PolyConnectivity);

  // Standard setup
  this->PolyData->SetPoints(this->Points);
  this->PolyData->SetPolys(this->Polys);

  this->PolyDataConstructed = 1;
}

vtkPolyData* vtkPolyhedron::GetPolyData()
{
  if (!this->PolyDataConstructed)
    {
    this->ConstructPolyData();
    }
  
  return this->PolyData;
}
//----------------------------------------------------------------------------
void vtkPolyhedron::ConstructLocator()
{
  if (this->LocatorConstructed)
    {
    return;
    }
  
  this->ConstructPolyData();
  
  // With the polydata set up, we can assign it to the  locator
  this->CellLocator->SetDataSet(this->PolyData);
  this->CellLocator->BuildLocator();

  this->LocatorConstructed = 1;
}


//----------------------------------------------------------------------------
void vtkPolyhedron::ComputeParametricCoordinate(double x[3], double pc[3])
{
  this->ComputeBounds();
  double *bounds = this->Bounds;

  pc[0] = (x[0] - bounds[0]) / (bounds[1] - bounds[0]);
  pc[1] = (x[1] - bounds[2]) / (bounds[3] - bounds[2]);
  pc[2] = (x[2] - bounds[4]) / (bounds[5] - bounds[4]);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::
ComputePositionFromParametricCoordinate(double pc[3], double x[3])
{
  this->ComputeBounds();
  double *bounds = this->Bounds;
  x[0] = ( 1 - pc[0] )* bounds[0] + pc[0] * bounds[1];
  x[1] = ( 1 - pc[1] )* bounds[2] + pc[1] * bounds[3];
  x[2] = ( 1 - pc[2] )* bounds[4] + pc[2] * bounds[5];
}

//----------------------------------------------------------------------------
// Should be called by GetCell() prior to any other method invocation and after the
// points, point ids, and faces have been loaded. 
void vtkPolyhedron::Initialize()
{
  // Clear out any remaining memory.
  this->PointIdMap->clear();
  
  // We need to create a reverse map from the point ids to their canonical cell
  // ids. This is a fancy way of saying that we have to be able to rapidly go
  // from a PointId[i] to the location i in the cell.
  vtkIdType i, id, numPointIds = this->PointIds->GetNumberOfIds();
  for (i=0; i < numPointIds; ++i)
    {
    id = this->PointIds->GetId(i);
    (*this->PointIdMap)[id] = i;
    }
  
  // Edges have to be reset
  this->EdgesGenerated = 0;
  this->EdgeTable->Reset();
  this->Edges->Reset();
  this->Faces->Reset();
  
  // Faces may need renumbering later. This means converting the face ids from
  // global ids to local, canonical ids.
  this->FacesGenerated = 0;

  // No bounds have been computed as of yet.
  this->BoundsComputed = 0;
  
  // No supplemental geometric stuff created
  this->PolyDataConstructed = 0;
  this->LocatorConstructed = 0;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::GetNumberOfEdges()
{
  // Make sure edges have been generated.
  if ( ! this->EdgesGenerated )
    {
    this->GenerateEdges();
    }
  
  return this->Edges->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
// This method requires that GenerateEdges() is invoked beforehand.
vtkCell *vtkPolyhedron::GetEdge(int edgeId)
{
  // Make sure edges have been generated.
  if ( ! this->EdgesGenerated )
    {
    this->GenerateEdges();
    }
  
  // Make sure requested edge is within range
  vtkIdType numEdges = this->Edges->GetNumberOfTuples();

  if ( edgeId < 0 || edgeId >= numEdges )
    {
    return NULL;
    }
  
  // Return the requested edge
  vtkIdType edge[2];
  this->Edges->GetTupleValue(edgeId,edge);

  // Recall that edge tuples are stored in canonical numbering
  for (int i=0; i<2; i++)
    {
    this->Line->PointIds->SetId(i,this->PointIds->GetId(edge[i]));
    this->Line->Points->SetPoint(i,this->Points->GetPoint(edge[i]));
    }

  return this->Line;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::GenerateEdges()
{
  if ( this->EdgesGenerated )
    {
    return this->Edges->GetNumberOfTuples();
    }

  //check the number of faces and return if there aren't any
  if ( this->GlobalFaces->GetValue(0) <= 0 ) 
    {
    return 0;
    }
  
  // Loop over all faces, inserting edges into the table
  vtkIdType *faces = this->GlobalFaces->GetPointer(0);
  vtkIdType nfaces = faces[0];
  vtkIdType *face = faces + 1;
  vtkIdType fid, i, edge[2], npts;

  this->EdgeTable->InitEdgeInsertion(this->Points->GetNumberOfPoints());
  for (fid=0; fid < nfaces; ++fid)
    {
    npts = face[0];
    for (i=1; i <= npts; ++i)
      {
      edge[0] = (*this->PointIdMap)[face[i]];
      edge[1] = (*this->PointIdMap)[(i != npts ? face[i+1] : face[1])];
      if ( this->EdgeTable->IsEdge(edge[0],edge[1]) == (-1) )
        {
        this->EdgeTable->InsertEdge(edge[0],edge[1]);
        this->Edges->InsertNextTupleValue(edge);
        }
      }
    face += face[0] + 1;
    } //for all faces

  // Okay all done
  this->EdgesGenerated = 1;
  return this->Edges->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
int vtkPolyhedron::GetNumberOfFaces()
{
  return this->GlobalFaces->GetValue(0);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::GenerateFaces()
{
  if ( this->FacesGenerated )
    {
    return;
    }

  // Basically we just ron through the faces and change the global ids to the
  // canonical ids using the PointIdMap.
  this->Faces->SetNumberOfTuples(this->GlobalFaces->GetNumberOfTuples());
  vtkIdType *gFaces = this->GlobalFaces->GetPointer(0);
  vtkIdType *faces = this->Faces->GetPointer(0);
  vtkIdType nfaces = gFaces[0]; faces[0] = nfaces;
  vtkIdType *gFace = gFaces + 1;
  vtkIdType *face = faces + 1;
  vtkIdType fid, i, id, npts;

  for (fid=0; fid < nfaces; ++fid)
    {
    npts = gFace[0];
    face[0] = npts;
    for (i=1; i <= npts; ++i)
      {
      id = (*this->PointIdMap)[gFace[i]];
      face[i] = id;
      }
    gFace += gFace[0] + 1;
    face += face[0] + 1;
    } //for all faces


  // Okay we've done the deed
  this->FacesGenerated = 1;
}

//----------------------------------------------------------------------------
vtkCell *vtkPolyhedron::GetFace(int faceId)
{
  if ( faceId < 0 || faceId >= this->GlobalFaces->GetValue(0) )
    {
    return NULL;
    }
  
  this->GenerateFaces();

  // Okay load up the polygon
  vtkIdType i, p, loc = this->FaceLocations->GetValue(faceId);
  vtkIdType *face = this->GlobalFaces->GetPointer(loc);
  
  this->Polygon->PointIds->SetNumberOfIds(face[0]);
  this->Polygon->Points->SetNumberOfPoints(face[0]);
    
  // grab faces in global id space
  for (i=0; i < face[0]; ++i)
    {
    this->Polygon->PointIds->SetId(i,face[i+1]);
    p = (*this->PointIdMap)[face[i+1]];
    this->Polygon->Points->SetPoint(i,this->Points->GetPoint(p));
    }

  return this->Polygon;
}

//----------------------------------------------------------------------------
// Specify the faces for this cell.
void vtkPolyhedron::SetFaces(vtkIdType *faces)
{
  // Set up face structure
  this->GlobalFaces->Reset();
  this->FaceLocations->Reset();

  if (!faces)
    {
    return;
    }
    
  vtkIdType nfaces = faces[0];
  this->FaceLocations->SetNumberOfValues(nfaces);

  this->GlobalFaces->InsertNextValue(nfaces);
  vtkIdType *face = faces + 1;
  vtkIdType faceLoc = 1;
  vtkIdType i, fid, npts;

  for (fid=0; fid < nfaces; ++fid)
    {
    npts = face[0];
    this->GlobalFaces->InsertNextValue(npts);
    for (i=1; i<=npts; ++i)
      {
      this->GlobalFaces->InsertNextValue(face[i]);
      }
    this->FaceLocations->SetValue(fid,faceLoc);

    faceLoc += face[0] + 1;
    face = faces + faceLoc;
    } //for all faces
}

//----------------------------------------------------------------------------
// Return the list of faces for this cell.
vtkIdType *vtkPolyhedron::GetFaces()
{
  return this->GlobalFaces->GetPointer(0);
}

//----------------------------------------------------------------------------
int vtkPolyhedron::IntersectWithLine(double p1[3], double p2[3], double tol, 
                                     double& tMin, double xMin[3], 
                                     double pc[3], int& subId)
{
  // It's easiest if this is done in canonical space
  this->GenerateFaces();

  // Loop over all the faces, intersecting them in turn.
  vtkIdType *face = this->Faces->GetPointer(0);
  vtkIdType nfaces = *face++;
  vtkIdType npts, i, fid, hit, numHits=0;
  double t=VTK_LARGE_FLOAT;
  double x[3];

  for (fid=0; fid < nfaces; ++fid)
    {
    npts = face[0];
    hit = 0;
    switch (npts)
      {
      case 3: //triangle
        for (i=0; i<3; i++)
          {
          this->Triangle->Points->SetPoint(i,this->Points->GetPoint(face[i+1]));
          this->Triangle->PointIds->SetId(i,face[i+1]);
          }
        hit = this->Triangle->IntersectWithLine(p1,p2,tol,t,x,pc,subId);
        break;
      case 4: //quad
        for (i=0; i<4; i++)
          {
          this->Quad->Points->SetPoint(i,this->Points->GetPoint(face[i+1]));
          this->Quad->PointIds->SetId(i,face[i+1]);
          }
        hit = this->Quad->IntersectWithLine(p1,p2,tol,t,x,pc,subId);
        break;
      default: //general polygon
        for (i=0; i<npts; i++)
          {
          this->Polygon->Points->SetPoint(i,this->Points->GetPoint(face[i+1]));
          this->Polygon->PointIds->SetId(i,face[i+1]);
          }
        hit = this->Polygon->IntersectWithLine(p1,p2,tol,t,x,pc,subId);
        break;
      }

    // Update minimum hit
    if ( hit )
      {
      numHits++;
      if ( t < tMin )
        {
        tMin = t;
        xMin[0] = x[0]; xMin[1] = x[1]; xMin[2] = x[2];
        }
      }

    face += face[0] + 1;
    }//for all faces

  // Compute parametric coordinates
  this->ComputeParametricCoordinate(xMin,pc);

  return numHits;
}

#define VTK_CERTAIN 1
#define VTK_UNCERTAIN 0
#define VTK_MAX_ITER 10    //Maximum iterations for ray-firing
#define VTK_VOTE_THRESHOLD 3

//----------------------------------------------------------------------------
// Shoot random rays and count the number of intersections
int vtkPolyhedron::IsInside(double x[3], double tolerance)
{
  // do a quick bounds check
  this->ComputeBounds();
  double *bounds = this->Bounds;
  if ( x[0] < bounds[0] || x[0] > bounds[1] ||
       x[1] < bounds[2] || x[1] > bounds[3] ||
       x[2] < bounds[4] || x[2] > bounds[5])
    {
    return 0;
    }
  
  // It's easiest if these computations are done in canonical space
  this->GenerateFaces();

  // This algorithm is adaptive; if there are enough faces in this
  // polyhedron, a cell locator is built to accelerate intersections.
  // Otherwise brute force looping over cells is used.
  vtkIdType *faceArray = this->Faces->GetPointer(0);
  vtkIdType nfaces = *faceArray++;
  if ( nfaces > 25 )
    {
    this->ConstructLocator();
    }

  // We need a length to normalize the computations
  double length = sqrt( this->Superclass::GetLength2() );

  //  Perform in/out by shooting random rays. Multiple rays are fired
  //  to improve accuracy of the result.
  //
  //  The variable iterNumber counts the number of rays fired and is
  //  limited by the defined variable VTK_MAX_ITER.
  //
  //  The variable deltaVotes keeps track of the number of votes for
  //  "in" versus "out" of the surface.  When deltaVotes > 0, more votes
  //  have counted for "in" than "out".  When deltaVotes < 0, more votes
  //  have counted for "out" than "in".  When the delta_vote exceeds or
  //  equals the defined variable VTK_VOTE_THRESHOLD, then the
  //  appropriate "in" or "out" status is returned.
  //
  double rayMag, ray[3], xray[3], t, pcoords[3], xint[3];
  int i, numInts, iterNumber, deltaVotes, subId;
  vtkIdType idx, numCells;
  double tol = tolerance * length;

  for (deltaVotes = 0, iterNumber = 1;
       (iterNumber < VTK_MAX_ITER) && (abs(deltaVotes) < VTK_VOTE_THRESHOLD);
       iterNumber++) 
    {
    //  Define a random ray to fire.
    rayMag = 0.0;
    while (rayMag == 0.0 )
      {
      for (i=0; i<3; i++)
        {
        ray[i] = vtkMath::Random(-1.0,1.0);
        }
      rayMag = vtkMath::Norm(ray);
      }

    // The ray must be appropriately sized wrt the bounding box. (It has to go
    // all the way through the bounding box.)
    for (i=0; i<3; i++)
      {
      xray[i] = x[i] + (length/rayMag)*ray[i];
      }

    // Intersect the line with each of the candidate cells
    numInts = 0;

    if ( this->LocatorConstructed )
      {
      // Retrieve the candidate cells from the locator
      this->CellLocator->FindCellsAlongLine(x,xray,tol,this->CellIds);
      numCells = this->CellIds->GetNumberOfIds();

      for ( idx=0; idx < numCells; idx++ )
        {
        this->PolyData->GetCell(this->CellIds->GetId(idx), this->Cell);
        if ( this->Cell->IntersectWithLine(x, xray, tol, t, xint, pcoords, subId) )
          {
          numInts++;
          }
        } //for all candidate cells
      }
    else
      {
      numCells = nfaces;
      this->ConstructPolyData();

      for ( idx=0; idx < numCells; idx++ )
        {
        this->PolyData->GetCell(idx, this->Cell);
        if ( this->Cell->IntersectWithLine(x, xray, tol, t, xint, pcoords, subId) )
          {
          numInts++;
          }
        } //for all candidate cells
      }
    
    // Count the result
    if ( (numInts % 2) == 0)
      {
      --deltaVotes;
      }
    else
      {
      ++deltaVotes;
      }
    } //try another ray

  //   If the number of votes is positive, the point is inside
  //
  return ( deltaVotes < 0 ? 0 : 1 );
}

#undef VTK_CERTAIN
#undef VTK_UNCERTAIN
#undef VTK_MAX_ITER
#undef VTK_VOTE_THRESHOLD


//----------------------------------------------------------------------------
int vtkPolyhedron::CellBoundary(int subId, double pcoords[3],
                                    vtkIdList *pts)
{ 
  return 0;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::EvaluatePosition( double x[3], double * closestPoint,
                                     int & vtkNotUsed(subId), double pcoords[3],
                                     double & minDist2, double * weights )
{
  // compute parametric coordinates 
  this->ComputeParametricCoordinate(x, pcoords);
  
  // construct polydata, the result is stored in this->PolyData,
  // the cell array is stored in this->Polys
  this->ConstructPolyData();
  
  // Construct cell locator
  this->ConstructLocator();
  
  // find closest point and store the squared distance
  vtkSmartPointer<vtkGenericCell> genCell = 
    vtkSmartPointer<vtkGenericCell>::New( );

  vtkIdType cellId;
  int id;
  this->CellLocator->FindClosestPoint(
    x, closestPoint, genCell, cellId, id, minDist2 );

  // set distance to be zero, if point is inside
  if (this->IsInside(x, std::numeric_limits<double>::infinity()))
    {
    minDist2 = 0.0;
    }
  
  // get the MVC weights
  this->InterpolateFunctions(x, weights);

  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::EvaluateLocation( int & vtkNotUsed(subId), double pcoords[3], 
                                      double x[3],  double * weights )
{
  this->ComputePositionFromParametricCoordinate(pcoords, x);
  
  this->InterpolateFunctions(x, weights);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::Derivatives(int vtkNotUsed(subId), double pcoords[3],
                                double *values, int dim, double *derivs)
{
  int i, j, k, idx;
  for ( j = 0; j < dim; j++ )
    {
    for ( i = 0; i < 3; i++ )
      {
      derivs[j*dim + i] = 0.0;
      }
    }

  static const double Sample_Offset_In_Parameter_Space = 0.01;
  
  double x[4][3];
  double coord[3];
  
  //compute positions of point and three offset sample points
  coord[0] = pcoords[0];
  coord[1] = pcoords[1];
  coord[2] = pcoords[2];
  this->ComputePositionFromParametricCoordinate(coord, x[0]);
  
  coord[0] += Sample_Offset_In_Parameter_Space;
  this->ComputePositionFromParametricCoordinate(coord, x[1]);
  coord[0] = pcoords[0];  

  coord[1] += Sample_Offset_In_Parameter_Space;
  this->ComputePositionFromParametricCoordinate(coord, x[2]);
  coord[1] = pcoords[1];  

  coord[2] += Sample_Offset_In_Parameter_Space;
  this->ComputePositionFromParametricCoordinate(coord, x[3]);
  coord[2] = pcoords[2];  

  this->ConstructPolyData();
  int numVerts = this->PolyData->GetNumberOfPoints();

  double *weights = new double[numVerts];
  double *sample = new double[dim*4];
  //for each sample point, sample data values
  for ( idx = 0, k = 0; k < 4; k++ ) //loop over three sample points
    {
    this->InterpolateFunctions(x[k],weights);
    for ( j = 0; j < dim; j++, idx++) //over number of derivates requested
      {
      sample[idx] = 0.0;
      for ( i = 0; i < numVerts; i++ )
        {
        sample[idx] += weights[i] * values[j + i*dim];
        }
      }
    }

  double v1[3], v2[3], v3[3];
  double l1, l2, l3;
  //compute differences along the two axes
  for ( i = 0; i < 3; i++ )
    {
    v1[i] = x[1][i] - x[0][i];
    v2[i] = x[2][i] - x[0][i];
    v3[i] = x[3][i] - x[0][i];
    }
  l1 = vtkMath::Normalize(v1);
  l2 = vtkMath::Normalize(v2);
  l3 = vtkMath::Normalize(v3);
  

  //compute derivatives along x-y-z axes
  double ddx, ddy, ddz;
  for ( j = 0; j < dim; j++ )
    {
    ddx = (sample[  dim+j] - sample[j]) / l1;
    ddy = (sample[2*dim+j] - sample[j]) / l2;
    ddz = (sample[3*dim+j] - sample[j]) / l3;

    //project onto global x-y-z axes
    derivs[3*j]     = ddx*v1[0] + ddy*v2[0] + ddz*v3[0];
    derivs[3*j + 1] = ddx*v1[1] + ddy*v2[1] + ddz*v3[1];
    derivs[3*j + 2] = ddx*v1[2] + ddy*v2[2] + ddz*v3[2];
    }

  delete [] weights;
  delete [] sample;
}

//----------------------------------------------------------------------------
double *vtkPolyhedron::GetParametricCoords()
{
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::InterpolateFunctions(double x[3], double *sf)
{
  // construct polydata, the result is stored in this->PolyData,
  // the cell array is stored in this->Polys
  this->ConstructPolyData();

  // compute the weights
  if (!this->PolyData->GetPoints())
    {
    return;
    }
  vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeights(
    x, this->PolyData->GetPoints(), this->Polys, sf);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::InterpolateDerivs(double x[3], double *derivs)
{
  (void)x;
  (void)derivs;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                               vtkPoints *pts)
{
  ptIds->Reset();
  pts->Reset();
  
  if (!this->GetPoints() || !this->GetNumberOfPoints())
    {
    return 0;
    }
  
  this->ComputeBounds();
  
  // use ordered triangulator to triangulate the polyhedron.
  vtkSmartPointer<vtkOrderedTriangulator> triangulator = 
    vtkSmartPointer<vtkOrderedTriangulator>::New();
  
  triangulator->InitTriangulation(this->Bounds, this->GetNumberOfPoints());
  triangulator->PreSortedOff();
  
  double point[3]; 
  for (vtkIdType i = 0; i < this->GetNumberOfPoints(); i++)
    {
    this->GetPoints()->GetPoint(i, point);
    triangulator->InsertPoint(i, point, point, 0);
    }
  triangulator->Triangulate();
  
  triangulator->AddTetras(0, ptIds, pts);
  
  // convert to global Ids
  vtkIdType* ids = ptIds->GetPointer(0);
  for (vtkIdType i = 0; i < ptIds->GetNumberOfIds(); i++)
    {
    ids[i] = this->PointIds->GetId(ids[i]);
    }
    
  return 1;
}

//----------------------------------------------------------------------------
// NOTE: inScalars are in canonoical id space. while inPd are in global id space.
int vtkPolyhedron::InternalContour(double value,
                            vtkIdType insideOut,
                            vtkIncrementalPointLocator *locator,
                            vtkDataArray *inScalars,
                            vtkDataArray *outScalars,
                            vtkPointData *inPd, 
                            vtkPointData *outPd,
                            vtkCellArray *contourPolys,
                            IdToIdVectorMapType & faceToPointsMap,
                            IdToIdVectorMapType & pointToFacesMap,
                            IdToIdMapType & pointIdMap)
{
  double x0[3], x1[3], x[3];
  double v0, v1, v, t;

  vtkIdType p0, p1, pid, fid, outPid, globalP0, globalP1;
  void * ptr = NULL;

  const double eps = 0.000001;

  pointToFacesMap.clear();
  faceToPointsMap.clear();
  pointIdMap.clear();

  // initialization
  this->GenerateEdges();
  this->GenerateFaces();
  this->ConstructPolyData();
  this->ComputeBounds();

  // first check if all positive or all negative, construct a point label vector
  // that records whether a point is a positive point 1, a negative point -1 or 
  // a contour point 0.
  bool allPositive = true;
  bool allNegative = true;
  vtkstd::vector<int> pointLabelVector;
  for (pid = 0; pid < this->Points->GetNumberOfPoints(); pid++)
    {
    v = inScalars->GetComponent(pid,0);
    if (v < value + eps)
      {
      allPositive = false;
      if (v > value - eps)
        {
        pointLabelVector.push_back(0);
        }
      else
        {
        pointLabelVector.push_back(-1);
        }
      }
    else if (v > value - eps)
      {
      allNegative = false;
      if (v < value + eps)
        {
        pointLabelVector.push_back(0);
        }
      else
        {
        pointLabelVector.push_back(1);
        }
      }
    }
  
  if ((allPositive && insideOut) || (allNegative && !insideOut))
    {
    return 2;
    }
  
  if (allPositive || allNegative)
    {
    return 1;
    }

  // Here we use a point merger to try to prevent the problem of duplicated 
  // points in the input. Since point duplication can be really complicated,
  // there is no guarantee that this will always work. 
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->DeepCopy(this->Points);
  vtkSmartPointer<vtkMergePoints> merge = vtkSmartPointer<vtkMergePoints>::New();
  //merge->SetTolerance(0.000001);
  merge->InitPointInsertion(points, this->Bounds);
  for (int i = 0; i < points->GetNumberOfPoints(); i++)
    {
    merge->InsertUniquePoint(points->GetPoint(i), pid);
    }

  for (vtkIdType i = 0; i < inScalars->GetNumberOfTuples(); i++)
    {
    outScalars->InsertNextTuple1(inScalars->GetTuple1(i));
    }
  
  // construct a face to contour points map
  IdToIdVectorMapType faceToContourPointsMap;
  
  IdToIdVectorMapIteratorType vfMapIt, vfMapIt0, vfMapIt1;
  IdToIdVectorMapIteratorType fvMapIt, fcpMapIt, fcpMapItTemp;

  IdToIdVectorMapType globalMap;

  // loop through all faces to construct PointToFacesMap and FaceToPointsMap
  vtkPolyhedronFaceIterator 
    faceIter(this->GetNumberOfFaces(), this->Faces->GetPointer(1));
  while (faceIter.Id < faceIter.NumberOfPolygons)
    {
    // the rest code of this function assumes that a face contains at least 
    // three vertices. return if find a single-vertex or double-vertex face.
    if (faceIter.CurrentPolygonSize < 3)
      {
      vtkErrorMacro("Find a face with " << faceIter.CurrentPolygonSize <<
        " vertices. Contouring aborted due to this degenrate case.");
      return -1;      
      }

    fid = faceIter.Id;
    IdVectorType vVector;
    IdVectorType gVector;
    for (vtkIdType i = 0; i < faceIter.CurrentPolygonSize; i++)
      {
      pid = faceIter.Current[i];
      vfMapIt = pointToFacesMap.find(pid);
      if (vfMapIt != pointToFacesMap.end())
        {
        vfMapIt->second.push_back(fid);
        }
      else
        {
        IdVectorType fVector;
        fVector.push_back(fid);
        pointToFacesMap.insert(IdToIdVectorPairType(pid, fVector));
        }
      vVector.push_back(pid);
      gVector.push_back(this->PointIds->GetId(pid));
      }
    
    faceToPointsMap.insert(IdToIdVectorPairType(fid, vVector));
    globalMap.insert(IdToIdVectorPairType(fid, gVector));
    ++faceIter;
    }

  // loop through all edges to find contour points and store them in the point 
  // locator. if the contour points are new (not overlap with any of original 
  // vertex), update PointToFacesMap, FaceToPointsMap and FaceToContourPointsMap.
  IdSetType cpSet; // contour point set
  this->EdgeTable->InitTraversal();
  while (this->EdgeTable->GetNextEdge(p0, p1, ptr))
    {
    // If both vertices are positive or negative, we do nothing and continue;
    if ((pointLabelVector[p0] == 1 && pointLabelVector[p1] == 1) ||
        (pointLabelVector[p0] == -1 && pointLabelVector[p1] == -1))
      {
      continue;
      }
    
    globalP0 = this->PointIds->GetId(p0);
    globalP1 = this->PointIds->GetId(p1);

    v0 = inScalars->GetComponent(p0,0);
    v1 = inScalars->GetComponent(p1,0);

    points->GetPoint(p0, x0);
    points->GetPoint(p1, x1);

    // If one or two of the vertices are contour points, we maintain the face
    // to contour point map then continue
    if (!pointLabelVector[p0] || !pointLabelVector[p1])
      {
      vtkIdType flag = 0;
      vtkIdType contourVertexIds[2] = {-1, -1};
      if (pointLabelVector[p0] == 0)
        {
        if (cpSet.insert(p0).second) // check if the point already exist in set
          {
          if (locator->InsertUniquePoint(x0, outPid))
            {
            outPd->CopyData(inPd, globalP0, outPid);
            }
          pointIdMap.insert(IdToIdPairType(p0, outPid));
          contourVertexIds[0] = p0;
          }
        flag = 1;
        }
      if (pointLabelVector[p1] == 0)
        {
        if (cpSet.insert(p1).second) // check if the point already exist in set
          {
          if (locator->InsertUniquePoint(x1, outPid))
            {
            outPd->CopyData(inPd, globalP1, outPid);
            }
          pointIdMap.insert(IdToIdPairType(p1, outPid));
          contourVertexIds[1] = p1;
          }
        flag = 1;
        }
      
      for (int i = 0; i < 2; i++)
        {
        if (contourVertexIds[i] < 0)
          {
          continue;
          }
        
        vfMapIt = pointToFacesMap.find(contourVertexIds[i]);
        if (vfMapIt == pointToFacesMap.end())
          {
          vtkErrorMacro("Cannot locate adjacent faces of a vertex. We "
            "should never get here. Contouring aborted.");
          return -1;
          }
        
        for (size_t k = 0; k < vfMapIt->second.size(); k++)
          {
          vtkIdType contourFaceId = vfMapIt->second[k];
          fcpMapIt = faceToContourPointsMap.find(contourFaceId);
          if (fcpMapIt != faceToContourPointsMap.end())
            {
            fcpMapIt->second.push_back(contourVertexIds[i]);
            }
          else
            {
            IdVectorType contourPointVector;
            contourPointVector.push_back(contourVertexIds[i]);
            faceToContourPointsMap.insert(
              IdToIdVectorPairType(contourFaceId, contourPointVector));
            }
          }
        }
      
      continue;
      }

   // If two edge vertices are one positive and one negative. We need to 
   // insert new contour points on this edge.
        
    t = (value - v0)/(v1 - v0);
    x[0] = (1 - t) * x0[0] + t * x1[0];
    x[1] = (1 - t) * x0[1] + t * x1[1];
    x[2] = (1 - t) * x0[2] + t * x1[2];

    if (merge->InsertUniquePoint(x, pid))
      {
      // update pointLabelVector: we know the pid will be the number of existing
      // point (original verices plus previously inserted contour points)
      pointLabelVector.push_back(0);
      
      // update PointToFacesMap: there should be two and only two faces adjacent
      // to the newly inserted contour point.
      vfMapIt0 = pointToFacesMap.find(p0);
      vfMapIt1 = pointToFacesMap.find(p1);
      IdVectorType fVector;
      IdVectorType fVector0 = vfMapIt0->second;
      IdVectorType fVector1 = vfMapIt1->second;
      for (size_t i = 0; i < fVector0.size(); i++)
        {
        for (size_t j = 0; j < fVector1.size(); j++)
          {
          if (fVector0[i] == fVector1[j])
            {
            fVector.push_back(fVector0[i]);
            }
          }
        }
      if (fVector.size() != 2)
        {
        vtkErrorMacro("The number of adjacent faces of a newly inserted contour "
          "point not equal to 2. We should never get here. Contouring aborted.");
        return -1;
        }
      pointToFacesMap.insert(IdToIdVectorPairType(pid, fVector));

      // update FaceToPointsMap: insert the new point to the adjacent faces,
      // but still need to keep the order
      for (int k = 0; k < 2; k++)
        {
        fvMapIt = faceToPointsMap.find(fVector[k]);
        InsertNewIdToIdVector(fvMapIt->second, pid, p0, p1);
        }

      // update FaceToContourPointsMap: insert the new point to the adjacent faces
      for (int k = 0; k < 2; k++)
        {
        fcpMapIt = faceToContourPointsMap.find(fVector[k]);
        if (fcpMapIt != faceToContourPointsMap.end())
          {
          fcpMapIt->second.push_back(pid);
          }
        else
          {
          IdVectorType contourPointVector;
          contourPointVector.push_back(pid);
          faceToContourPointsMap.insert(
            IdToIdVectorPairType(fVector[k], contourPointVector));
          }
        }

      // Maintain point data. only add to locator when it has never been added
      // as contour point of previous processed cells.
      if (locator->InsertUniquePoint(x, outPid) && outPd)
        {
        outPd->InterpolateEdge(inPd,outPid,globalP0,globalP1,t);
        }

      // A point unique to merge may not be unique to locator, since it may have 
      // been inserted to locator as contour point of previous processed cells.
      if (outScalars)
        {
        outScalars->InsertTuple1(pid, value);
        }

      pointIdMap.insert(IdToIdPairType(pid, outPid));
      }
      
    cpSet.insert(pid);
    }

  // Extract valid edges between contour points. We store edge information in a
  // edge map ceMap. The key (first field) of ceMap is contour point Pd. The 
  // second field of ceMap is a vector of Ids of connected contour points.
  IdToIdVectorMapType ceMap; // edge map
  
  // The following process needs to know whether a contour point is 2-connected.
  // This information is important. Contour edges connecting two 2-connected 
  // points or one 2-connected point and one branching point (connectivity > 2)
  // are boundary edges that only need to be traversed once. On the other hand, 
  // edges connecting two branching points are internal edges. They are shared
  // by two polygon faces and therefore need to be traversed twice, from both
  // directions. Note: a newly inserted contour point is always 2-connected.
  // While original vertex contour points can be 2-connected or branching point.
  // We store this information in a vector. Note: for fast access, the vector is 
  // indexed by point id.
  IdVectorType isBranchPointVector;
  vtkIdType numberOfAllPoints = merge->GetPoints()->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numberOfAllPoints; i++)
    {
    isBranchPointVector.push_back(0);
    }
  
  int maxConnectivity = ExtractContourConnectivities(ceMap, isBranchPointVector,
                             cpSet, pointLabelVector, pointToFacesMap, 
                             faceToPointsMap, faceToContourPointsMap);

  // special handling of point or line cases.
  vtkIdType numberOfContourPoints = cpSet.size();
  if (numberOfContourPoints < 3)
    {
    for (size_t i = 0; i < pointLabelVector.size(); i++)
      {
      if (pointLabelVector[i] == 1)
        {
        return insideOut ? 2 : 1;
        }
      if (pointLabelVector[i] == -1)
        {
        return insideOut ? 1 : 2;
        }
      }
    }

  // Compute the normal of contour point and sort its edges counter-clockwise.
  if (maxConnectivity > 2)
    {
    OrderConnectedContourPoints(ceMap, isBranchPointVector,
                                merge->GetPoints(), pointLabelVector);
    }
  
  // cpSet and ceMap defines the contour graph. We now need to travel through 
  // the graph to extract non-overlapping polygons. The polygons can share 
  // edges but none of them is a subset of another one. 
  // Here we use the order of the edges. Specifically, when a contour point 
  // is visited, we will choose the outgoing edge to be the edge previous to the 
  // incoming edge. 
  vtkstd::vector<IdVectorType> polygonVector;
  IdToIdVectorMapType::iterator ceMapIt, ceBackupMapIt;
  IdSetType::iterator cpSetIt;
  IdVectorType::iterator cpVectorIt;
  
  // backup ceMap. During graph travasal, we will remove edges from contour point
  // which can mess up the ordering.
  IdToIdVectorMapType ceBackupMap = ceMap;
  
  while (!cpSet.empty())
    {
    // we prefer starting from a branching point as its edges are ordered.
    vtkIdType startPid = -1;
    for (cpSetIt = cpSet.begin(); cpSetIt != cpSet.end(); ++cpSetIt)
      {
      if (isBranchPointVector[*cpSetIt])
        {
        startPid = *cpSetIt;
        break;
        }
      }
    if (startPid < 0)
      {
      startPid = *(cpSet.begin());
      }

    // check if the point still have untravelled outgoing edges.
    ceMapIt = ceMap.find(startPid);
    if (ceMapIt == ceMap.end())
      {
      cpSet.erase(cpSetIt);
      continue;
      }
    
    vtkIdType currPid = startPid;
    vtkIdType prevPid = -1;
    vtkIdType nextPid = -1;
    
    // vector to record points on a contour polygon
    IdVectorType cpLoop;

    // continue to find the next contour point.
    while (!cpLoop.empty() || prevPid == -1)
      {

      // when back to the start point, break the loop.
      if (!cpLoop.empty() && currPid == startPid)
        {
        break;
        }

      cpSetIt = cpSet.find(currPid);
      ceMapIt = ceMap.find(currPid);

      // we should never arrive to a deadend.
      if (ceMapIt == ceMap.end() || cpSetIt == cpSet.end())
        {
        vtkWarningMacro("Find unexpected case, the input polyhedron cell may "
        "contain intersecting edges. Contouring will continue, but this cell "
        "will be not be processed.");
        return -1;
        }

      // add current point to the polygon loop
      cpLoop.push_back(currPid);
      
      // get the current available outgoing edges
      IdVectorType edges = ceMapIt->second;
      
      // choose the next point to travel. the outgoing edge is chosen to be the
      // one previous to the incoming edge.
      if (prevPid == -1)
        {
        nextPid = edges[0];
        }
      else
        {
        if (!isBranchPointVector[currPid])
          {
          nextPid = edges[0] == prevPid ? edges[1] : edges[0];
          }
        else
          {
          IdVectorType backupEdges = ceBackupMap.find(currPid)->second;
          for (size_t i = 0; i < backupEdges.size(); i++)
            {
            if (backupEdges[i] == prevPid)
              {
              if (i == 0)
                {
                nextPid = backupEdges[backupEdges.size() - 1];
                }
              else
                {
                nextPid = backupEdges[i-1];
                }
              break;
              }
            }
          }
        }
      
      // remove the outgoing edge
      bool foundEdge = false;
      for (size_t i = 0; i < edges.size(); i++)
        {
        if (edges[i] == nextPid)
          {
          foundEdge = true;
          edges.erase(edges.begin()+i);
          }
        }
      
      // the next edge shouldn't have been travelled and thus been removed
      if (!foundEdge)
        {
        vtkWarningMacro("Find unexpected case, the input polyhedron cell may "
        "contain intersecting edges. Contouring will continue, but this cell "
        "will be not be processed.");
        return -1;
        }
      
      // updating ceMap. removing point from ceMap and cpSet if necessary. 
      // notice that for a 2-connected contour point, its edges have only
      // one adjacent polygon and therefore will only be visited once. so
      // we should remove the contour point the first time it is visited.
      if (edges.empty() || isBranchPointVector[currPid] == 0)
        {
        ceMap.erase(ceMapIt);
        cpSet.erase(cpSetIt);
        }
      else
        {
        ceMapIt->second = edges;
        }
      
      // move on
      prevPid = currPid;
      currPid = nextPid;
      nextPid = -1;

      }// end_inner_while_loop

    if (cpLoop.empty())
      {
      continue;
      }
    
    // record polygon loop. notice that we should also remove edges going from
    // a branching point to a 2-connected point visited in this loop.
    polygonVector.push_back(cpLoop);
    
    for (size_t i = 0; i < cpLoop.size(); i++)
      {
      if (isBranchPointVector[cpLoop[i]] == 0)
        {
        ceBackupMapIt = ceBackupMap.find(cpLoop[i]);
        IdVectorType connectedPts = ceBackupMapIt->second;
        for (size_t j = 0; j < connectedPts.size(); j++)
          {
          ceMapIt = ceMap.find(connectedPts[j]);
          if (ceMapIt != ceMap.end())
            {
            for (size_t k = 0; k < ceMapIt->second.size(); k++)
              {
              if (ceMapIt->second[k] == cpLoop[i])
                {
                ceMapIt->second.erase(ceMapIt->second.begin()+k);
                }
              }
            if (ceMapIt->second.empty())
              {
              cpSetIt = cpSet.find(ceMapIt->first);
              if (cpSetIt != cpSet.end())
                {
                cpSet.erase(cpSetIt);
                }
              ceMap.erase(ceMapIt);
              }
            }
          }
        }
      }
    } // end_outer_while_loop

  //
  // Finally, add contour polygons to the output
  for (size_t i = 0; i < polygonVector.size(); i++)
    {
    IdVectorType polygon = polygonVector[i];
    
    vtkIdType npts = static_cast<vtkIdType>(polygon.size());
    vtkIdType *pts = &(polygon[0]);

    if (npts < 3) // skip point or line contour
      {
      continue;
      }
    
    // check the dimensionality of the contour
    int ret = CheckContourDimensions(merge->GetPoints(), npts, pts);
    
    if (ret <= 1) // skip single point or co-linear points
      {
      }
    else if (ret == 2) // planar polygon, add directly
      {
      contourPolys->InsertNextCell(npts, pts);
      }
    else  // 3D points, need to triangulate the original polygon
      {
      Triangulate3DContour(npts, pts, contourPolys); 
      }
    }
  
  return 0;
}


//----------------------------------------------------------------------------
void vtkPolyhedron::Contour(double value,
                            vtkDataArray *pointScalars,
                            vtkIncrementalPointLocator *locator,
                            vtkCellArray *verts,
                            vtkCellArray *lines,
                            vtkCellArray *polys,
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, vtkIdType cellId,
                            vtkCellData *outCd)
{
  vtkIdType offset = 0;
  if (verts)
    {
    offset += verts->GetNumberOfCells();
    }
  if (lines)
    {
    offset += lines->GetNumberOfCells();
    }

  IdToIdVectorMapType faceToPointsMap;
  IdToIdVectorMapType pointToFacesMap;
  IdToIdMapType       pointIdMap;

  vtkSmartPointer<vtkCellArray> contourPolys =
    vtkSmartPointer<vtkCellArray>::New();
  
  int ret = this->InternalContour(value, 0, locator, pointScalars, NULL, 
    inPd, outPd, contourPolys, faceToPointsMap, pointToFacesMap, pointIdMap);
  if (ret != 0)
    {
    return;
    }
  
  vtkIdType npts = 0;
  vtkIdType *pts = 0;
  contourPolys->InitTraversal();
  while (contourPolys->GetNextCell(npts, pts))
    {
    if (!ConvertPointIds(npts, pts, pointIdMap))
      {
      vtkErrorMacro("Cannot find the id of an output point. We should never "
        "get here. Contouring aborted.");
      return;
      }
    
    vtkIdType newCellId = offset + polys->InsertNextCell(npts, pts);
    outCd->CopyData(inCd, cellId, newCellId);
    }
}


//----------------------------------------------------------------------------
void vtkPolyhedron::Clip(double value,
                         vtkDataArray *pointScalars,
                         vtkIncrementalPointLocator *locator, 
                         vtkCellArray *connectivity,
                         vtkPointData *inPd, vtkPointData *outPd,
                         vtkCellData *inCd, vtkIdType cellId,
                         vtkCellData *outCd, int insideOut)
{
  IdToIdVectorMapType faceToPointsMap;
  IdToIdVectorMapType pointToFacesMap;
  IdToIdMapType       pointIdMap;
  vtkIdType newPid, newCellId;

  vtkIdType npts = 0;
  vtkIdType *pts = 0;

  // vector to store cell connectivity
  IdVectorType cellVector;

  vtkSmartPointer<vtkDoubleArray> contourScalars = 
    vtkSmartPointer<vtkDoubleArray>::New();
  vtkSmartPointer<vtkCellArray> contourPolys =
    vtkSmartPointer<vtkCellArray>::New();

  int ret = this->InternalContour(value, insideOut, locator, pointScalars, contourScalars, 
    inPd, outPd, contourPolys, faceToPointsMap, pointToFacesMap, pointIdMap);
  if (ret == 2)
    {
    return;
    }
  
  // polyhedron is all
  if (ret == 1)
    {
    cellVector.push_back(this->GetNumberOfFaces());

    // loop through all faces to add them into cellVector
    vtkPolyhedronFaceIterator 
      faceIter(this->GetNumberOfFaces(), this->Faces->GetPointer(1));
    while (faceIter.Id < faceIter.NumberOfPolygons)
      {
      IdVectorType pids;
      for (vtkIdType i = 0; i < faceIter.CurrentPolygonSize; i++)
        {
        vtkIdType pid = faceIter.Current[i];
        if (locator->InsertUniquePoint(this->Points->GetPoint(pid), newPid))
          {
          vtkIdType globalPid = this->PointIds->GetId(pid);
          outPd->CopyData(inPd, globalPid, newPid);
          }
        pids.push_back(pid);
        pointIdMap.insert(IdToIdPairType(pid, newPid));
        }

      npts = static_cast<vtkIdType>(pids.size());
      pts = &(pids[0]);
      if (!ConvertPointIds(npts, pts, pointIdMap))
        {
        vtkErrorMacro("Cannot find the id of an output point. We should never "
          "get here. Clipping aborted.");
        return;
        }
      cellVector.push_back(npts);
      cellVector.insert(cellVector.end(), pts, pts+npts);
     
      ++faceIter;
      }
    
    newCellId = connectivity->InsertNextCell(
      static_cast<vtkIdType>(cellVector.size()), &(cellVector[0]));
    outCd->CopyData(inCd, cellId, newCellId);
    
    return;
    }

  // prepare visited array for all faces
  bool* visited = new bool [this->GetNumberOfFaces()];
  for (int i = 0; i < this->GetNumberOfFaces(); i++)
    {
    visited[i] = false;
    }
  
  const double eps = 0.000001;
  
  // Main algorithm: go through all positive points (points on the right side 
  // of the contour).  These do not include contour points.
  // For each point on the right side, find all of its adjacent faces. There 
  // maybe two types of faces, (1) faces with all positive points, or 
  // (2) faces with positive negative and contour points. For case (1), we will
  // keep the original face and add it into the result polyhedron. For case (2),
  // we will subdivide the original face, and add the subface that includes 
  // positive points into the result polyhedron.
  vtkstd::vector<IdVectorType> faces;
  IdToIdVectorMapIteratorType pfMapIt, fpMapIt;
  for (vtkIdType pid = 0; pid < this->Points->GetNumberOfPoints(); pid++)
    {
    // find if a point is a positive point
    double v = contourScalars->GetComponent(pid,0);
    if ( (insideOut && (v > value-eps)) || ((!insideOut) && (v < value+eps)) )
      {
      continue;
      }
    
    // find adjacent faces of the positive point
    pfMapIt = pointToFacesMap.find(pid);
    if (pfMapIt == pointToFacesMap.end())
      {
      vtkErrorMacro("Cannot locate adjacent faces of a positive point. We should "
        "never get here. Clipping continues but may generate wrong result.");
      continue;
      }
    IdVectorType fids = pfMapIt->second;
    
    // for each adjacent face
    for (size_t i = 0; i < fids.size(); i++)
      {
      vtkIdType fid = fids[i];
      if (visited[fid])
        {
        continue;
        }
      
      fpMapIt = faceToPointsMap.find(fid);
      if (fpMapIt == faceToPointsMap.end())
        {
        vtkErrorMacro("Cannot locate points on a face. We should "
          "never get here. Clipping continues but may generate wrong result.");
        continue;
        }
      IdVectorType pids = fpMapIt->second;
      vtkIdType numFacePoints = static_cast<vtkIdType>(pids.size());

      // locate the positive point inside the id vector.      
      vtkIdType positivePt = -1;
      for (vtkIdType j = 0; j < numFacePoints; j++)
        {
        if (pid == pids[j])
          {
          positivePt = j;
          break;
          }
        }
      
      // positive point not found: this can happen when the current face 
      // has been partially visited before, and some points have been removed from 
      // its point vector. 
      if (positivePt < 0 || positivePt >= numFacePoints)
        {
        continue;
        }

      // a new id vector to hold ids of points on new surface patch
      IdVectorType newpids;
      newpids.push_back(pid);

      // step through the ajacent points on both sides of the positive point.
      // stop when a contour point or a negative point is hit.
      bool startFound = false;
      bool endFound = false;

      vtkIdType startPt = positivePt - 1;
      vtkIdType endPt = positivePt + 1;
      for (vtkIdType k = 0; k < numFacePoints; k++)
        {
        if (startFound && endFound)
          {
          break;
          }
        
        if (!startFound)
          {
          if (startPt < 0)
            {
            startPt = numFacePoints - 1;
            }
          
          newpids.insert(newpids.begin(), pids[startPt]);
          v = contourScalars->GetComponent(pids[startPt],0);
          if ((insideOut && (v > value-eps)) || ((!insideOut) && (v < value+eps)))
            {
            startFound = true;
            if ((insideOut && (v > value+eps)) || ((!insideOut) && (v < value-eps)))
              {
              vtkWarningMacro("A positive point is directly connected to a "
                "negative point with no contour point in between. We should "
                "never get here.");
              startPt = startPt == numFacePoints-1 ? 0 : startPt++;
              newpids.erase(newpids.begin());
              }
            }
          else
            {
            startPt--;
            }                
          }

        if (!endFound)
          {
          if (endPt > numFacePoints - 1)
            {
            endPt = 0;
            }
          
          newpids.push_back(pids[endPt]);
          v = contourScalars->GetComponent(pids[endPt],0);
          if ((insideOut && (v > value-eps)) || ((!insideOut) && (v < value+eps)))
            {
            endFound = true;
            if ((insideOut && (v > value+eps)) || ((!insideOut) && (v < value-eps)))
              {
              vtkWarningMacro("A positive point is directly connected to a "
                "negative point with no contour point in between. We should "
                "never get here.");
              endPt = endPt == 0 ? numFacePoints-1 : endPt--;
              newpids.pop_back();
              }
            }
          else
            {
            endPt++;
            }                
          }
        }// end inner for loop for finding start and end points

      // if face are entirely positive, add it directly into the face list
      if (!startFound && !endFound)
        {
        visited[fid] = true;
        faces.push_back(pids);
        }
      
      // if face contain contour points
      else if (startFound && endFound)
        {
        // a point or a line
        if (newpids.size() < 3)
          {
          visited[fid] = true;
          }
        // if face only contains one contour point, this is a special case that
        // may only happen when one of the original vertex is a contour point.
        // we will add this face to the result polyhedron.
        else if (startPt == endPt)
          {
          visited[fid] = true;
          faces.push_back(pids);
          }
        // Face contain at least two contour points. In this case, we will create
        // a new face patch whose close boundary is start point -->contour point
        // --> end point --> start point. Notice that the face may contain other
        // positive points and contour points. So we will not label the face as 
        // visited. Instead, we will erase the chunk from start point to end
        // point from the point id vector of the face. So that the other part
        // can still be visited in the future.
        else
          {
          if (!EraseSegmentFromIdVector(pids, positivePt, startPt, endPt))
            {
            vtkErrorMacro("Erase segment from Id vector failed. We should never get "
              "here.");
            visited[fid] = true;
            continue;
            }
          if (pids.size()<=2) // all but two contour points are left
            {
            pids.clear();
            visited[fid] = true;
            }
          fpMapIt->second = pids;
          faces.push_back(newpids); 
          }
        }
      
      // only find start or only find end. this should never happen
      else
        {
        visited[fid] = true;
        vtkErrorMacro("We should never get here. Locating contour points failed. "
          "Clipping continues but may generate wrong result.");
        }
      } // end for each face

    } // end for_pid 

  vtkIdType numAllFaces = contourPolys->GetNumberOfCells() + 
                          static_cast<vtkIdType>(faces.size());
  cellVector.push_back(numAllFaces);
  
  // add contour faces
  contourPolys->InitTraversal();
  while (contourPolys->GetNextCell(npts, pts))
    {
    if (!ConvertPointIds(npts, pts, pointIdMap, 1-insideOut))
      {
      vtkErrorMacro("Cannot find the id of an output point. We should never "
        "get here. Clipping aborted.");
      return;
      }
    cellVector.push_back(npts);
    cellVector.insert(cellVector.end(), pts, pts+npts);
    }

  // add other faces
  for (size_t i = 0; i < faces.size(); i++)
    {
    IdVectorType pids = faces[i];
    for (size_t j = 0; j < pids.size(); j++)
      {
      vtkIdType pid = pids[j];
      vtkPolyhedron::IdToIdMapType::iterator iter = pointIdMap.find(pid);
      if (iter == pointIdMap.end()) // must be original points
        {
        if (locator->InsertUniquePoint(this->Points->GetPoint(pid), newPid))
          {
          vtkIdType globalPid = this->PointIds->GetId(pid);
          outPd->CopyData(inPd, globalPid, newPid);
          }
        pointIdMap.insert(IdToIdPairType(pid, newPid));
        }
      }
    
    npts = static_cast<vtkIdType>(pids.size());
    pts = &(pids[0]);
    if (!ConvertPointIds(npts, pts, pointIdMap))
      {
      vtkErrorMacro("Cannot find the id of an output point. We should never "
        "get here. Clipping aborted.");
      return;
      }
    cellVector.push_back(npts);
    cellVector.insert(cellVector.end(), pts, pts+npts);
    }

  newCellId = connectivity->InsertNextCell(
    static_cast<vtkIdType>(cellVector.size()), &(cellVector[0]));
  outCd->CopyData(inCd, cellId, newCellId);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::DecomposeAPolyhedronCell(vtkCellArray * polyhedronCell, 
       vtkIdType & numCellPts, vtkIdType & nCellfaces, 
       vtkCellArray * cellArray, vtkIdTypeArray * faces)
{
  vtkIdType *cellStream = 0;
  vtkIdType cellLength = 0;
  
  polyhedronCell->InitTraversal();
  polyhedronCell->GetNextCell(cellLength, cellStream);
  
  vtkPolyhedron::DecomposeAPolyhedronCell(
    cellStream, numCellPts, nCellfaces, cellArray, faces);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::DecomposeAPolyhedronCell(vtkIdType *cellStream,
       vtkIdType & numCellPts, vtkIdType & nCellFaces, 
       vtkCellArray * cellArray, vtkIdTypeArray * faces)
{
  nCellFaces = cellStream[0];
  if (nCellFaces <= 0)
    {
    return;
    }
  
  vtkPolyhedron::DecomposeAPolyhedronCell(
    nCellFaces, cellStream+1, numCellPts, cellArray, faces);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::DecomposeAPolyhedronCell(vtkIdType nCellFaces,
       vtkIdType * cellStream, vtkIdType & numCellPts, 
       vtkCellArray * cellArray, vtkIdTypeArray * faces)
{
  IdSetType cellPointSet;
  IdSetType::iterator it;
  
  // insert number of faces into the face array
  faces->InsertNextValue(nCellFaces);
  
  // for each face
  for (vtkIdType fid = 0; fid < nCellFaces; fid++)
    {
    // extract all points on the same face, store them into a set
    vtkIdType npts = *cellStream++;
    faces->InsertNextValue(npts);
    for (vtkIdType i = 0; i < npts; i++)
      {
      vtkIdType pid = *cellStream++;
      faces->InsertNextValue(pid);
      cellPointSet.insert(pid);
      }
    }
  
  // standard cell connectivity array that stores the number of points plus
  // a list of point ids.
  cellArray->InsertNextCell(static_cast<int>(cellPointSet.size()));
  for (it = cellPointSet.begin(); it != cellPointSet.end(); ++it)
    {
    cellArray->InsertCellPoint(*it);
    }
  
  // the real number of points in the polyhedron cell.
  numCellPts = cellPointSet.size();
}

//----------------------------------------------------------------------------
void vtkPolyhedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Triangle:\n";
  this->Triangle->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Polygon:\n";
  this->Polygon->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Tetra:\n";
  this->Tetra->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Faces:\n";
  this->GlobalFaces->PrintSelf(os,indent.GetNextIndent());

}
