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
#include "vtkType.h"

#include <map>
#include <vector>
#include <set>
#include <list>
#include <limits>

vtkStandardNewMacro(vtkPolyhedron);

// Special typedef
typedef std::vector<vtkIdType>                 vtkIdVectorType;
class vtkPointIdMap : public std::map<vtkIdType,vtkIdType>{};
class vtkIdToIdMapType : public std::map<vtkIdType, vtkIdType>{};
class vtkIdToIdVectorMapType : public std::map<vtkIdType, vtkIdVectorType>{};
typedef std::map<vtkIdType,vtkIdType*>::iterator PointIdMapIterator;
typedef vtkIdToIdVectorMapType::iterator          vtkIdToIdVectorMapIteratorType;
typedef std::pair<vtkIdType, vtkIdVectorType>  vtkIdToIdVectorPairType;
typedef std::pair<vtkIdType, vtkIdType>        vtkIdToIdPairType;
typedef std::set<vtkIdType>                    vtkIdSetType;

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
      this->Id++;
      if (this->Id < this->NumberOfPolygons)
        {
        this->CurrentPolygonSize = this->Polygon[0];
        }
      else
        {
        this->CurrentPolygonSize = VTK_LARGE_ID;
        }
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
class vtkPolyhedron::vtkInternal
{
public:
  vtkIdTypeArray * FacesBackup;
  vtkEdgeTable * EdgeTableBackup;

vtkInternal()
{
  this->FacesBackup = NULL;
  this->EdgeTableBackup = NULL;
}

~vtkInternal()
{
  this->FacesBackup = NULL;
  this->EdgeTableBackup = NULL;
}

//----------------------------------------------------------------------------
// Here we use a point merger to try to prevent the problem of duplicated
// points in the input.
void RemoveDuplicatedPointsFromFaceArrayAndEdgeTable(vtkPoints * points,
                                                     vtkIdTypeArray * & faces,
                                                     vtkEdgeTable * & edgeTable,
                                                     double *bounds)
{
  const double eps = 0.000001;
  vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkPointLocator> merge = vtkSmartPointer<vtkPointLocator>::New();
  merge->SetTolerance(eps);
  merge->InitPointInsertion(newPoints, bounds);
  bool foundDupPoint = false;
  vtkIdType pid = -1;
  vtkIdToIdMapType pidMap0;
  for (vtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
    {
    if (!merge->InsertUniquePoint(points->GetPoint(i), pid))
      {
      foundDupPoint = true;
      }
    if (pidMap0.find(pid) == pidMap0.end())
      {
      pidMap0.insert(vtkIdToIdPairType(pid,i));
      }
    }

  // update face array and edge table if necessary.
  if (foundDupPoint)
    {
    vtkIdToIdMapType pidMap;
    for (vtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
      {
      pid = merge->IsInsertedPoint(points->GetPoint(i));
      pidMap.insert(vtkIdToIdPairType(i, pidMap0.find(pid)->second));
      }

    this->FacesBackup = faces;
    this->EdgeTableBackup = edgeTable;

    vtkIdType nfaces = 0;
    vtkIdType insertId = 0;

    faces = vtkIdTypeArray::New();
    faces->SetNumberOfTuples(points->GetNumberOfPoints()*10);
    faces->InsertComponent(insertId++, 0, 0); // allocate space for nfaces
    edgeTable = vtkEdgeTable::New();
    edgeTable->InitEdgeInsertion(points->GetNumberOfPoints());

    vtkPolyhedronFaceIterator
      faceIter(this->FacesBackup->GetValue(0), this->FacesBackup->GetPointer(1));
    while (faceIter.Id < faceIter.NumberOfPolygons)
      {
      vtkIdVectorType vVector;
      for (vtkIdType i = 0; i < faceIter.CurrentPolygonSize; i++)
        {
        pid = pidMap.find(faceIter.Current[i])->second;
        vVector.push_back(pid);
        }
      bool dupPointRemoved = true;
      while (dupPointRemoved && vVector.size() > 2)
        {
        dupPointRemoved = false;
        if (vVector[0] == vVector[vVector.size()-1])
          {
          vVector.erase(vVector.begin()+vVector.size()-1);
          dupPointRemoved = true;
          }
        for (size_t i = 1; i < vVector.size(); i++)
          {
          if (vVector[i] == vVector[i-1])
            {
            vVector.erase(vVector.begin()+i);
            dupPointRemoved = true;
            }
          }
        }
      if (vVector.size() < 3)
        {
        ++faceIter;
        continue;
        }

      nfaces++;

      faces->InsertComponent(insertId++, 0, vVector.size());
      for (size_t i = 0; i < vVector.size(); i++)
        {
        faces->InsertComponent(insertId++, 0, vVector[i]);
        }
      if (edgeTable->IsEdge(vVector[0],vVector[vVector.size()-1]) == (-1))
        {
        edgeTable->InsertEdge(vVector[0],vVector[vVector.size()-1]);
        }
      for (size_t i = 1; i < vVector.size(); i++)
        {
        if (edgeTable->IsEdge(vVector[i],vVector[i-1]) == (-1))
          {
          edgeTable->InsertEdge(vVector[i],vVector[i-1]);
          }
        }

      ++faceIter;
      }

    faces->SetComponent(0,0,nfaces);
    }
  else
    {
    this->FacesBackup = NULL;
    this->EdgeTableBackup = NULL;
    }
}

//----------------------------------------------------------------------------
// Here we use a point merger to try to prevent the problem of duplicated
// points in the input.
void RestoreFaceArrayAndEdgeTable(vtkIdTypeArray * & faces,
                                  vtkEdgeTable * & edgeTable)
{
  if (this->FacesBackup)
    {
    faces->Delete();
    faces = this->FacesBackup;
    }
  if (this->EdgeTableBackup)
    {
    edgeTable->Delete();
    edgeTable = this->EdgeTableBackup;
    }
}

//----------------------------------------------------------------------------
// insert new id element in between two existing adjacent id elements.
// this is a convenient function. no check whether the input elements
// exist in the vector. no check for element adjacency.
int InsertNewIdToIdVector(vtkIdVectorType & idVector, vtkIdType id,
                          vtkIdType id0, vtkIdType id1)
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

  vtkIdVectorType::iterator iter = idVector.begin();
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
};

// Convinient function used by clip. The id is the vector index of the positive
// point, id0 is the vector index of the start point, and id1 is the vector index
// of the end point.
//----------------------------------------------------------------------------
int EraseSegmentFromIdVector(vtkIdVectorType & idVector, vtkIdType id,
                             vtkIdType id0, vtkIdType id1)
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
};

// convert the point ids from map.first to map.second
//----------------------------------------------------------------------------
int ConvertPointIds(vtkIdType npts, vtkIdType * pts,
                    vtkIdToIdMapType & map, vtkIdType reverse = 0)
{
  for (vtkIdType i = 0; i < npts; i++)
    {
    vtkIdType id = reverse ? npts-1-i : i;
    vtkIdToIdMapType::iterator iter = map.find(pts[id]);
    if (iter == map.end())
      {
      return 0;
      }
    pts[id] = iter->second;
    }
  return 1;
};

//----------------------------------------------------------------------------
// The connected contour points are found by (1) locating the current
// contour point in the face loop, (2) looping through face point:
//  meet a positive point, keep going.
//  meet a contour point, store it and stop marching in this direction.
//  meet a negative point, stop marching in this direction.
//  meet the same point from both directions, stop.
// This loop may find zero, one or two connected contour points.
void FindConnectedContourPointsOnFace(vtkIdVectorType & facePtsVector,
                                      vtkIdVectorType & faceContourPtsVec,
                                      vtkIdType currContourPoint,
                                      vtkIdVectorType & pointLabelVec,
                                      vtkIdSetType & connectedContourPtsSet,
                                      vtkIdSetType & unConnectedContourPtsSet)
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
};

