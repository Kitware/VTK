/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIncrementalOctreePointLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkIncrementalOctreeNode.h"
#include "vtkIncrementalOctreePointLocator.h"

#include <map>
#include <list>
#include <stack>
#include <queue>
#include <vector>

vtkStandardNewMacro( vtkIncrementalOctreePointLocator );

// ---------------------------------------------------------------------------
// ----------------------------- Sorting  Points -----------------------------
// ---------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Helper class for sorting points in support of point location, specifically,
// vtkIncrementalOctreePointLocator::FindClosestNPoints().
namespace
{
  class SortPoints
  {
  public:
    SortPoints( int N )
    {
        this->NumberPoints = 0;
        this->NumRequested = N;
        this->LargestDist2 = VTK_DOUBLE_MAX;
    }

    void InsertPoint( double dist2, vtkIdType pntId )
    {
        // a new pair may be inserted as long as the squared distance is less
        // than the largest one of the current map OR the number of inserted
        // points is still less than that of requested points
        if (    dist2 <= this->LargestDist2
             || this->NumberPoints < this->NumRequested
           )
        {
          this->NumberPoints ++;
          std::map<  double,  std::list< vtkIdType >  >::iterator
            it = this->dist2ToIds.find( dist2 );

          if ( it == this->dist2ToIds.end() )
          {
            // no any entry corresponds to this squared distance
            std::list< vtkIdType >   idset;
            idset.push_back( pntId );
            this->dist2ToIds[ dist2 ] = idset;
          }
          else
          {
            // there is an entry corresponding to this squared distance
            it->second.push_back( pntId );
          }

          if ( this->NumberPoints > this->NumRequested )
          {
            // we need to go to the very last entry
            it = this->dist2ToIds.end();
            it --;

            // Even if we remove the very last entry, the number of points
            // will still be greater than that of requested points. This
            // indicates we can safely remove the very last entry and update
            // the largest squared distance with that of the entry before the
            // very last one.
            if (   this->NumberPoints - it->second.size()
                 > this->NumRequested
               )
            {
              this->NumberPoints -= it->second.size();
              std::map<  double,  std::list< vtkIdType >  >::iterator
                it2 = it;
              it2 --;
              this->LargestDist2 = it2->first;
              this->dist2ToIds.erase( it );
            }
          }
        }
    }

    void GetSortedIds( vtkIdList * idList )
    {
        // determine how many points will be actually exported
        idList->Reset();
        vtkIdType numIds = ( this->NumRequested < this->NumberPoints )
                           ? this->NumRequested : this->NumberPoints;

        idList->SetNumberOfIds( numIds );

        // clear the counter and go to the very first entry
        vtkIdType counter = 0;
        std::map<  double,  std::list< vtkIdType >  >::iterator
          it = this->dist2ToIds.begin();

        // export the point indices
        while ( counter < numIds && it != this->dist2ToIds.end() )
        {
          std::list<vtkIdType>::iterator lit = it->second.begin();

          while ( counter < numIds && lit != it->second.end() )
          {
            idList->InsertId( counter, *lit );
            counter ++;
            lit ++;
          }

          it ++;
        }
    }

    double GetLargestDist2()
    {
        return this->LargestDist2;
    }

  private:
    size_t  NumRequested;
    size_t  NumberPoints;
    double  LargestDist2;
    std::map<  double,  std::list< vtkIdType >  >  dist2ToIds;
  };
}

// ---------------------------------------------------------------------------
// --------------------- vtkIncrementalOctreePointLocator --------------------
// ---------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkIncrementalOctreePointLocator::vtkIncrementalOctreePointLocator()
{
  this->FudgeFactor      = 0;
  this->OctreeMaxDimSize = 0;
  this->BuildCubicOctree = 0;
  this->MaxPointsPerLeaf = 128;
  this->InsertTolerance2 = 0.000001;
  this->LocatorPoints  = NULL;
  this->OctreeRootNode = NULL;
}