//----------------------------------------------------------------------------
void RemoveIdFromIdToIdVectorMap(vtkIdToIdVectorMapType & map, vtkIdType id)
{
  vtkIdToIdVectorMapIteratorType mit = map.begin();
  for (; mit != map.end(); ++mit)
    {
    vtkIdVectorType::iterator vit = mit->second.begin();
    for (; vit != mit->second.end(); ++vit)
      {
      if ((*vit) == id)
        {
        mit->second.erase(vit);
        break;
        }
      }
    }
};

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
                 vtkIdToIdVectorMapType & ceMap,
                 vtkIdSetType & cpSet,
                 vtkIdVectorType & pointLabelVector,
                 vtkIdToIdVectorMapType & pointToFacesMap,
                 vtkIdToIdVectorMapType & faceToPointsMap,
                 vtkIdToIdVectorMapType & faceToContourPointsMap)
{
  int maxConnectivity = 0;
  if (cpSet.empty())
    {
    return 0;
    }

  vtkIdSetType contourBranchesSet;
  vtkIdSetType nonContourBranchesSet;
  vtkIdVectorType contourBranchesVector;
  vtkIdSetType::iterator cpSetIt;
  vtkIdToIdVectorMapType::iterator fcpMapIt, fvMapIt, ceMapIt, ceMapIt1;
  for (cpSetIt = cpSet.begin(); cpSetIt != cpSet.end(); /*manual increment*/)
    {
    contourBranchesSet.clear();
    nonContourBranchesSet.clear();
    contourBranchesVector.clear();
    vtkIdType pid = *cpSetIt;
    vtkIdVectorType fVector = pointToFacesMap.find(pid)->second;
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
        cout << "Cannot find point ids of a face. We should never get "
          "here. Contouring aborted." << endl;
        return 0;
        }


      // find connected contour points and store them in the set. Notice that
      // some weird topology will classify a point as a connected contour point
      // in one face and a non-connected contour point in some other face. we
      // will extract the union.
      FindConnectedContourPointsOnFace(
                  fvMapIt->second, fcpMapIt->second, pid,
                  pointLabelVector, contourBranchesSet, nonContourBranchesSet);
      }

    if (!contourBranchesSet.empty())
      {
      vtkIdSetType::iterator ccpSetIt = contourBranchesSet.begin();
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
        vtkIdToIdVectorPairType(pid, contourBranchesVector));
      ++cpSetIt;
      }
    else // throw away point contour or edge contour.
      {
      if (cpSetIt != cpSet.begin())
        {
        vtkIdSetType::iterator tempIt = cpSetIt;
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
    vtkIdVectorType edges = ceMapIt->second;
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
      vtkIdToIdVectorMapType::iterator tempIt = ceMapIt;
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

  // set maxConnectivity.
  for (ceMapIt = ceMap.begin(); ceMapIt != ceMap.end(); ++ceMapIt)
    {
    if (static_cast<int>(ceMapIt->second.size()) > maxConnectivity)
      {
      maxConnectivity = static_cast<int>(ceMapIt->second.size());
      }
    }

  return maxConnectivity;
};

//----------------------------------------------------------------------------
// Use eigenvalues to determine the dimension of the input contour points.
// This chunk of code is mostly copied from vtkOBBTree::ComputeOBB()
// Function return 0 if input is a single point, 1 if co-linear,
// 2 if co-planar, 3 if 3D. It also returns the center as well as the normal
// (the eigenvector with the smallest eigenvalue) of the input contour pointset.
int CheckContourDimensions(vtkPoints* points, vtkIdType npts, vtkIdType * ptIds,
                           double * normal, double * center)
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

  if (normal)
    {
    for (i =0; i < 3; i++)
      {
      double norm = vtkMath::Norm(a[i], 3);
      if (norm > 0.000001)
        {
        break;
        }
      }
    if (i < 3)
      {
      normal[0] = v2[0];
      normal[1] = v2[1];
      normal[2] = v2[2];
      }
    else
      {
      points->GetPoint(ptIds[0], v0);
      points->GetPoint(ptIds[1], v1);
      v0[0] = v0[0] - mean[0];
      v0[1] = v0[1] - mean[1];
      v0[2] = v0[2] - mean[2];
      v1[0] = v1[0] - mean[0];
      v1[1] = v1[1] - mean[1];
      v1[2] = v1[2] - mean[2];
      vtkMath::Normalize(v0);
      vtkMath::Normalize(v1);
      vtkMath::Cross(v0, v1, normal);
      vtkMath::Normalize(normal);
      }
    }
  if (center)
    {
    center[0] = mean[0];
    center[1] = mean[1];
    center[2] = mean[2];
    }

  return ret;
};

//----------------------------------------------------------------------------
// For each contour point, compute the normal (pointing to the positive side),
// then sort the other contour points connected to it, such that the connecting
// edges are ordered contour-clockwise when viewed from the normal direction.

// Input ceMap shows that a contour point (map->first) is connected to a number
// of other contour points (map->second). It does not distinguish boundary
// edges from internal edges. The following function also update ceMap such that
// a boundary edge a-->b (assuming traversing from the counter-clockwise
// direction) is only stored once ({a, [b, ...]}). an internal edge a<-->b is
// stored twice ({a, [b, ...] and {b, [a, ...]}}.

// Current implementation of this function assumes planar contours, we only
// compute normal once and reuse it for all other contour points.
// TODO: for non-planar cut, need to compute normal for each contour point. We
// then project edges onto a tangent plane and sort them.
void OrderMultiConnectedContourPoints(vtkIdToIdVectorMapType & cpMap,
                                     vtkIdToIdVectorMapType & cpBackupMap,
                                     vtkIdSetType & cpSet,
                                     vtkPoints * points)
{
  double o[3], p[3], x0[3], x1[3], e0[3], e1[3], n[3], nn[3];
  vtkIdSetType::iterator setIt;
  vtkIdVectorType pids;
  for (setIt = cpSet.begin(); setIt != cpSet.end(); ++setIt)
    {
    pids.push_back(*setIt);
    }

  // return if the input contour points are 1D. Note: the function also
  // compute normal n and center c.
  if (CheckContourDimensions(
        points, static_cast<vtkIdType>(pids.size()), &(pids[0]), n, o) < 2)
    {
    return;
    }
  vtkMath::Normalize(n);

  // locate an extreme point in a direction normal to the normal. this
  // extreme point is a convex vertex.
  vtkIdToIdVectorMapType::iterator mapIt = cpMap.begin();
  points->GetPoint(mapIt->first, p);
  e0[0] = p[0] - o[0];
  e0[1] = p[1] - o[1];
  e0[2] = p[2] - o[2];
  vtkMath::Normalize(e0);
  vtkMath::Cross(e0, n, nn);
  vtkMath::Normalize(nn);

  double maxDistance = VTK_DOUBLE_MIN;
  vtkIdType maxPid = -1;
  for (; mapIt != cpMap.end(); ++mapIt)
    {
    points->GetPoint(mapIt->first, p);
    e0[0] = p[0] - o[0];
    e0[1] = p[1] - o[1];
    e0[2] = p[2] - o[2];
    double distance = vtkMath::Dot(nn, e0);
    if (distance > maxDistance)
      {
      maxDistance = distance;
      maxPid = mapIt->first;
      }
    }

  // Order edges of the contour point contour-clockwise. Note that a boundary
  // point has two boundary edges. We will remove the incoming boundary edge
  // and store the outgoing boundary edge at the end (after all internal edges).
  // incoming and outgoing boudnary edges are defined when they are traversed
  // counter-clockwisely.
  std::vector<double> extremePointAngles; // record the angles of extreme point
  vtkIdVectorType edges;
  size_t edgesSize = 0;
  const double eps = 0.0000001;
  for (mapIt = cpMap.begin(); mapIt != cpMap.end(); ++mapIt)
    {
    edges = mapIt->second;
    edgesSize = edges.size();

    // If the contour point is 2-connected we don't need to order them.
    if (edgesSize >=3 || mapIt->first == maxPid)
      {
      // get the current first edge
      points->GetPoint(mapIt->first, p);
      points->GetPoint(edges[0], x0);
      e0[0] = x0[0] - p[0];
      e0[1] = x0[1] - p[1];
      e0[2] = x0[2] - p[2];
      vtkMath::Normalize(e0);
      vtkMath::Cross(e0, n, x0);
      vtkMath::Cross(n, x0, e0);
      vtkMath::Normalize(e0);

      // compute the angles from other edges to the first edge
      std::vector<double> angles;
      angles.push_back(0);
      const double maxDotProduct = 0.95;
      for (size_t i = 1; i < edgesSize; i++)
        {
        points->GetPoint(edges[i], x1);
        e1[0] = x1[0] - p[0];
        e1[1] = x1[1] - p[1];
        e1[2] = x1[2] - p[2];
        vtkMath::Normalize(e1);
        vtkMath::Cross(e1, n, x1);
        vtkMath::Cross(n, x1, e1);
        vtkMath::Normalize(e1);
        double dotproduct = vtkMath::Dot(e0, e1);
        double angle = acos(dotproduct);
        if (dotproduct < maxDotProduct && dotproduct > -maxDotProduct)
          {
          vtkMath::Cross(e0, e1, nn);
          if (vtkMath::Dot(n, nn) < 0)
            {
            angle = 2.0*vtkMath::Pi() - angle;
            }
          }
        else if (dotproduct > maxDotProduct)
          {
          vtkMath::Cross(e0, n, nn);
          angle = acos(vtkMath::Dot(nn, e1)) - vtkMath::Pi()/2.0;
          }
        else if (dotproduct < -maxDotProduct)
          {
          vtkMath::Cross(n, e0, nn);
          angle = acos(vtkMath::Dot(nn, e1)) + vtkMath::Pi()/2.0;
          }
        if (angle < -eps)
          {
          angle += 2.0*vtkMath::Pi();
          }
        if (angle > 2.0*vtkMath::Pi()+eps)
          {
          angle -= 2.0*vtkMath::Pi();
          }
        angles.push_back(angle);
        }

      // sort edges
      for (size_t i = 1; i < edgesSize-1; i++)
        {
        for (size_t j = i+1; j < edgesSize; j++)
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

      if (mapIt->first == maxPid)
        {
        extremePointAngles = angles;
        }
      }
    }

  // store the sorted map.
  cpBackupMap = cpMap;

  // find the incoming and outgoing boundary edges of the extreme point. we use
  // the observation: if the outgoing boundary edge is chosen as the reference
  // edge. the angle between all other edges and the outgoing boundary edges
  // will be in [0, pi]. the incoming boundary edge will be the one that is
  // previous to the outgoing boundary edge.
  mapIt = cpMap.find(maxPid);
  edges = mapIt->second;
  edgesSize = edges.size();
  if (extremePointAngles.size() != edgesSize)
    {
    cout << "The size of the edge array does not match the size of the "
      "angle array. We should never get here." << endl;
    return;
    }
  vtkIdType outBoundary = -1;
  vtkIdType inBoundary = -1;
  for (size_t i = 0; i < edgesSize; i++)
    {
    double angle0 = extremePointAngles[i];
    size_t j = 0;
    for (; j < edgesSize; j++)
      {
      double angle = extremePointAngles[j] - angle0;
      if (angle < 0)
        {
        angle = angle + 2.0*vtkMath::Pi();
        }
      if (angle > vtkMath::Pi())
        {
        break;
        }
      }
    if (j == edgesSize)
      {
      outBoundary = static_cast<vtkIdType>(i);
      inBoundary = outBoundary - 1 < 0 ?
                     static_cast<vtkIdType>(edgesSize) - 1 : outBoundary - 1;
      break;
      }
    }

  vtkIdType prevPid = maxPid;
  vtkIdType currPid = edges[outBoundary];

  // remove incoming boundary edge.
  edges.erase(edges.begin() + inBoundary);
  cpMap.find(maxPid)->second = edges;

  // traverse the contour graph to remove all incoming boundary edges.
  while (currPid != maxPid)
    {
    edges = cpMap.find(currPid)->second;
    edgesSize = edges.size();
    size_t i;
    bool foundPrevPid = false;
    for (i = 0; i < edgesSize; i++)
      {
      if (edges[i] == prevPid)
        {
        inBoundary = static_cast<vtkIdType>(i);
        outBoundary = inBoundary + 1 >= static_cast<vtkIdType>(edgesSize) ?
                        0 : inBoundary + 1;
        foundPrevPid = true;
        break;
        }
      }
    if (!foundPrevPid) // traversing failed.
      {
      return;
      }
    prevPid = currPid;
    currPid = edges[outBoundary];
    edges.erase(edges.begin() + inBoundary);
    cpMap.find(prevPid)->second = edges;
    }
};

//-----------------------------------------------------------------------------
void OrderTwoConnectedContourPoints(vtkIdToIdVectorMapType & cpMap,
                                   vtkIdToIdVectorMapType & cpBackupMap)
{
  // backup the map.
  cpBackupMap = cpMap;

  // traverse edges
  vtkIdToIdVectorMapType::iterator mapIt = cpMap.begin();
  vtkIdVectorType edges = mapIt->second;
  vtkIdType startPid = mapIt->first;

  // choose one as incoming edge and one as outgoing edge
  vtkIdType outBoundary = 0;
  vtkIdType inBoundary = 1;

  // find next point
  vtkIdType prevPid = mapIt->first;
  vtkIdType currPid = edges[outBoundary];

  // remove incoming boundary edge.
  edges.erase(edges.begin() + inBoundary);
  cpMap.find(startPid)->second = edges;

  // traverse the edge graph to remove all incoming boundary edges.
  while (currPid != startPid)
    {
    mapIt = cpMap.find(currPid);
    if (mapIt == cpMap.end())
      {
      cout << "Find an unexpected case. The input polyhedron cell may not be a "
        << "water tight or the polygonal faces may not be planar. Contouring "
        << "will continue, but this cell may not be processed correctly." << endl;
      break;
      }
    edges = mapIt->second;
    if (edges[0] == prevPid)
      {
      inBoundary = 0;
      outBoundary = 1;
      }
    else
      {
      inBoundary = 1;
      outBoundary = 0;
      }
    prevPid = currPid;
    currPid = edges[outBoundary];
    edges.erase(edges.begin() + inBoundary);
    cpMap.find(prevPid)->second = edges;
    }
};

//----------------------------------------------------------------------------
// This function is called when InternalContour() finds an unexpected case
// (typically caused by a non-watertight cell). In this case, we will ignore
// the existing edges between contours. Instead, simply order them as a polygon
// around the center point.
int OrderDisconnectedContourPoints(vtkIdSetType & cpSet,
                                   vtkPoints * points,
                                   vtkIdVectorType & pointLabelVector,
                                   vtkIdVectorType & polygon)
{
  polygon.clear();
  if (cpSet.empty())
    {
    return 0;
    }

  double o[3], x[3], e0[3], e[3], n[3], nn[3];
  vtkIdSetType::iterator setIt;
  for (setIt = cpSet.begin(); setIt != cpSet.end(); ++setIt)
    {
    polygon.push_back(*setIt);
    }

  // return if the input contour points are 1D. Note: the function also
  // compute normal n and center c.
  if (CheckContourDimensions(
        points, static_cast<vtkIdType>(polygon.size()), &(polygon[0]), n, o) < 2)
    {
    return 0;
    }

  // make sure normal n points to the positive side
  vtkIdType numPoints = static_cast<vtkIdType>(pointLabelVector.size());
  for (vtkIdType i = 0; i < numPoints; i++)
    {
    if (pointLabelVector[i] == 1)
      {
      points->GetPoint(i, x);
      e[0] = x[0] - o[0];
      e[1] = x[1] - o[1];
      e[2] = x[2] - o[2];
      if (vtkMath::Dot(e, n) < 0)
        {
        n[0] = -n[0];
        n[1] = -n[1];
        n[2] = -n[2];
        }
      break;
      }
    else if (pointLabelVector[i] == -1)
      {
      points->GetPoint(i, x);
      e[0] = x[0] - o[0];
      e[1] = x[1] - o[1];
      e[2] = x[2] - o[2];
      if (vtkMath::Dot(e, n) > 0)
        {
        n[0] = -n[0];
        n[1] = -n[1];
        n[2] = -n[2];
        }
      break;
      }
    }

  // now loop over contour points to order them.
  std::vector<double> angles;
  angles.push_back(0.0);

  // choose to start from the first point
  points->GetPoint(polygon[0], x);
  e0[0] = x[0] - o[0];
  e0[1] = x[1] - o[1];
  e0[2] = x[2] - o[2];
  vtkMath::Cross(e0, n, nn);
  vtkMath::Cross(n, nn, e0);
  vtkMath::Normalize(e0);

  // compute the angles from other edges to the first edge
  for (size_t i = 1; i < polygon.size(); i++)
    {
    points->GetPoint(polygon[i], x);
    e[0] = x[0] - o[0];
    e[1] = x[1] - o[1];
    e[2] = x[2] - o[2];
    vtkMath::Cross(e, n, nn);
    vtkMath::Cross(n, nn, e);
    vtkMath::Normalize(e);

    const double maxDotProduct = 0.95;
    double dotproduct = vtkMath::Dot(e0, e);
    double angle = acos(dotproduct);
    if (dotproduct < maxDotProduct && dotproduct > -maxDotProduct)
      {
      vtkMath::Cross(e0, e, nn);
      if (vtkMath::Dot(n, nn) < 0)
        {
        angle += vtkMath::Pi();
        }
      }
    else if (dotproduct > maxDotProduct)
      {
      vtkMath::Cross(e0, n, nn);
      angle = acos(vtkMath::Dot(nn, e)) - vtkMath::Pi()/2.0;
      }
    else
      {
      vtkMath::Cross(n, e0, nn);
      angle = acos(vtkMath::Dot(nn, e)) + vtkMath::Pi()/2.0;
      }
    angles.push_back(angle);
    }

  // sort contour points
  for (size_t i = 1; i < polygon.size(); i++)
    {
    for (size_t j = i+1; j < polygon.size(); j++)
      {
      if (angles[i] > angles[j])
        {
        vtkIdType temp = polygon[i];
        polygon[i] = polygon[j];
        polygon[j] = temp;
        double angle = angles[i];
        angles[i] = angles[j];
        angles[j] = angle;
        }
      }
    }

  return 1;
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
};

}; //end vtkInternal class


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
  //this->Polys->Register(this);
  //this->Polys->Delete();
  this->PolyConnectivity = vtkIdTypeArray::New();
  this->LocatorConstructed = 0;
  this->CellLocator = vtkCellLocator::New();
  this->CellIds = vtkIdList::New();
  this->Cell = vtkGenericCell::New();

  this->Internal = new vtkInternal();
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
  delete this->Internal;
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

  if (this->Faces->GetNumberOfTuples() == 0)
    {
    return;
    }

  this->PolyConnectivity->SetNumberOfTuples(this->Faces->GetMaxId()-1);
  this->PolyConnectivity->
    SetArray(this->Faces->GetPointer(1), this->Faces->GetMaxId()-1, 1);
  this->Polys->SetNumberOfCells(*(this->Faces->GetPointer(0)));
  this->Polys->
    SetCells(*(this->Faces->GetPointer(0)), this->PolyConnectivity);

  // Standard setup
  this->PolyData->Initialize();
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
  this->CellLocator->Initialize();
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

  // Polys have to be reset
  this->Polys->Reset();
  this->PolyConnectivity->Reset();

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

  return static_cast<int>(this->Edges->GetNumberOfTuples());
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
  if ( this->GlobalFaces->GetNumberOfTuples() == 0 ||
       this->GlobalFaces->GetValue(0) <= 0 )
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
  // Make sure faces have been generated.
  if ( ! this->FacesGenerated )
    {
    this->GenerateFaces();
    }

  if (this->GlobalFaces->GetNumberOfTuples() == 0)
    {
    return 0;
    }

  return static_cast<int>(this->GlobalFaces->GetValue(0));
}