//----------------------------------------------------------------------------
vtkIncrementalOctreePointLocator::~vtkIncrementalOctreePointLocator()
{
  this->FreeSearchStructure();
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::DeleteAllDescendants
  ( vtkIncrementalOctreeNode * node )
{
  if ( node->IsLeaf() == 0 )
  {
    for ( int i = 0; i < 8; i ++ )
    {
      vtkIncrementalOctreeNode * child = node->GetChild( i );
      vtkIncrementalOctreePointLocator::DeleteAllDescendants( child );
      child = NULL;
    }
    node->DeleteChildNodes();
  }
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::FreeSearchStructure()
{
  if ( this->OctreeRootNode )
  {
    vtkIncrementalOctreePointLocator::DeleteAllDescendants
                                      ( this->OctreeRootNode );
    this->OctreeRootNode->Delete();
    this->OctreeRootNode = NULL;
  }

  if ( this->LocatorPoints )
  {
    this->LocatorPoints->UnRegister( this );
    this->LocatorPoints = NULL;
  }
}

//----------------------------------------------------------------------------
int vtkIncrementalOctreePointLocator::GetNumberOfPoints()
{
  return ( this->OctreeRootNode == NULL )
         ? 0
         : this->OctreeRootNode->GetNumberOfPoints();
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::GetBounds( double * bounds )
{
  if ( this->OctreeRootNode )
  {
    double *    minBounds = this->OctreeRootNode->GetMinBounds();
    double *    maxBounds = this->OctreeRootNode->GetMaxBounds();
    bounds[0] = minBounds[0];
    bounds[1] = maxBounds[0];
    bounds[2] = minBounds[1];
    bounds[3] = maxBounds[1];
    bounds[4] = minBounds[2];
    bounds[5] = maxBounds[2];
    minBounds = maxBounds = NULL;
  }
}

//----------------------------------------------------------------------------
vtkIncrementalOctreeNode * vtkIncrementalOctreePointLocator::GetLeafContainer
  ( vtkIncrementalOctreeNode * node, const double pnt[3] )
{
  return (   ( node->IsLeaf() )
           ? node
           : this->GetLeafContainer
             (    node->GetChild(  node->GetChildIndex( pnt )  ),    pnt    )
         );
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindClosestInsertedPoint
  ( const double x[3] )
{
  if ( this->OctreeRootNode == NULL ||
       this->OctreeRootNode->GetNumberOfPoints() == 0 ||
       this->OctreeRootNode->ContainsPoint( x )  == 0
     )
  {
    return -1;
  }

  double    miniDist2 = this->OctreeMaxDimSize * this->OctreeMaxDimSize * 4.0;
  double    elseDist2;      // inter-node search
  vtkIdType elsePntId;      // inter-node search

  vtkIncrementalOctreeNode * pLeafNode = this->GetLeafContainer( this->OctreeRootNode, x );
  vtkIdType pointIndx = this->FindClosestPointInLeafNode( pLeafNode, x, &miniDist2 );

  if ( miniDist2 > 0.0 )
  {
    if (   pLeafNode->GetDistance2ToInnerBoundary( x, this->OctreeRootNode )
         < miniDist2
       )
    {
      elsePntId = this->FindClosestPointInSphereWithoutTolerance
                        ( x, miniDist2, pLeafNode, &elseDist2 );
      if ( elseDist2 < miniDist2 )
      {
        pointIndx = elsePntId;
        miniDist2 = elseDist2;
      }
    }
  }

  pLeafNode = NULL;
  return pointIndx;
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "FudgeFactor: "      << this->FudgeFactor      << endl;
  os << indent << "LocatorPoints: "    << this->LocatorPoints    << endl;
  os << indent << "OctreeRootNode: "   << this->OctreeRootNode   << endl;
  os << indent << "BuildCubicOctree: " << this->BuildCubicOctree << endl;
  os << indent << "MaxPointsPerLeaf: " << this->MaxPointsPerLeaf << endl;
  os << indent << "InsertTolerance2: " << this->InsertTolerance2 << endl;
  os << indent << "OctreeMaxDimSize: " << this->OctreeMaxDimSize << endl;
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::GenerateRepresentation
  ( int nodeLevel, vtkPolyData * polysData )
{
  if ( this->OctreeRootNode == NULL )
  {
    vtkErrorMacro( "vtkIncrementalOctreePointLocator::GenerateRepresentation" );
    vtkErrorMacro( "(): the octree is not yet available" );
    return;
  }

  int            tempLevel;
  vtkPoints    * thePoints = NULL;
  vtkCellArray * nodeQuads = NULL;
  vtkIncrementalOctreeNode * pTempNode = NULL;
  std::list < vtkIncrementalOctreeNode * > nodesList;
  std::queue< std::pair < vtkIncrementalOctreeNode *, int > > pairQueue;

  // recursively process the nodes in the octree
  pairQueue.push(  std::make_pair( this->OctreeRootNode, 0 )  );
  while ( !pairQueue.empty() )
  {
    pTempNode = pairQueue.front().first;
    tempLevel = pairQueue.front().second;
    pairQueue.pop();

    if ( tempLevel == nodeLevel )
    {
      nodesList.push_back( pTempNode );
    }
    else
    if ( !pTempNode->IsLeaf() )
    {
      for ( int i = 0; i < 8; i ++ )
      {
        pairQueue.push(    std::make_pair(  pTempNode->GetChild( i ),
                                               nodeLevel + 1
                                            )
                      );
      }
    }
  }

  // collect the vertices and quads of each node
  thePoints = vtkPoints::New();
  thePoints->Allocate(  8  *  static_cast < int > ( nodesList.size() )  );
  nodeQuads = vtkCellArray::New();
  nodeQuads->Allocate(  6  *  static_cast < int > ( nodesList.size() )  );
  for ( std::list< vtkIncrementalOctreeNode * >::iterator
        lit = nodesList.begin(); lit != nodesList.end(); lit ++ )
  {
    vtkIncrementalOctreePointLocator::AddPolys( *lit, thePoints, nodeQuads );
  }

  // attach points and quads
  polysData->SetPoints( thePoints );
  polysData->SetPolys ( nodeQuads );
  thePoints->Delete();
  nodeQuads->Delete();
  thePoints = NULL;
  nodeQuads = NULL;
  pTempNode = NULL;
}

//----------------------------------------------------------------------------
static vtkIdType OCTREE_NODE_FACES_LUT[6][4] =
{
  { 0, 1, 5, 4 },  { 0, 4, 6, 2 },
  { 6, 7, 3, 2 },  { 1, 3, 7, 5 },
  { 2, 3, 1, 0 },  { 4, 5, 7, 6 }
};

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::AddPolys
  ( vtkIncrementalOctreeNode * node, vtkPoints * points, vtkCellArray * polygs )
{
  int         i;
  double      bounds[6];
  double      ptCord[3];
  vtkIdType   pntIds[8];
  vtkIdType   idList[4];

  node->GetBounds( bounds );
  for ( i = 0; i < 8; i ++ )
  {
    ptCord[0] = bounds[ i & 1 ];
    ptCord[1] = bounds[ i & 2 ];
    ptCord[2] = bounds[ i & 4 ];
    pntIds[i] = points->InsertNextPoint( ptCord );
  }

  for ( i = 0; i < 6; i ++ )
  {
    idList[0] = pntIds[ OCTREE_NODE_FACES_LUT[i][0] ];
    idList[1] = pntIds[ OCTREE_NODE_FACES_LUT[i][1] ];
    idList[2] = pntIds[ OCTREE_NODE_FACES_LUT[i][2] ];
    idList[3] = pntIds[ OCTREE_NODE_FACES_LUT[i][3] ];
    polygs->InsertNextCell( 4, idList );
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindClosestPointInLeafNode
  ( vtkIncrementalOctreeNode * leafNode, const double point[3], double * dist2 )
{
  // NOTE: dist2 MUST be inited with a very huge value below,  but instead of
  // this->OctreeMaxDimSize * this->OctreeMaxDimSize * 4.0, because the point
  // under check may be outside the octree and hence the squared distance can
  // be greater than the latter or other similar octree-based specific values.
  *dist2 = VTK_DOUBLE_MAX;

  if ( leafNode->GetPointIdSet() == NULL )
  {
    return -1;
  }

  int         numPts = 0;
  double      tmpDst = 0.0;
  double      tmpPnt[3];
  vtkIdType   tmpIdx = -1;
  vtkIdType   pntIdx = -1;
  vtkIdList * idList = NULL;

  idList = leafNode->GetPointIdSet();
  numPts = idList->GetNumberOfIds( );

  for ( int i = 0; i < numPts; i ++ )
  {
    tmpIdx  = idList->GetId( i );
    this->LocatorPoints->GetPoint( tmpIdx, tmpPnt );
    tmpDst  = vtkMath::Distance2BetweenPoints( tmpPnt, point );
    if (  tmpDst  <  ( *dist2 )  )
    {
      *dist2  = tmpDst;
      pntIdx  = tmpIdx;
    }

    if (  ( *dist2 )  ==  0.0  )
    {
      break;
    }
  }

  idList = NULL;

  return pntIdx;
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindClosestPointInSphere
  ( const double point[3], double radius2, vtkIncrementalOctreeNode * maskNode,
    double * minDist2, const double * refDist2 )
{
  vtkIdType  pointIndx = -1;
  std::stack< vtkIncrementalOctreeNode * >  nodesBase;
  nodesBase.push( this->OctreeRootNode );

  while ( !nodesBase.empty() && ( *minDist2 ) > 0.0 )
  {
    vtkIncrementalOctreeNode * checkNode = nodesBase.top();
    nodesBase.pop();

    if ( !checkNode->IsLeaf() )
    {
      for ( int i = 0; i < 8; i ++ )
      {
        vtkIncrementalOctreeNode * childNode = checkNode->GetChild( i );

        // use ( radius2 + radius2 ) to skip empty nodes
        double distToData = ( childNode->GetNumberOfPoints() )
                            ? childNode->GetDistance2ToBoundary
                                         ( point, this->OctreeRootNode, 1 )
                            : ( radius2 + radius2 );

        // If a child node is not the mask node AND its distance, specifically
        // the data bounding box (determined by the points inside or under) to
        // the point, is less than the threshold radius (one exception is the
        // point's container nodes), it is pushed to the stack as a suspect.
        if (    ( childNode != maskNode )
             && (    (  distToData <= ( *refDist2 )  )
                  || (  childNode->ContainsPoint( point )  ==  1  )
                )
           )
        {
          nodesBase.push( childNode );
        }

        childNode = NULL;
      }
    }
    else
    {
      // now that the node under check is a leaf, let's find the closest
      // point therein and the minimum distance
      double tempDist2;
      int    tempPntId = this->FindClosestPointInLeafNode
                               ( checkNode, point, &tempDist2 );

      if (  tempDist2  <  ( *minDist2 )  )
      {
        *minDist2 = tempDist2;
        pointIndx = tempPntId;
      }
    }

    checkNode = NULL;
  }

  return  (  ( *minDist2 )  <=  radius2  )  ?  pointIndx  :  -1;
}

// ---------------------------------------------------------------------------
// ----------------------------- Point  Location -----------------------------
// ---------------------------------------------------------------------------

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::BuildLocator()
{
  // assume point location is necessary for vtkPointSet data only
  if ( !this->DataSet || !this->DataSet->IsA( "vtkPointSet" ) )
  {
    vtkErrorMacro( "Dataset is NULL or it is not of type vtkPointSet" );
    return;
  }

  int  numPoints = this->DataSet->GetNumberOfPoints();
  if ( numPoints < 1 || numPoints >= VTK_INT_MAX )
  {
    // current implementation does not support 64-bit point indices
    // due to performance consideration
    vtkErrorMacro( << "No points to build an octree with or " );
    vtkErrorMacro( << "failure to support 64-bit point ids"  );
    return;
  }

  // construct an octree only if necessary
  if (    ( this->BuildTime > this->MTime )
       && ( this->BuildTime > this->DataSet->GetMTime() )
     )
  {
    return;
  }
  vtkDebugMacro( << "Creating an incremental octree" );

  // build an octree by populating it with check-free insertion of point ids
  double       theBounds[6];
  double       theCoords[3];
  vtkIdType    pointIndx;
  vtkPoints *  thePoints = vtkPointSet::SafeDownCast( this->DataSet )
                           ->GetPoints();
  thePoints->GetBounds( theBounds );
  this->InitPointInsertion( thePoints, theBounds );

  for ( pointIndx = 0; pointIndx < numPoints; pointIndx ++ )
  {
    thePoints->GetPoint( pointIndx, theCoords );

    // the 3D point coordinate is actually not inserted to vtkPoints at all
    // while only the point index is inserted to the vtkIdList of the
    // container leaf
    this->InsertPointWithoutChecking( theCoords, pointIndx, 0 );
  }
  thePoints = NULL;

  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindClosestPointInSphereWithoutTolerance
  ( const double point[3], double radius2,
    vtkIncrementalOctreeNode * maskNode, double * minDist2 )
{
  // It might be unsafe to use a ratio less than 1.1 since radius2 itself
  // could be very small and 1.00001 might just be equal to radius2.
  *minDist2 = radius2 * 1.1;
  return this->FindClosestPointInSphere( point, radius2, maskNode,
                                               minDist2, minDist2 );
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindClosestPoint
  ( double x, double y, double z )
{
  double dumbDist2;
  double theCoords[3] = { x, y, z };
  return this->FindClosestPoint( theCoords, &dumbDist2 );
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindClosestPoint( const double x[3] )
{
  double dumbDist2;
  return this->FindClosestPoint( x, &dumbDist2 );
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindClosestPoint
  ( double x, double y, double z, double * miniDist2 )
{
  double theCoords[3] = { x, y, z };
  return this->FindClosestPoint( theCoords, miniDist2 );
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindClosestPoint
  ( const double x[3], double * miniDist2 )
{
  this->BuildLocator();

  // init miniDist2 for early exit
  *miniDist2 = this->OctreeMaxDimSize * this->OctreeMaxDimSize * 4.0;
  if ( this->OctreeRootNode == NULL ||
       this->OctreeRootNode->GetNumberOfPoints() == 0
     )
  {
    return -1;
  }

  double    elseDist2;      // inter-node search
  vtkIdType elsePntId;      // inter-node search
  vtkIdType pointIndx = -1;
  vtkIncrementalOctreeNode * pLeafNode = NULL;

  if (  this->OctreeRootNode->ContainsPoint( x )  )
  {  // the point is inside the octree
    pLeafNode = this->GetLeafContainer( this->OctreeRootNode, x );
    pointIndx = this->FindClosestPointInLeafNode( pLeafNode, x, miniDist2 );

    if (  ( *miniDist2 )  >  0.0  )
    {
      if (   pLeafNode->GetDistance2ToInnerBoundary( x, this->OctreeRootNode )
           < ( *miniDist2 )
         )
      {
        elsePntId = this->FindClosestPointInSphereWithoutTolerance
                          ( x, *miniDist2, pLeafNode, &elseDist2 );
        if (  elseDist2  <  ( *miniDist2)  )
        {
          pointIndx  = elsePntId;
          *miniDist2 = elseDist2;
        }
      }
    }
  }
  else // the point is outside the octree
  {
    double   initialPt[3];
    double * minBounds = this->OctreeRootNode->GetMinBounds();
    double * maxBounds = this->OctreeRootNode->GetMaxBounds();
    this->OctreeRootNode->GetDistance2ToBoundary
                          ( x, initialPt, this->OctreeRootNode, 1 );

    // This initial (closest) point might be outside the octree a little bit
    if ( initialPt[0] <= minBounds[0] )
    {
      initialPt[0] = minBounds[0] + this->FudgeFactor;
    }
    else
    if ( initialPt[0] >= maxBounds[0] )
    {
      initialPt[0] = maxBounds[0] - this->FudgeFactor;
    }

    if ( initialPt[1] <= minBounds[1] )
    {
      initialPt[1] = minBounds[1] + this->FudgeFactor;
    }
    else
    if ( initialPt[1] >= maxBounds[1] )
    {
      initialPt[1] = maxBounds[1] - this->FudgeFactor;
    }

    if ( initialPt[2] <= minBounds[2] )
    {
      initialPt[2] = minBounds[2] + this->FudgeFactor;
    }
    else
    if ( initialPt[2] >= maxBounds[2] )
    {
      initialPt[2] = maxBounds[2] - this->FudgeFactor;
    }

    pLeafNode = this->GetLeafContainer( this->OctreeRootNode, initialPt );
    pointIndx = this->FindClosestPointInLeafNode( pLeafNode, x, miniDist2 );
    elsePntId = this->FindClosestPointInSphereWithoutTolerance
                      ( x, *miniDist2,  pLeafNode, &elseDist2 );

    if (  elseDist2  <  ( *miniDist2 )  )
    {
      pointIndx  = elsePntId;
      *miniDist2 = elseDist2;
    }

    minBounds = maxBounds = NULL;
  }

  pLeafNode = NULL;
  return pointIndx;
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindClosestPointWithinRadius
  ( double radius, const double x[3], double & dist2 )
{
  this->BuildLocator();
  return this->FindClosestPointInSphereWithoutTolerance
               ( x, radius * radius, NULL, &dist2 );
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindClosestPointWithinSquaredRadius
  ( double radius2, const double x[3], double & dist2 )
{
  this->BuildLocator();
  return this->FindClosestPointInSphereWithoutTolerance
               ( x, radius2, NULL, &dist2 );
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::FindPointsWithinSquaredRadius
  ( vtkIncrementalOctreeNode * node, double radius2,
    const double point[3], vtkIdList * idList )
{
  int         i, j;
  int         numberPnts;
  double      tempValue0;
  double      tempValue1;
  double      pt2PtDist2;
  double      pointCoord[3];
  double      nodeBounds[6];
  double      outMinDst2 = 0.0; // min distance to the node: for outside point
  double      maximDist2 = 0.0; // max distance to the node: inside or outside
  vtkIdType   localIndex = 0;
  vtkIdType   pointIndex = 0;
  vtkIdList * nodePntIds = NULL;

  node->GetBounds( nodeBounds );

  for ( i = 0; i < 3; i ++ ) // for each axis
  {
    j = ( i << 1 );
    tempValue0 = point[i] - nodeBounds[j];
    tempValue1 = nodeBounds[ j + 1 ] - point[i];

    if ( tempValue0 < 0.0 )
    {
      outMinDst2 += tempValue0 * tempValue0;
      maximDist2 += tempValue1 * tempValue1;
    }
    else
    if ( tempValue1 < 0.0 )
    {
      outMinDst2 += tempValue1 * tempValue1;
      maximDist2 += tempValue0 * tempValue0;
    }
    else
    if ( tempValue1 > tempValue0 )
    {
      maximDist2 += tempValue1 * tempValue1;
    }
    else
    {
      maximDist2 += tempValue0 * tempValue0;
    }
  }

  if ( outMinDst2 > radius2 )
  {
    // the node is totally outside the search sphere
    return;
  }

  if ( maximDist2 <= radius2 )
  {
    // the node is totally inside the search sphere
    node->ExportAllPointIdsByInsertion( idList );
    return;
  }

  // the node intersects with, but is not totally inside, the search sphere
  if ( node->IsLeaf() )
  {
    numberPnts = node->GetNumberOfPoints();
    nodePntIds = node->GetPointIdSet();

    for ( localIndex = 0; localIndex < numberPnts; localIndex ++ )
    {
      pointIndex = nodePntIds->GetId( localIndex );
      this->LocatorPoints->GetPoint( pointIndex, pointCoord );

      pt2PtDist2 = vtkMath::Distance2BetweenPoints( pointCoord, point );
      if ( pt2PtDist2 <= radius2 )
      {
        idList->InsertNextId( pointIndex );
      }
    }

    nodePntIds = NULL;
  }
  else
  {
    for ( i = 0; i < 8; i ++ )
    {
      this->FindPointsWithinSquaredRadius(  node->GetChild( i ),
                                            radius2,  point,  idList  );
    }
  }
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::FindPointsWithinSquaredRadius
  ( double R2, const double x[3], vtkIdList * result )
{
  result->Reset();
  this->BuildLocator();
  this->FindPointsWithinSquaredRadius( this->OctreeRootNode, R2, x, result );
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::FindPointsWithinRadius( double R,
  const double x[3], vtkIdList * result )
{
  result->Reset();
  this->BuildLocator();
  this->FindPointsWithinSquaredRadius
        ( this->OctreeRootNode, R * R, x, result );
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::FindClosestNPoints
  ( int N, const double x[3], vtkIdList * result )
{
  result->Reset();
  this->BuildLocator();

  int  totalPnts = this->OctreeRootNode->GetNumberOfPoints(); // possibly 0

  if ( N > totalPnts )
  {
    N = totalPnts;
    vtkWarningMacro( "Number of requested points > that of available points" );
  }

  if ( N <= 0 )
  {
    vtkWarningMacro( "invalid N or the octree is still empty" );
    return;
  }


  // We are going to find the lowest-possible node to start with, startNode,
  // by using a top-down recursive search mechanism. Such a starting node
  // belongs to one of the following cases (numPoints: number of points in or
  // under startNode).
  //
  // (1) startNode is a     leaf node AND numPoints = N
  // (2) startNode is a     leaf node AND numPoints > N
  // (3) startNode is a non-leaf node AND numPoints = N
  // (4) startNode is a non-leaf node AND numPoints > N
  //
  // * case 4 occurs, when none of the other three cases holds, by going one
  //   level up --- one-step regression.
  //
  // * The point may be outside startNode, as is usually the case, even if it
  //   is inside the octree root node. To address such scenarios, the initial
  //   point-inside-the-node case might be followed by the point-outside-the-
  //   node case to quickly locate the most compact startNode. Otherwise the
  //   resulting startNode might contain a huge number of points, which would
  //   significantly degrade the search performance.

  int         i;
  int         beenFound;
  int         numPoints;
  double      tempDist2;
  double      miniDist2;
  double      maxiDist2;
  double      pntCoords[3];
  vtkIdType   pointIndx;
  vtkIdList * pntIdList = NULL;
  vtkIncrementalOctreeNode * startNode = NULL;
  vtkIncrementalOctreeNode * pTheChild = NULL;
  vtkIncrementalOctreeNode * pThisNode = this->OctreeRootNode;
  vtkIncrementalOctreeNode * theParent = pThisNode;
  std::queue< vtkIncrementalOctreeNode * > nodeQueue;

  beenFound = 0;
  numPoints = pThisNode->GetNumberOfPoints();
  while ( beenFound == 0 )
  {
    if (  pThisNode->ContainsPoint( x )  )      // point inside the node
    {
      while (  !pThisNode->IsLeaf()  &&  numPoints > N  )
      {
        theParent = pThisNode;
        pThisNode = pThisNode->GetChild(  pThisNode->GetChildIndex( x )  );
        numPoints = pThisNode->GetNumberOfPoints();
      }

      if ( numPoints )
      {
        // The point is still inside pThisNode
        beenFound = 1;
        pThisNode = ( numPoints >= N ) ? pThisNode : theParent;
      }
      else
      {
        // The point is inside an empty node (pThisNode), but outside the node
        // with closest points --- the closest node (a sibling of pThisNode).
        // We need to locate this closest node via the parent node and proceed
        // with it (the closest node) further in search for startNode, but by
        // means of the other case (point outside the node).
        miniDist2 = VTK_DOUBLE_MAX;
        for ( i = 0; i < 8; i ++ )
        {
          pTheChild = theParent->GetChild( i );
          tempDist2 = pTheChild->GetDistance2ToBoundary
                                 ( x, this->OctreeRootNode, 1 );
          if ( tempDist2 < miniDist2 )
          {
            miniDist2 = tempDist2;
            pThisNode = pTheChild;
          }
        }
      }
    }
    else                                        // point outside the node
    {
      while (  !pThisNode->IsLeaf()  &&  numPoints  >  N  )
      {
        // find the child closest (in terms of data) to the given point
        theParent = pThisNode;
        miniDist2 = VTK_DOUBLE_MAX;
        for ( i = 0; i < 8; i ++ )
        {
          pTheChild = theParent->GetChild( i );
          tempDist2 = pTheChild->GetDistance2ToBoundary
                                 ( x, this->OctreeRootNode, 1 );
          if ( tempDist2 < miniDist2 )
          {
            miniDist2 = tempDist2;
            pThisNode = pTheChild;
          }
        }
        numPoints = pThisNode->GetNumberOfPoints();
      }

      beenFound = 1;
      pThisNode = ( numPoints >= N ) ? pThisNode : theParent;
    }

    // update the number of points in the node in case of a switch from point-
    // inside-the-node to point-outside-the-node.
    numPoints = pThisNode->GetNumberOfPoints();
  }

  // this is where we can get the really most compact starting node
  startNode = pThisNode;
  numPoints = startNode->GetNumberOfPoints();


  // Given the starting node, we select the points inside it and sort them
  SortPoints  ptsSorter( N );
  pointIndx = 0;
  pntIdList = vtkIdList::New();
  pntIdList->SetNumberOfIds( numPoints );
  startNode->ExportAllPointIdsByDirectSet( &pointIndx, pntIdList );

  for ( i = 0; i < numPoints; i ++ )
  {
    pointIndx = pntIdList->GetId( i );
    this->LocatorPoints->GetPoint( pointIndx, pntCoords );
    tempDist2 = vtkMath::Distance2BetweenPoints( x, pntCoords );
    ptsSorter.InsertPoint( tempDist2, pointIndx );
  }


  // We still need to check other nodes in case they contain closer points
  nodeQueue.push( this->OctreeRootNode );
  maxiDist2 = ptsSorter.GetLargestDist2();
  while ( !nodeQueue.empty() )
  {
    pThisNode = nodeQueue.front();
    nodeQueue.pop();

    // skip the start node as we have just processed it
    if ( pThisNode == startNode )
    {
      continue;
    }

    if ( !pThisNode->IsLeaf() )
    {
      // this is a non-leaf node and we need to push some children if necessary
      for ( i = 0; i < 8; i ++ )
      {
        pTheChild = pThisNode->GetChild( i );
        if (    pTheChild->ContainsPointByData( x ) == 1
             || pTheChild->GetDistance2ToBoundary( x, this->OctreeRootNode, 1 )
                < maxiDist2
           )
        {
          nodeQueue.push( pTheChild );
        }
      }
    }
    else
    if (  pThisNode->GetDistance2ToBoundary( x, this->OctreeRootNode, 1 )
        < maxiDist2  )
    {
      // This is a leaf node AND its data bounding box is close enough for us
      // to process the points inside the node. Note that the success of the
      // above distance check indicates that there is at least one point in
      // the node. Otherwise the point-to-node distance (in terms of data)
      // would be VTK_DOUBLE_MAX.

      // obtain the point indices
      numPoints = pThisNode->GetNumberOfPoints();
      pointIndx = 0;
      pntIdList->Reset();
      pntIdList->SetNumberOfIds( numPoints );
      pThisNode->ExportAllPointIdsByDirectSet( &pointIndx, pntIdList );

      // insert the points to the sorter if necessary
      for ( i = 0; i < numPoints; i ++ )
      {
        pointIndx = pntIdList->GetId( i );
        this->LocatorPoints->GetPoint( pointIndx, pntCoords );
        tempDist2 = vtkMath::Distance2BetweenPoints( x, pntCoords );
        ptsSorter.InsertPoint( tempDist2, pointIndx );
      }

      // as we might have inserted some points, we need to update maxiDist2
      maxiDist2 = ptsSorter.GetLargestDist2();
    }
  }


  // obtain the point indices
  result->SetNumberOfIds( N );
  ptsSorter.GetSortedIds( result );


  // release memory
  pntIdList->Delete();
  pntIdList = NULL;
  startNode = NULL;
  pTheChild = NULL;
  pThisNode = NULL;
  theParent = NULL;
}

// ---------------------------------------------------------------------------
// ----------------------------- Point Insertion -----------------------------
// ---------------------------------------------------------------------------

//----------------------------------------------------------------------------
int vtkIncrementalOctreePointLocator::InitPointInsertion( vtkPoints * points,
  const double bounds[6] )
{
  return this->InitPointInsertion( points, bounds, 0 );
}

//----------------------------------------------------------------------------
int vtkIncrementalOctreePointLocator::InitPointInsertion( vtkPoints * points,
  const double bounds[6], vtkIdType vtkNotUsed( estNumPts ) )
{
  int     i,  bbIndex;
  double  dimDiff[3], tmpBbox[6];

  if ( points == NULL )
  {
    vtkErrorMacro( << "a valid vtkPoints object required for point insertion" );
    return 0;
  }

  // destroy the existing octree, if any
  this->FreeSearchStructure();

  // detach the old vtkPoints object, if any, before attaching a new one
  if ( this->LocatorPoints != NULL )
  {
    this->LocatorPoints->UnRegister( this );
  }
  this->LocatorPoints = points;
  this->LocatorPoints->Register( this );

  // obtain the threshold squared distance
  this->InsertTolerance2 = this->Tolerance * this->Tolerance;

  // Fix bounds
  // (1) push out a little bit if the original volume is too flat --- a slab
  // (2) pull back the x, y, and z's lower bounds a little bit such that
  //     points are clearly "inside" the spatial region.  Point p is taken as
  //     "inside" range r = [r1, r2] if and only if r1 < p <= r2.
  this->OctreeMaxDimSize = 0.0;
  for ( i = 0; i < 3; i ++ )
  {
    bbIndex = ( i << 1 );
    tmpBbox[ bbIndex     ] = bounds[ bbIndex     ];
    tmpBbox[ bbIndex + 1 ] = bounds[ bbIndex + 1 ];
    dimDiff[i] = tmpBbox[ bbIndex + 1 ] - tmpBbox[ bbIndex ];
    this->OctreeMaxDimSize = ( dimDiff[i] > this->OctreeMaxDimSize )
                             ? dimDiff[i] : this->OctreeMaxDimSize;
  }

  if ( this->BuildCubicOctree )
  {
    // make the bounding box a cube and hence descendant octants cubes too
    for ( i = 0; i < 3; i ++ )
    {
      if ( dimDiff[i] != this->OctreeMaxDimSize )
      {
        double delta = this->OctreeMaxDimSize - dimDiff[i];
        tmpBbox[   i << 1       ] -= 0.5 * delta;
        tmpBbox[ ( i << 1 ) + 1 ] += 0.5 * delta;
        dimDiff[i] = this->OctreeMaxDimSize;
      }
    }
  }

  this->FudgeFactor  = this->OctreeMaxDimSize * 10e-6;
  double minSideSize = this->OctreeMaxDimSize * 10e-2;

  for ( i = 0; i < 3; i ++ )
  {
    if ( dimDiff[i] < minSideSize ) // case (1) above
    {
      bbIndex  =  ( i << 1 );
      double  tempVal = tmpBbox[ bbIndex ];
      tmpBbox[ bbIndex     ] = tmpBbox[ bbIndex + 1 ] - minSideSize;
      tmpBbox[ bbIndex + 1 ] = tempVal + minSideSize;
    }
    else                             // case (2) above
    {
      tmpBbox[ i << 1 ] -= this->FudgeFactor;
    }
  }

  // init the octree with an empty leaf node
  this->OctreeRootNode = vtkIncrementalOctreeNode::New();

  // this call internally inits the middle (center) and data range, too
  this->OctreeRootNode->SetBounds( tmpBbox[0], tmpBbox[1],
                                   tmpBbox[2], tmpBbox[3],
                                   tmpBbox[4], tmpBbox[5] );

  return 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindClosestPointInSphereWithTolerance
  ( const double point[3], double radius2,
    vtkIncrementalOctreeNode * maskNode, double * minDist2 )
{
  *minDist2 = this->OctreeMaxDimSize * this->OctreeMaxDimSize * 4.0;
  return this->FindClosestPointInSphere( point, radius2, maskNode,
                                               minDist2, &radius2 );
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindDuplicateFloatTypePointInVisitedLeafNode
  ( vtkIncrementalOctreeNode * leafNode, const double point[3] )
{
  int         numPts = 0;
  float       thePnt[3];
  float *     pFloat = NULL;
  float *     tmpPnt = NULL;
  vtkIdType   tmpIdx = -1;
  vtkIdType   pntIdx = -1;
  vtkIdList * idList = NULL;

  thePnt[0] = static_cast< float >( point[0] );
  thePnt[1] = static_cast< float >( point[1] );
  thePnt[2] = static_cast< float >( point[2] );

  idList = leafNode->GetPointIdSet();
  numPts = idList->GetNumberOfIds( );
  pFloat = (  static_cast< vtkFloatArray * > ( this->LocatorPoints->GetData() )  )
           ->GetPointer( 0 );

  for ( int i = 0; i < numPts; i ++ )
  {
    tmpIdx = idList->GetId( i );
    tmpPnt = pFloat + (  ( tmpIdx << 1 )  +  tmpIdx  );

    if (  ( thePnt[0] == tmpPnt[0] ) &&
          ( thePnt[1] == tmpPnt[1] ) &&
          ( thePnt[2] == tmpPnt[2] )
       )
    {
      pntIdx = tmpIdx;
      break;
    }
  }

  pFloat = NULL;
  tmpPnt = NULL;
  idList = NULL;

  return pntIdx;
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindDuplicateDoubleTypePointInVisitedLeafNode
  ( vtkIncrementalOctreeNode * leafNode, const double point[3] )
{
  int         numPts = 0;
  double *    pArray = NULL;
  double *    tmpPnt = NULL;
  vtkIdType   tmpIdx = -1;
  vtkIdType   pntIdx = -1;
  vtkIdList * idList = NULL;

  idList = leafNode->GetPointIdSet();
  numPts = idList->GetNumberOfIds( );
  pArray = (  static_cast< vtkDoubleArray * > ( this->LocatorPoints->GetData() )  )
           ->GetPointer( 0 );

  for ( int i = 0; i < numPts; i ++ )
  {
    tmpIdx = idList->GetId( i );
    tmpPnt = pArray + (  ( tmpIdx << 1 )  +  tmpIdx  );

    if (  ( point[0] == tmpPnt[0] ) &&
          ( point[1] == tmpPnt[1] ) &&
          ( point[2] == tmpPnt[2] )
       )
    {
      pntIdx = tmpIdx;
      break;
    }
  }

  pArray = NULL;
  tmpPnt = NULL;
  idList = NULL;

  return pntIdx;
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::FindDuplicatePointInLeafNode
  ( vtkIncrementalOctreeNode * leafNode, const double point[3] )
{
  if ( leafNode->GetPointIdSet() == NULL )
  {
    return -1;
  }

  return ( this->LocatorPoints->GetDataType() == VTK_FLOAT )
         ? this->FindDuplicateFloatTypePointInVisitedLeafNode
                 ( leafNode, point )
         : this->FindDuplicateDoubleTypePointInVisitedLeafNode
                 ( leafNode, point );
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::IsInsertedPointForZeroTolerance
  ( const double x[3], vtkIncrementalOctreeNode ** leafContainer )
{
  // the target leaf node always exists there since the root node of the
  // octree has been initialized to cover all possible points to be inserted
  // and therefore we do not need to check it here
  *leafContainer = this->GetLeafContainer( this->OctreeRootNode, x );
  vtkIdType  pointIdx = this->FindDuplicatePointInLeafNode
                              ( *leafContainer, x );
  return pointIdx;
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::IsInsertedPointForNonZeroTolerance
  ( const double x[3], vtkIncrementalOctreeNode ** leafContainer )
{
  double     minDist2;  // min distance to ALL existing points
  double     elseDst2;  // min DiSTance to other nodes (inner boundaries)
  double     dist2Ext;  // min distance to an EXTended set of nodes
  vtkIdType  pntIdExt;

  // the target leaf node always exists there since the root node of the
  // octree has been initialized to cover all possible points to be inserted
  // and therefore we do not need to check it here
  *leafContainer = this->GetLeafContainer( this->OctreeRootNode, x );
  vtkIdType  pointIdx = this->FindClosestPointInLeafNode
                              ( *leafContainer, x, &minDist2 );

  if ( minDist2 == 0.0 )
  {
    return pointIdx;
  }

  // As no any 'duplicate' point exists in this leaf node, we need to expand
  // the search scope to capture possible closer points in other nodes.
  elseDst2 = ( *leafContainer )->GetDistance2ToInnerBoundary
                                 ( x, this->OctreeRootNode );

  if ( elseDst2 < this->InsertTolerance2 )
  {
    // one or multiple closer points might exist in the neighboring nodes
    pntIdExt = this->FindClosestPointInSphereWithTolerance
                     (  x, this->InsertTolerance2, *leafContainer, &dist2Ext );

    if ( dist2Ext < minDist2 )
    {
      minDist2 = dist2Ext;
      pointIdx = pntIdExt;
    }
  }

  return  ( minDist2 <= this->InsertTolerance2 )  ?  pointIdx  :  -1;
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::IsInsertedPoint
  ( double x, double  y, double z )
{
  double xyz[3] = { x, y, z };
  return this->IsInsertedPoint( xyz );
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::IsInsertedPoint( const double x[3] )
{
  vtkIncrementalOctreeNode * leafContainer = NULL;
  return this->IsInsertedPoint( x, &leafContainer );
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::IsInsertedPoint
  ( const double x[3], vtkIncrementalOctreeNode ** leafContainer )
{
  return ( this->InsertTolerance2 == 0.0 )
         ? this->IsInsertedPointForZeroTolerance( x, leafContainer )
         : this->IsInsertedPointForNonZeroTolerance( x, leafContainer );
}

//----------------------------------------------------------------------------
int vtkIncrementalOctreePointLocator::InsertUniquePoint( const double point[3],
                                                         vtkIdType & pntId )
{
  vtkIncrementalOctreeNode * leafContainer = NULL;
  pntId = this->IsInsertedPoint( point, &leafContainer );
  return (    ( pntId > -1 )
           ?  0
           :  leafContainer->InsertPoint( this->LocatorPoints,     point,
                                          this->MaxPointsPerLeaf, &pntId, 2 )
         );
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::InsertPointWithoutChecking
  ( const double point[3], vtkIdType & pntId, int insert )
{
  this->GetLeafContainer( this->OctreeRootNode, point )
      ->InsertPoint( this->LocatorPoints,     point,
                     this->MaxPointsPerLeaf, &pntId, ( insert << 1 ) );
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreePointLocator::InsertPoint
  ( vtkIdType ptId, const double x[3] )
{
  this->GetLeafContainer( this->OctreeRootNode, x )
      ->InsertPoint( this->LocatorPoints, x, this->MaxPointsPerLeaf, &ptId, 1 );
}

//----------------------------------------------------------------------------
vtkIdType vtkIncrementalOctreePointLocator::InsertNextPoint( const double x[3] )
{
  vtkIdType  pntId = -1;
  this->GetLeafContainer( this->OctreeRootNode, x )
      ->InsertPoint( this->LocatorPoints, x, this->MaxPointsPerLeaf, &pntId, 2 );
  return pntId;
}