//----------------------------------------------------------------------------
void vtkPolyhedron::GenerateFaces()
{
  if ( this->FacesGenerated )
    {
    return;
    }

  if (this->GlobalFaces->GetNumberOfTuples() == 0)
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
  if (!this->GlobalFaces->GetNumberOfTuples())
    {
    return NULL;
    }

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
  vtkIdType npts, i, fid, numHits=0;
  double t=VTK_LARGE_FLOAT;
  double x[3];

  tMin=VTK_LARGE_FLOAT;
  for (fid=0; fid < nfaces; ++fid)
    {
    npts = face[0];
    vtkIdType hit = 0;
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
int vtkPolyhedron::CellBoundary(int vtkNotUsed(subId), double pcoords[3],
                                    vtkIdList *pts)
{
  double x[3], n[3], o[3], v[3];
  double dist, minDist = VTK_DOUBLE_MAX;
  vtkIdType numFacePts = -1;
  vtkIdType * facePts = 0;

  // compute coordinates
  this->ComputePositionFromParametricCoordinate(pcoords, x);

  vtkPolyhedronFaceIterator
    faceIter(this->GetNumberOfFaces(), this->Faces->GetPointer(1));
  while (faceIter.Id < faceIter.NumberOfPolygons)
    {
    if (faceIter.CurrentPolygonSize < 3)
      {
      continue;
      }

    vtkPolygon::ComputeNormal(this->Points, faceIter.CurrentPolygonSize,
                              faceIter.Current, n);
    vtkMath::Normalize(n);
    this->Points->GetPoint(faceIter.Current[0], o);
    v[0] = x[0] - o[0];
    v[1] = x[1] - o[1];
    v[2] = x[2] - o[2];
    dist = fabs(vtkMath::Dot(v, n));
    if (dist < minDist)
      {
      minDist = dist;
      numFacePts = faceIter.CurrentPolygonSize;
      facePts = faceIter.Current;
      }

    ++faceIter;
    }

  pts->Reset();
  if (numFacePts > 0)
    {
    for (vtkIdType i = 0; i < numFacePts; i++)
      {
      pts->InsertNextId(this->PointIds->GetId(facePts[i]));
      }
    }

  // determine whether point is inside of polygon
  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
       pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
       pcoords[2] >= 0.0 && pcoords[2] <= 1.0 &&
       (this->IsInside(x, std::numeric_limits<double>::infinity())) )
    {
    return 1;
    }
  else
    {
    return 0;
    }
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
  vtkIdType cellId;
  int id;
  double cp[3];
  this->Cell->Initialize();
  this->CellLocator->FindClosestPoint(
    x, cp, this->Cell, cellId, id, minDist2 );

  if (closestPoint)
    {
    closestPoint[0] = cp[0];
    closestPoint[1] = cp[1];
    closestPoint[2] = cp[2];
    }

  // get the MVC weights
  this->InterpolateFunctions(x, weights);

  // set distance to be zero, if point is inside
  int isInside = this->IsInside(x, std::numeric_limits<double>::infinity());
  if (isInside)
    {
    minDist2 = 0.0;
    }

  return isInside;
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
int vtkPolyhedron::IntersectWithContour(double value,
                                        int insideOut,
                                        vtkDataArray *inScalars)
{
  const double eps = 0.000001;
  bool allPositive = true;
  bool allNegative = true;
  for (vtkIdType pid = 0; pid < this->Points->GetNumberOfPoints(); pid++)
    {
    double v = inScalars->GetComponent(pid,0);
    if (v < value + eps)
      {
      allPositive = false;
      }
    else if (v > value - eps)
      {
      allNegative = false;
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

  return 0;
}

//----------------------------------------------------------------------------
// Internal implementation of contouring algorithm
// NOTE: inScalars are in canonoical id space. while inPd are in global id space.
int vtkPolyhedron::InternalContour(double value,
                                   int insideOut,
                                   vtkIncrementalPointLocator *locator,
                                   vtkDataArray *inScalars,
                                   vtkDataArray *outScalars,
                                   vtkPointData *inPd,
                                   vtkPointData *outPd,
                                   vtkCellArray *contourPolys,
                                   vtkIdToIdVectorMapType & faceToPointsMap,
                                   vtkIdToIdVectorMapType & pointToFacesMap,
                                   vtkIdToIdMapType & pointIdMap)
{
  const double eps = 0.000001;
  double x0[3], x1[3], x[3];
  double v0, v1, v, t;

  vtkIdType p0, p1, pid, fid, outPid, globalP0, globalP1;
  void * ptr = NULL;

  pointToFacesMap.clear();
  faceToPointsMap.clear();
  pointIdMap.clear();

  vtkIdVectorType pointLabelVector;
  for (pid = 0; pid < this->Points->GetNumberOfPoints(); pid++)
    {
    v = inScalars->GetComponent(pid,0);
    if (v < value + eps)
      {
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

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->DeepCopy(this->Points);

  if (outScalars)
    {
    for (vtkIdType i = 0; i < inScalars->GetNumberOfTuples(); i++)
      {
      outScalars->InsertNextTuple1(inScalars->GetTuple1(i));
      }
    }

  // construct a face to contour points map
  vtkIdToIdVectorMapType faceToContourPointsMap;

  vtkIdToIdVectorMapIteratorType vfMapIt, vfMapIt0, vfMapIt1;
  vtkIdToIdVectorMapIteratorType fvMapIt, fcpMapIt, fcpMapItTemp;

  // loop through all faces to construct PointToFacesMap and FaceToPointsMap
  vtkPolyhedronFaceIterator
    faceIter(this->Faces->GetValue(0), this->Faces->GetPointer(1));
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
    vtkIdVectorType vVector;
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
        vtkIdVectorType fVector;
        fVector.push_back(fid);
        pointToFacesMap.insert(vtkIdToIdVectorPairType(pid, fVector));
        }
      vVector.push_back(pid);
      }

    faceToPointsMap.insert(vtkIdToIdVectorPairType(fid, vVector));
    ++faceIter;
    }

  // loop through all edges to find contour points and store them in the point
  // locator. if the contour points are new (not overlap with any of original
  // vertex), update PointToFacesMap, FaceToPointsMap and FaceToContourPointsMap.
  vtkIdSetType cpSet; // contour point set
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
      vtkIdType contourVertexIds[2];
      contourVertexIds[0] = -1;
      contourVertexIds[1] = -1;
      if (pointLabelVector[p0] == 0)
        {
        if (cpSet.insert(p0).second) // check if the point already exist in set
          {
          if (locator->InsertUniquePoint(x0, outPid))
            {
            outPd->CopyData(inPd, globalP0, outPid);
            }
          pointIdMap.insert(vtkIdToIdPairType(p0, outPid));
          contourVertexIds[0] = p0;
          }
        }
      if (pointLabelVector[p1] == 0)
        {
        if (cpSet.insert(p1).second) // check if the point already exist in set
          {
          if (locator->InsertUniquePoint(x1, outPid))
            {
            outPd->CopyData(inPd, globalP1, outPid);
            }
          pointIdMap.insert(vtkIdToIdPairType(p1, outPid));
          contourVertexIds[1] = p1;
          }
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
          vtkErrorMacro("Cannot locate adjacent faces of a vertex. We should "
            "never get here. Contouring continue but result maybe wrong.");
          continue;
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
            vtkIdVectorType contourPointVector;
            contourPointVector.push_back(contourVertexIds[i]);
            faceToContourPointsMap.insert(
              vtkIdToIdVectorPairType(contourFaceId, contourPointVector));
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

    pid = points->InsertNextPoint(x);
    // update pointLabelVector: we know the pid will be the number of existing
    // point (original verices plus previously inserted contour points)
    pointLabelVector.push_back(0);

    // update PointToFacesMap: there should be two and only two faces adjacent
    // to the newly inserted contour point.
    vfMapIt0 = pointToFacesMap.find(p0);
    vfMapIt1 = pointToFacesMap.find(p1);
    vtkIdVectorType fVector;
    vtkIdVectorType fVector0 = vfMapIt0->second;
    vtkIdVectorType fVector1 = vfMapIt1->second;
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
      continue;
      }
    pointToFacesMap.insert(vtkIdToIdVectorPairType(pid, fVector));

    // update FaceToPointsMap: insert the new point to the adjacent faces,
    // but still need to keep the order
    for (int k = 0; k < 2; k++)
      {
      fvMapIt = faceToPointsMap.find(fVector[k]);
      this->Internal->InsertNewIdToIdVector(fvMapIt->second, pid, p0, p1);
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
        vtkIdVectorType contourPointVector;
        contourPointVector.push_back(pid);
        faceToContourPointsMap.insert(
          vtkIdToIdVectorPairType(fVector[k], contourPointVector));
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

    pointIdMap.insert(vtkIdToIdPairType(pid, outPid));

    cpSet.insert(pid);
    }

  // Extract valid edges between contour points. We store edge information in a
  // edge map ceMap. The key (first field) of ceMap is contour point Pd. The
  // second field of ceMap is a vector of Ids of connected contour points. This
  // process may remove point from cpSet if that point is only connected to one
  // other contour point and therefore form a edge face.
  vtkIdToIdVectorMapType ceMap; // edge map
  int maxConnectivity = this->Internal->ExtractContourConnectivities(
                             ceMap, cpSet, pointLabelVector, pointToFacesMap,
                             faceToPointsMap, faceToContourPointsMap);

  // special handling of point or line cases.
  if (cpSet.size() < 3 || ceMap.size() < 3)
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
    return -1;
    }

  // The following process needs to know whether a contour point is boundary
  // point and therefore contain two boundary edges.
  // This information is important. As we traverse the edges to extract polygons
  // contour edges only need to be traversed once, while internal edges need to
  // be traversed twice.
  // Note that in the simple case, where all contour points are 2-connected and
  // all contour edges are boundary edges. The result contour only contains one
  // single polygon. Otherwise, there are both boundary points (connected to two
  // boundary edges and zero or one or multiple internal edges) and internal
  // points (only connected to internal edges). The result contour contains
  // multiple polygons. In the latter case, we will need to distinguish boundary
  // contour points and interior contour points.

  // ceMap only shows that a contour point (map->first) is connected to a number
  // of other contour points (map->second). the following function computes
  // the normal of the contour point (map->first) and then sorts the connected
  // contour points (map->second) such that the connected edges are ordered
  // counter-clockwise. the sorted edge graph is stored in ceBackupMap.
  // the following function also distinguishes boundary edges from internal ones
  // a boundary edge a-->b (assuming traversing from the counter-clockwise
  // direction) is only stored once ({a, [b, ...]}). an internal edge a<-->b is
  // stored twice ({a, [b, ...] and {b, [a, ...]}}. this graph is stored in
  // the updated ceMap.
  vtkIdToIdVectorMapType ceBackupMap;
  if (maxConnectivity > 2)
    {
    this->Internal->OrderMultiConnectedContourPoints(ceMap, ceBackupMap,
                                                     cpSet, points);
    }
  else
    {
    this->Internal->OrderTwoConnectedContourPoints(ceMap, ceBackupMap);
    }

  // cpSet and ceMap defines the contour graph. We now need to travel through
  // the graph to extract non-overlapping polygons. The polygons can share
  // edges but none of them is a subset of another one.
  // Here we use the order of the edges. Specifically, when a contour point
  // is visited, we will choose the outgoing edge to be the edge previous to the
  // incoming edge in the ceBackupMap.
  std::vector<vtkIdVectorType> polygonVector;
  vtkIdToIdVectorMapType::iterator ceMapIt, ceBackupMapIt;
  vtkIdSetType::iterator cpSetIt;
  vtkIdVectorType::iterator cpVectorIt;

  // backup ceMap. During graph travasal, we will remove edges from contour point
  // which can mess up the ordering.
  vtkIdSetType cpBackupSet = cpSet;
  bool unexpectedCell = false;
  while (!cpSet.empty())
    {
    vtkIdType startPid = *(cpSet.begin());

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
    vtkIdVectorType cpLoop;

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
        unexpectedCell = true;
        break;
        }

      // add current point to the polygon loop
      cpLoop.push_back(currPid);

      // get the current available outgoing edges
      vtkIdVectorType edges = ceMapIt->second;

      // choose the next point to travel. the outgoing edge is chosen to be the
      // one previous to the incoming edge.
      if (prevPid == -1)
        {
        nextPid = edges[0];
        }
      else
        {
        if (edges.size() == 1)
          {
          nextPid = edges[0];
          }
        else if (edges.size() == 2)
          {
          nextPid = edges[0] == prevPid ? edges[1] : edges[0];
          }
        else
          {
          vtkIdVectorType backupEdges = ceBackupMap.find(currPid)->second;
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
        unexpectedCell = true;
        break;
        }

      // removing point from ceMap and cpSet if all its edges have been visited.
      if (edges.empty())
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

    if (unexpectedCell)
      {
      //vtkWarningMacro("Find an unexpected case. The input polyhedron cell may "
      //"not be a water tight cell. Or the contouring function is non-planar and "
      //"intersects more than two edges and/or vertices on one face of the input "
      //"polyhedron cell. Contouring will continue, but this cell will be not be "
      //"processed.");

      polygonVector.clear();
      vtkIdVectorType polygon;
      if (this->Internal->OrderDisconnectedContourPoints(cpBackupSet,
                               points, pointLabelVector, polygon))
         {
         polygonVector.push_back(polygon);
         }
      break;
      }

    if (!cpLoop.empty())
      {
      // record polygon loop.
      polygonVector.push_back(cpLoop);
      }
    } // end_outer_while_loop

  //
  // Finally, add contour polygons to the output
  for (size_t i = 0; i < polygonVector.size(); i++)
    {
    vtkIdVectorType polygon = polygonVector[i];

    vtkIdType npts = static_cast<vtkIdType>(polygon.size());
    vtkIdType *pts = &(polygon[0]);

    if (npts < 3) // skip point or line contour
      {
      continue;
      }

    // check the dimensionality of the contour
    int ret = this->Internal->
      CheckContourDimensions(points, npts, pts, NULL, NULL);

    if (ret <= 1) // skip single point or co-linear points
      {
      }
    else if (ret == 2) // planar polygon, add directly
      {
      contourPolys->InsertNextCell(npts, pts);
      }
    else  // 3D points, need to triangulate the original polygon
      {
      this->Internal->Triangulate3DContour(npts, pts, contourPolys);
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
  vtkIdToIdVectorMapType faceToPointsMap;
  vtkIdToIdVectorMapType pointToFacesMap;
  vtkIdToIdMapType       pointIdMap;  //local one, not this->PointIdMap
  vtkIdType offset = 0;
  if (verts)
    {
    offset += verts->GetNumberOfCells();
    }
  if (lines)
    {
    offset += lines->GetNumberOfCells();
    }

  // initialization
  this->GenerateEdges();
  this->GenerateFaces();
  this->ConstructPolyData();
  this->ComputeBounds();

  vtkIdVectorType pointLabelVector;
  if (this->IntersectWithContour(value, 0, pointScalars))
    {
    return;
    }

  this->Internal->RemoveDuplicatedPointsFromFaceArrayAndEdgeTable(
    this->Points, this->Faces, this->EdgeTable, this->Bounds);

  vtkSmartPointer<vtkCellArray> contourPolys =
    vtkSmartPointer<vtkCellArray>::New();

  int ret = this->InternalContour(value, 0, locator, pointScalars,
                    NULL, inPd, outPd, contourPolys,
                    faceToPointsMap, pointToFacesMap, pointIdMap);
  if (ret != 0)
    {
    this->Internal->RestoreFaceArrayAndEdgeTable(this->Faces, this->EdgeTable);
    return;
    }

  vtkIdType npts = 0;
  vtkIdType *pts = 0;
  contourPolys->InitTraversal();
  while (contourPolys->GetNextCell(npts, pts))
    {
    if (!this->Internal->ConvertPointIds(npts, pts, pointIdMap))
      {
      vtkErrorMacro("Cannot find the id of an output point. We should never "
        "get here. Contouring aborted.");
      this->Internal->RestoreFaceArrayAndEdgeTable(this->Faces, this->EdgeTable);
      return;
      }

    vtkIdType newCellId = offset + polys->InsertNextCell(npts, pts);
    outCd->CopyData(inCd, cellId, newCellId);
    }

  this->Internal->RestoreFaceArrayAndEdgeTable(this->Faces, this->EdgeTable);
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
  vtkIdToIdVectorMapType faceToPointsMap;
  vtkIdToIdVectorMapType pointToFacesMap;
  vtkIdToIdMapType       pointIdMap; //local one, not this->PointIdMap
  vtkIdType newPid, newCellId;

  vtkIdType npts = 0;
  vtkIdType *pts = 0;

  // initialization
  this->GenerateEdges();
  this->GenerateFaces();
  this->ConstructPolyData();
  this->ComputeBounds();

  // vector to store cell connectivity
  vtkIdVectorType cellVector;

  // vector to store which side of the clip function the polyhedron vertices are
  vtkIdVectorType pointLabelVector;

  // check if polyhedron is all in
  if (this->IntersectWithContour(value, insideOut, pointScalars) == 1)
    {
    cellVector.push_back(this->Faces->GetValue(0));

    // loop through all faces to add them into cellVector
    vtkPolyhedronFaceIterator
      faceIter(this->Faces->GetValue(0), this->Faces->GetPointer(1));
    while (faceIter.Id < faceIter.NumberOfPolygons)
      {
      vtkIdVectorType pids;
      for (vtkIdType i = 0; i < faceIter.CurrentPolygonSize; i++)
        {
        vtkIdType pid = faceIter.Current[i];
        if (locator->InsertUniquePoint(this->Points->GetPoint(pid), newPid))
          {
          vtkIdType globalPid = this->PointIds->GetId(pid);
          outPd->CopyData(inPd, globalPid, newPid);
          }
        pids.push_back(pid);
        pointIdMap.insert(vtkIdToIdPairType(pid, newPid));
        }

      npts = static_cast<vtkIdType>(pids.size());
      if (npts == 0)
        {
        ++faceIter;
        continue;
        }
      pts = &(pids[0]);
      if (!this->Internal->ConvertPointIds(npts, pts, pointIdMap))
        {
        vtkErrorMacro("Cannot find the id of an output point. We should never "
          "get here. Clipping aborted.");
        this->Internal->RestoreFaceArrayAndEdgeTable(this->Faces, this->EdgeTable);
        return;
        }
      cellVector.push_back(npts);
      cellVector.insert(cellVector.end(), pts, pts+npts);

      ++faceIter;
      }
    if (!cellVector.empty())
      {
      newCellId = connectivity->InsertNextCell(
        static_cast<vtkIdType>(cellVector.size()), &(cellVector[0]));
      outCd->CopyData(inCd, cellId, newCellId);
      }
    return;
    }

  this->Internal->RemoveDuplicatedPointsFromFaceArrayAndEdgeTable(
    this->Points, this->Faces, this->EdgeTable, this->Bounds);

  vtkSmartPointer<vtkDoubleArray> contourScalars =
    vtkSmartPointer<vtkDoubleArray>::New();
  vtkSmartPointer<vtkCellArray> contourPolys =
    vtkSmartPointer<vtkCellArray>::New();

  int ret = this->InternalContour(value, insideOut, locator, pointScalars,
                    contourScalars, inPd, outPd, contourPolys,
                    faceToPointsMap, pointToFacesMap, pointIdMap);

  // error occurs
  if (ret == -1)
    {
    this->Internal->RestoreFaceArrayAndEdgeTable(this->Faces, this->EdgeTable);
    return;
    }

  // polyhedron is all outside
  if (ret == 2)
    {
    this->Internal->RestoreFaceArrayAndEdgeTable(this->Faces, this->EdgeTable);
    return;
    }

  // polyhedron is all inside
  if (ret == 1)
    {
    cellVector.push_back(this->Faces->GetValue(0));

    // loop through all faces to add them into cellVector
    vtkPolyhedronFaceIterator
      faceIter(this->Faces->GetValue(0), this->Faces->GetPointer(1));
    while (faceIter.Id < faceIter.NumberOfPolygons)
      {
      vtkIdVectorType pids;
      for (vtkIdType i = 0; i < faceIter.CurrentPolygonSize; i++)
        {
        vtkIdType pid = faceIter.Current[i];
        if (locator->InsertUniquePoint(this->Points->GetPoint(pid), newPid))
          {
          vtkIdType globalPid = this->PointIds->GetId(pid);
          outPd->CopyData(inPd, globalPid, newPid);
          }
        pids.push_back(pid);
        pointIdMap.insert(vtkIdToIdPairType(pid, newPid));
        }

      npts = static_cast<vtkIdType>(pids.size());
      if (npts == 0)
        {
        if (faceIter.Id < faceIter.NumberOfPolygons - 1)
          {
          ++faceIter;
          continue;
          }
        else
          {
          break;
          }
        }
      pts = &(pids[0]);
      if (!this->Internal->ConvertPointIds(npts, pts, pointIdMap))
        {
        vtkErrorMacro("Cannot find the id of an output point. We should never "
          "get here. Clipping aborted.");
        this->Internal->RestoreFaceArrayAndEdgeTable(this->Faces, this->EdgeTable);
        return;
        }
      cellVector.push_back(npts);
      cellVector.insert(cellVector.end(), pts, pts+npts);

      ++faceIter;
      }
    if (!cellVector.empty())
      {
      newCellId = connectivity->InsertNextCell(
        static_cast<vtkIdType>(cellVector.size()), &(cellVector[0]));
      outCd->CopyData(inCd, cellId, newCellId);
      }
    this->Internal->RestoreFaceArrayAndEdgeTable(this->Faces, this->EdgeTable);
    return;
    }

  // prepare visited array for all faces
  bool* visited = new bool [this->Faces->GetValue(0)];
  for (int i = 0; i < this->Faces->GetValue(0); i++)
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
  std::vector<vtkIdVectorType> faces;
  vtkIdToIdVectorMapIteratorType pfMapIt, fpMapIt;
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
      continue;
      }
    vtkIdVectorType fids = pfMapIt->second;

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
      vtkIdVectorType pids = fpMapIt->second;
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
      vtkIdVectorType newpids;
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
              if (startPt == numFacePoints-1)
                {
                startPt = 0;
                }
              else
                {
                startPt++;
                }
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
              if (endPt == 0)
                {
                endPt = numFacePoints-1;
                }
              else
                {
                endPt--;
                }
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
          if (!this->Internal->EraseSegmentFromIdVector(
                     pids, positivePt, startPt, endPt))
            {
            vtkErrorMacro("Erase segment from Id vector failed. We should "
              "never get here.");
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

  delete [] visited;

  // not a valid output when the clip plane passes through the cell boundary
  // faces.
  if (faces.empty())
    {
    this->Internal->RestoreFaceArrayAndEdgeTable(this->Faces, this->EdgeTable);
    return;
    }

  vtkIdType numAllFaces = contourPolys->GetNumberOfCells() +
                          static_cast<vtkIdType>(faces.size());
  cellVector.push_back(numAllFaces);

  // add contour faces
  contourPolys->InitTraversal();
  while (contourPolys->GetNextCell(npts, pts))
    {
    if (!this->Internal->ConvertPointIds(npts, pts, pointIdMap, insideOut))
      {
      vtkErrorMacro("Cannot find the id of an output point. We should never "
        "get here. Clipping aborted.");
      this->Internal->RestoreFaceArrayAndEdgeTable(this->Faces, this->EdgeTable);
      return;
      }
    cellVector.push_back(npts);
    cellVector.insert(cellVector.end(), pts, pts+npts);
    }

  // add other faces
  for (size_t i = 0; i < faces.size(); i++)
    {
    vtkIdVectorType pids = faces[i];
    for (size_t j = 0; j < pids.size(); j++)
      {
      vtkIdType pid = pids[j];
      vtkIdToIdMapType::iterator iter = pointIdMap.find(pid);
      if (iter == pointIdMap.end()) // must be original points
        {
        if (locator->InsertUniquePoint(this->Points->GetPoint(pid), newPid))
          {
          vtkIdType globalPid = this->PointIds->GetId(pid);
          outPd->CopyData(inPd, globalPid, newPid);
          }
        pointIdMap.insert(vtkIdToIdPairType(pid, newPid));
        }
      }

    npts = static_cast<vtkIdType>(pids.size());
    pts = &(pids[0]);
    if (!this->Internal->ConvertPointIds(npts, pts, pointIdMap))
      {
      vtkErrorMacro("Cannot find the id of an output point. We should never "
        "get here. Clipping aborted.");
      this->Internal->RestoreFaceArrayAndEdgeTable(this->Faces, this->EdgeTable);
      return;
      }
    cellVector.push_back(npts);
    cellVector.insert(cellVector.end(), pts, pts+npts);
    }

  newCellId = connectivity->InsertNextCell(
    static_cast<vtkIdType>(cellVector.size()), &(cellVector[0]));
  outCd->CopyData(inCd, cellId, newCellId);

  this->Internal->RestoreFaceArrayAndEdgeTable(this->Faces, this->EdgeTable);
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
