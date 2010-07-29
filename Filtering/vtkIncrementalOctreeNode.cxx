/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIncrementalOctreeNode.cxx

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
#include "vtkObjectFactory.h"
#include "vtkIncrementalOctreeNode.h"

vtkStandardNewMacro( vtkIncrementalOctreeNode );

vtkCxxSetObjectMacro( vtkIncrementalOctreeNode, PointIdSet, vtkIdList );
vtkCxxSetObjectMacro( vtkIncrementalOctreeNode, Parent, vtkIncrementalOctreeNode );

// ---------------------------------------------------------------------------
// ----------------------------- Helper functions ----------------------------
// ---------------------------------------------------------------------------

//---------------------------------------------------------------------------- 
// This is an empty function which provides only the point index to the caller
// function vtkIncreemntalOctreeNode::InsertPoint(). The caller inserts the
// point index to the vtkIdList maintained by a leaf node, without inserting
// the point (coordinate) to vtkPoints at all.
void _OctreeNodeGetPointId( vtkPoints * vtkNotUsed( points ), 
  vtkIdType * vtkNotUsed( pntIdx ), const double * vtkNotUsed( coords ) )
{
  // the 3D point coordinate is not inserted to vtkPoints at all
}

//----------------------------------------------------------------------------  
// Insert a point, with a specified point index, to a vtkPoints object by
// calling vtkPoints::InsertPoint().
void _OctreeNodeInsertPoint( vtkPoints * points, vtkIdType * pntIdx, 
                             const double * coords )
{
  points->InsertPoint( *pntIdx, coords );
}

//----------------------------------------------------------------------------
// Insert a point to a vtkPoints by calling vtkPoints::InsertNextPoint().
void _OctreeNodeInsertNextPoint( vtkPoints * points, vtkIdType * pntIdx, 
                                 const double * coords )
{
  *pntIdx = points->InsertNextPoint( coords );
}

//----------------------------------------------------------------------------
// Function pointers in support of three point insertion modes.
typedef void ( * OCTREENODE_INSERTPOINT_FUNCTION )
  ( vtkPoints * points, vtkIdType * pntIdx, const double * coords );
             
OCTREENODE_INSERTPOINT_FUNCTION OCTREENODE_INSERTPOINT[3] =
{ 
  _OctreeNodeGetPointId, 
  _OctreeNodeInsertPoint, 
  _OctreeNodeInsertNextPoint 
};

// ---------------------------------------------------------------------------
// ------------------------- vtkIncrementalOctreeNode ------------------------
// ---------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkIncrementalOctreeNode::vtkIncrementalOctreeNode()
{
  this->Parent = NULL;
  this->Children = NULL;
  this->PointIdSet = NULL; 
  this->NumberOfPoints = 0;
  
  // unnecessary to initialize spatial and data bounding boxes here as 
  // SetBounds() are always called by the user for the root node of an
  // octree and this function is also always called for any decendant 
  // node upon node-subdivison.  for now though we'll set them to
  // something to avoid uninitialized variable warnings
  for(int i=0;i<3;i++)
    {
    this->MinBounds[i] = this->MinDataBounds[i] = VTK_DOUBLE_MIN;
    this->MaxBounds[i] = this->MaxDataBounds[i] = VTK_DOUBLE_MAX;
    }
}

//----------------------------------------------------------------------------
vtkIncrementalOctreeNode::~vtkIncrementalOctreeNode()
{
  if ( this->Parent )
    {
    this->Parent->UnRegister( this );
    this->Parent = NULL;
    }

  this->DeleteChildNodes();
  this->DeletePointIdSet();
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreeNode::DeleteChildNodes()
{     
  if ( this->Children )
    {
    for ( int i = 0; i < 8; i ++ )
      {
      this->Children[i]->Delete();
      this->Children[i] = NULL;
      }
      
    delete [] this->Children;
    this->Children = NULL;
    }
}   

//----------------------------------------------------------------------------
void vtkIncrementalOctreeNode::CreatePointIdSet( int initSize, int growSize )
{
  if ( this->PointIdSet == NULL )
    {
    this->PointIdSet = vtkIdList::New();
    this->PointIdSet->Allocate( initSize, growSize ); 
    }
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreeNode::DeletePointIdSet()
{
  if ( this->PointIdSet )
    {
    this->PointIdSet->Delete();
    this->PointIdSet = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreeNode::SetBounds( double x1, double x2, double y1,
                                          double y2, double z1, double z2 )
{
  this->MinBounds[0] = x1;
  this->MaxBounds[0] = x2;
  this->MinBounds[1] = y1;     
  this->MaxBounds[1] = y2;
  this->MinBounds[2] = z1;     
  this->MaxBounds[2] = z2;
  
  this->MinDataBounds[0] = x2;     
  this->MaxDataBounds[0] = x1;
  this->MinDataBounds[1] = y2;     
  this->MaxDataBounds[1] = y1;
  this->MinDataBounds[2] = z2;     
  this->MaxDataBounds[2] = z1;
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreeNode::GetBounds( double bounds[6] ) const
{
   bounds[0] = this->MinBounds[0];
   bounds[1] = this->MaxBounds[0];
   bounds[2] = this->MinBounds[1];
   bounds[3] = this->MaxBounds[1];
   bounds[4] = this->MinBounds[2];
   bounds[5] = this->MaxBounds[2];
}

//----------------------------------------------------------------------------
int vtkIncrementalOctreeNode::UpdateCounterAndDataBounds
  ( const double point[3], int nHits, int updateData )
{
  this->NumberOfPoints += nHits;
  
  if ( updateData == 0 ) return 0;
  
  int  updated = 0;
  
  if ( point[0] < this->MinDataBounds[0] )
    {
    updated = 1;
    this->MinDataBounds[0] = point[0];
    }
  if ( point[0] > this->MaxDataBounds[0] ) 
    {
    updated = 1;
    this->MaxDataBounds[0] = point[0];
    }
    
  if ( point[1] < this->MinDataBounds[1] ) 
    {
    updated = 1;
    this->MinDataBounds[1] = point[1];
    }
  if ( point[1] > this->MaxDataBounds[1] ) 
    {
    updated = 1;
    this->MaxDataBounds[1] = point[1];
    }
    
  if ( point[2] < this->MinDataBounds[2] ) 
    {
    updated = 1;
    this->MinDataBounds[2] = point[2];
    }
  if ( point[2] > this->MaxDataBounds[2] ) 
    {
    updated = 1;
    this->MaxDataBounds[2] = point[2];
    }
    
  return updated;
}

//----------------------------------------------------------------------------
// Given the index (0 ~ 7) of a child node, the spatial bounding axis (0 ~ 2
// for x, y, and z), and the value (0 ~ 1 for min and max) to access, this LUT
// allows for rapid assignment of its spatial bounding box --- MinBounds[3]
// and MaxBounds[3], with each specific value or entry of this LUT pointing to
// MinBounds[3] for 0, center point for 1, or MaxBounds[3] for 2.
static int OCTREE_CHILD_BOUNDS_LUT[8][3][2] = 
{
  {  { 0, 1 },  { 0, 1 },  { 0, 1 }  },
  {  { 1, 2 },  { 0, 1 },  { 0, 1 }  },
  {  { 0, 1 },  { 1, 2 },  { 0, 1 }  },
  {  { 1, 2 },  { 1, 2 },  { 0, 1 }  },
     
  {  { 0, 1 },  { 0, 1 },  { 1, 2 }  },
  {  { 1, 2 },  { 0, 1 },  { 1, 2 }  },
  {  { 0, 1 },  { 1, 2 },  { 1, 2 }  },
  {  { 1, 2 },  { 1, 2 },  { 1, 2 }  }
};

//----------------------------------------------------------------------------
void vtkIncrementalOctreeNode::SeperateExactlyDuplicatePointsFromNewInsertion
  ( vtkPoints * points, vtkIdList * pntIds, const double newPnt[3], 
    vtkIdType * pntIdx, int maxPts, int ptMode )
{
  // the number of points already maintained in this leaf node
  // >= maxPts AND all of them are exactly duplicate with one another
  //           BUT the new point is  not a duplicate of them any more
  
  int         i;
  double      dupPnt[3];
  double      octMin[3];
  double      octMid[3];
  double      octMax[3];
  double *    boxPtr[3] = { NULL, NULL, NULL };
  vtkIncrementalOctreeNode * ocNode = NULL;
  vtkIncrementalOctreeNode * duplic = this;
  vtkIncrementalOctreeNode * single = this;
  
  // the coordiate of the duplicate points: note pntIds == this->PointIdSet
  points->GetPoint(  pntIds->GetId( 0 ),  dupPnt  );
  
  while ( duplic == single ) // as long as separation has not been achieved
    {
    // update the current (in recursion) node and access the bounding box info
    ocNode    = duplic;
    octMid[0] = ( ocNode->MinBounds[0] + ocNode->MaxBounds[0] ) * 0.5;
    octMid[1] = ( ocNode->MinBounds[1] + ocNode->MaxBounds[1] ) * 0.5;
    octMid[2] = ( ocNode->MinBounds[2] + ocNode->MaxBounds[2] ) * 0.5;
    boxPtr[0] = ocNode->MinBounds;
    boxPtr[1] = octMid;
    boxPtr[2] = ocNode->MaxBounds;
    
    // create eight child nodes
    ocNode->Children = new vtkIncrementalOctreeNode * [8];
    for ( i = 0; i < 8; i ++ )
      {
      // x-bound: axis 0
      octMin[0] = boxPtr[ OCTREE_CHILD_BOUNDS_LUT[i][0][0] ] [0];
      octMax[0] = boxPtr[ OCTREE_CHILD_BOUNDS_LUT[i][0][1] ] [0];
    
      // y-bound: axis 1
      octMin[1] = boxPtr[ OCTREE_CHILD_BOUNDS_LUT[i][1][0] ] [1];
      octMax[1] = boxPtr[ OCTREE_CHILD_BOUNDS_LUT[i][1][1] ] [1];
    
      // z-bound: axis 2
      octMin[2] = boxPtr[ OCTREE_CHILD_BOUNDS_LUT[i][2][0] ] [2];
      octMax[2] = boxPtr[ OCTREE_CHILD_BOUNDS_LUT[i][2][1] ] [2];
      
      ocNode->Children[i] = vtkIncrementalOctreeNode::New();
      ocNode->Children[i]->SetParent( ocNode );
      ocNode->Children[i]->SetBounds( octMin[0], octMax[0],
                                      octMin[1], octMax[1],
                                      octMin[2], octMax[2] );
      }
    
    // determine the leaf node of the duplicate points & that of the new point
    duplic = ocNode->Children[  ocNode->GetChildIndex( dupPnt )  ];
    single = ocNode->Children[  ocNode->GetChildIndex( newPnt )  ];
    }
  boxPtr[0] = NULL;
  boxPtr[1] = NULL;
  boxPtr[2] = NULL;
  
  // Now the duplicate points have been separated from the new point //
  
  // create a vtkIdList object for the new point
  // update the counter and the data bounding box until the root node
  // (including the root node)
  OCTREENODE_INSERTPOINT[ptMode] ( points, pntIdx, newPnt );
  single->CreatePointIdSet(  ( maxPts >> 2 ),  ( maxPts >> 1 )  );  
  single->GetPointIdSet()->InsertNextId( *pntIdx );
  single->UpdateCounterAndDataBoundsRecursively( newPnt, 1, 1, NULL );
  
  // We just need to reference pntIds while un-registering it from 'this'.
  // This avoids deep-copying point ids from pntIds to duplic's PointIdSet.
  // update the counter and the data bounding box, but until 'this' node
  // (excluding 'this' node)
  duplic->SetPointIdSet( pntIds );
  duplic->UpdateCounterAndDataBoundsRecursively
          ( dupPnt, pntIds->GetNumberOfIds(), 1, this );
  
  // handle memory
  ocNode = NULL;
  duplic = NULL;
  single = NULL;
}

//----------------------------------------------------------------------------
int vtkIncrementalOctreeNode::CreateChildNodes
  ( vtkPoints * points, vtkIdList * pntIds, const double newPnt[3],
    vtkIdType * pntIdx, int maxPts, int ptMode )
{ 
  // There are two scenarios for which this function is invoked.
  //
  // (1) the number of points already maintained in this leaf node
  //     == maxPts AND not all of them are exactly duplicate
  //               AND the new point is  not a duplicate of them all
  // (2) the number of points already maintained in this leaf node
  //     >= maxPts AND all of them are exactly duplicate with one another
  //               BUT the new point is  not a duplicate of them any more
  
  // address case (2) first if necessary
  double    sample[3];
  points->GetPoint(  pntIds->GetId( 0 ),  sample  );
  if (  this->ContainsDuplicatePointsOnly( sample )  ==  1  )
    {
    this->SeperateExactlyDuplicatePointsFromNewInsertion
          ( points, pntIds, newPnt, pntIdx, maxPts, ptMode );
    
    // notify vtkIncrementalOctreeNode::InsertPoint() that pntIds just needs
    // to be unregistered from 'this', but must NOT be destroyed at all.
    return 0;
    }
    
  // then address case (1) below
  
  int       i;
  int       target;
  int       dvidId = -1; // index of the sub-dividing octant, if any
  int       fullId = -1; // index of the full octant, if any
  int       numIds[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  double    octMin[3];
  double    octMax[3];
  double    tempPt[3];
  vtkIdType tempId;
  
  double    octMid[3] = {  ( this->MinBounds[0] + this->MaxBounds[0] ) * 0.5,
                           ( this->MinBounds[1] + this->MaxBounds[1] ) * 0.5,
                           ( this->MinBounds[2] + this->MaxBounds[2] ) * 0.5
                        };
  double *  boxPtr[3];
  boxPtr[0] = this->MinBounds;
  boxPtr[1] = octMid;
  boxPtr[2] = this->MaxBounds;
  
  // create eight child nodes
  this->Children = new vtkIncrementalOctreeNode * [8];
  for ( i = 0; i < 8; i ++ )
    {
    // x-bound: axis 0
    octMin[0] = boxPtr[ OCTREE_CHILD_BOUNDS_LUT[i][0][0] ] [0];
    octMax[0] = boxPtr[ OCTREE_CHILD_BOUNDS_LUT[i][0][1] ] [0];
    
    // y-bound: axis 1
    octMin[1] = boxPtr[ OCTREE_CHILD_BOUNDS_LUT[i][1][0] ] [1];
    octMax[1] = boxPtr[ OCTREE_CHILD_BOUNDS_LUT[i][1][1] ] [1];
    
    // z-bound: axis 2
    octMin[2] = boxPtr[ OCTREE_CHILD_BOUNDS_LUT[i][2][0] ] [2];
    octMax[2] = boxPtr[ OCTREE_CHILD_BOUNDS_LUT[i][2][1] ] [2];
      
    // This call internally sets the cener and default data bounding box, too. 
    this->Children[i] = vtkIncrementalOctreeNode::New();
    this->Children[i]->SetParent( this );
    this->Children[i]->SetBounds( octMin[0], octMax[0],
                                  octMin[1], octMax[1],
                                  octMin[2], octMax[2] );
      
    // allocate a list of point-indices (size = 2^n) for index registration
    this->Children[i]->CreatePointIdSet( ( maxPts >> 2 ), ( maxPts >> 1 ) );
    }
  boxPtr[0] = NULL;
  boxPtr[1] = NULL;
  boxPtr[2] = NULL;
      
  // distribute the available point-indices to the eight child nodes
  for ( i = 0; i < maxPts; i ++ )
    {
    tempId = pntIds->GetId( i );
    points->GetPoint( tempId, tempPt );
    target = this->GetChildIndex( tempPt );
    this->Children[ target ]->GetPointIdSet()->InsertNextId( tempId );
    this->Children[ target ]->UpdateCounterAndDataBounds( tempPt );
    numIds[ target ] ++;
    }  
  
  // locate the full child, just if any
  for ( i = 0; i < 8; i ++ )
    {
    if ( numIds[i] == maxPts )
      {
      fullId = i;
      break;
      }
    }
    
  target = this->GetChildIndex( newPnt );  
  if ( fullId == target ) 
    {
    // The fact is that we are going to insert the new point to an already
    // full octant (child node). Thus we need to further divide this child
    // to avoid the overflow problem.
    this->Children[ target ]->CreateChildNodes( points, pntIds, newPnt,
                                                pntIdx, maxPts, ptMode );
    dvidId = fullId;
    }
  else
    {
    // the initial division is a success
    OCTREENODE_INSERTPOINT[ptMode] ( points, pntIdx, newPnt );
    this->Children[ target ]->GetPointIdSet()->InsertNextId( *pntIdx );
    this->Children[ target ]->UpdateCounterAndDataBoundsRecursively
                              ( newPnt, 1, 1, NULL );
    
    // NOTE: The counter below might reach the threshold, though we delay the
    // sub-division of this child node until the next point insertion occurs.
    numIds[ target ] ++;
    }
    
  // Now it is time to reclaim those un-used vtkIdList objects, of which each
  // either is empty or still needs to be deleted due to further division of
  // the child node. This post-deallocation of the un-used vtkIdList objects 
  // (of some child nodes) is based on the assumption that retrieving the 
  // 'maxPts' points from vtkPoints and the associated 'maxPts' point-indices 
  // from vtkIdList is more expensive than reclaiming at most 8 vtkIdList 
  // objects at hand.
  for ( i = 0; i < 8; i ++ )
    {
    if ( numIds[i] == 0 || i == dvidId )
      {
      this->Children[i]->DeletePointIdSet();
      }
    }
  
  // notify vtkIncrementalOctreeNode::InsertPoint() to destroy pntIds
  return 1;
}

//---------------------------------------------------------------------------- 
int  vtkIncrementalOctreeNode::InsertPoint( vtkPoints * points,
  const double newPnt[3], int maxPts, vtkIdType * pntId, int ptMode )
{     
  if ( this->PointIdSet )
    {
    // there has been at least one point index 
    if (    this->PointIdSet->GetNumberOfIds() < maxPts
         || this->ContainsDuplicatePointsOnly( newPnt ) == 1
       )
      {
      // this leaf node is not full or
      // this leaf node is full, but of all exactly duplicate points
      // and the point under check is another duplicate of these points
      OCTREENODE_INSERTPOINT[ptMode] ( points, pntId, newPnt );
      this->PointIdSet->InsertNextId( *pntId );
      this->UpdateCounterAndDataBoundsRecursively( newPnt, 1, 1, NULL );
      }
    else
      { 
      // overflow: divide this node and delete the list of point-indices.
      // Note that the number of exactly duplicate points might be greater
      // than or equal to maxPts.
      if ( this->CreateChildNodes( points, this->PointIdSet,
                                   newPnt, pntId, maxPts, ptMode )
         )
        { 
        this->PointIdSet->Delete();
        }
      else
        {
        this->PointIdSet->UnRegister( this );
        }
      this->PointIdSet = NULL;
      }
    }
  else
    { 
    // there has been no any point index registered in this leaf node
    OCTREENODE_INSERTPOINT[ptMode] ( points, pntId, newPnt );
    this->PointIdSet = vtkIdList::New();
    this->PointIdSet->Allocate( ( maxPts >> 2 ), ( maxPts >> 1 ) );
    this->PointIdSet->InsertNextId( *pntId );
    this->UpdateCounterAndDataBoundsRecursively( newPnt, 1, 1, NULL );
    } 
    
  return 1;
}

//----------------------------------------------------------------------------
double vtkIncrementalOctreeNode::GetDistance2ToBoundary
  ( const double point[3], double closest[3], int innerOnly, 
    vtkIncrementalOctreeNode* rootNode, int checkData )
{
  // It is mandatory that GetMinDataBounds() and GetMaxDataBounds() be used. 
  // Direct access to MinDataBounds and MaxDataBounds might incur problems.
  double * thisMin = NULL;
  double * thisMax = NULL;
  double * rootMin = NULL;
  double * rootMax = NULL;
  double   minDist = VTK_DOUBLE_MAX; // minimum distance to the boundaries
  if ( checkData )
    {
    thisMin = this->GetMinDataBounds();
    thisMax = this->GetMaxDataBounds();
    rootMin = rootNode->GetMinDataBounds();
    rootMax = rootNode->GetMaxDataBounds();
    }
  else
    {
    thisMin = this->MinBounds;
    thisMax = this->MaxBounds;
    rootMin = rootNode->GetMinBounds();
    rootMax = rootNode->GetMaxBounds();
    }
  
  int      minFace = 0;  // index of the face with min distance to the point
  int      beXless = int( point[0] < thisMin[0] );
  int      beXmore = int( point[0] > thisMax[0] );
  int      beYless = int( point[1] < thisMin[1] );
  int      beYmore = int( point[1] > thisMax[1] );
  int      beZless = int( point[2] < thisMin[2] );
  int      beZmore = int( point[2] > thisMax[2] );
  int      withinX = int(  ( !beXless )  &&  ( !beXmore )  );
  int      withinY = int(  ( !beYless )  &&  ( !beYmore )  );
  int      withinZ = int(  ( !beZless )  &&  ( !beZmore )  );
  int      xyzFlag = ( withinZ << 2 ) + ( withinY << 1 ) + withinX;
  
  switch ( xyzFlag )
    {
    case 0: // withinZ = 0; withinY = 0;  withinX = 0
            // closest to a corner
    
      closest[0] = ( beXless ? thisMin[0] : thisMax[0] );
      closest[1] = ( beYless ? thisMin[1] : thisMax[1] );
      closest[2] = ( beZless ? thisMin[2] : thisMax[2] );
      minDist    = vtkMath::Distance2BetweenPoints( point, closest );
      break;
      
    case 1: // withinZ = 0; withinY = 0; withinX = 1
            // closest to an x-aligned edge
    
      closest[0] = point[0];  
      closest[1] = ( beYless ? thisMin[1] : thisMax[1] );
      closest[2] = ( beZless ? thisMin[2] : thisMax[2] );
      minDist    = vtkMath::Distance2BetweenPoints( point, closest );
      break;
      
    case 2: // withinZ = 0; withinY = 1; withinX = 0
            // closest to a y-aligned edge
    
      closest[0] = ( beXless ? thisMin[0] : thisMax[0] );
      closest[1] = point[1];
      closest[2] = ( beZless ? thisMin[2] : thisMax[2] );
      minDist    = vtkMath::Distance2BetweenPoints( point, closest );
      break;
      
    case 3: // withinZ = 0; withinY = 1; withinX = 1
            // closest to a z-face
      
      if ( beZless )
        {
        minDist    = thisMin[2] - point[2];
        closest[2] = thisMin[2];
        }
      else
        {
        minDist    = point[2] - thisMax[2];
        closest[2] = thisMax[2];
        }
     
      minDist   *= minDist;
      closest[0] = point[0]; 
      closest[1] = point[1];
      break;
      
    case 4: // withinZ = 1; withinY = 0; withinX = 0
            // cloest to a z-aligned edge
    
      closest[0] = ( beXless ? thisMin[0] : thisMax[0] );
      closest[1] = ( beYless ? thisMin[1] : thisMax[1] );
      closest[2] = point[2]; 
      minDist    = vtkMath::Distance2BetweenPoints( point, closest );
      break;
      
    case 5: // withinZ = 1; withinY = 0; withinX = 1
            // closest to a y-face

      if ( beYless )
        {
        minDist    = thisMin[1] - point[1];
        closest[1] = thisMin[1];
        }
      else
        {
        minDist    = point[1] - thisMax[1];
        closest[1] = thisMax[1];
        }
     
      minDist   *= minDist;
      closest[0] = point[0]; 
      closest[2] = point[2];
      break;
      
    case 6: // withinZ = 1; withinY = 1; withinX = 0
            // closest to an x-face

      if ( beXless )
        {
        minDist    = thisMin[0] - point[0];
        closest[0] = thisMin[0];
        }
      else
        {
        minDist    = point[0] - thisMax[0];
        closest[0] = thisMax[0];
        }
    
      minDist   *= minDist;
      closest[1] = point[1]; 
      closest[2] = point[2];
      break;
      
    case 7: // withinZ = 1; withinY = 1;  withinZ = 1
            // point is inside the box
    
      if ( innerOnly ) // check only inner boundaries
        {
        double faceDst;
        
        faceDst   = point[0] - thisMin[0]; // x-min face
        if ( thisMin[0] != rootMin[0] && faceDst < minDist ) 
          {
          minFace = 0;
          minDist = faceDst;
          }
          
        faceDst   = thisMax[0] - point[0]; // x-max face
        if ( thisMax[0] != rootMax[0] && faceDst < minDist ) 
          {
          minFace = 1;
          minDist = faceDst;
          }
          
        faceDst   = point[1] - thisMin[1]; // y-min face
        if ( thisMin[1] != rootMin[1] && faceDst < minDist ) 
          {
          minFace = 2;
          minDist = faceDst;
          }
          
        faceDst   = thisMax[1] - point[1]; // y-max face
        if ( thisMax[1] != rootMax[1] && faceDst < minDist ) 
          {
          minFace = 3;
          minDist = faceDst;
          }
          
        faceDst   = point[2] - thisMin[2]; // z-min face
        if ( thisMin[2] != rootMin[2] && faceDst < minDist ) 
          {
          minFace = 4;
          minDist = faceDst;
          }
        
        faceDst   = thisMax[2] - point[2]; // z-max face
        if ( thisMax[2] != rootMax[2] && faceDst < minDist ) 
          {
          minFace = 5;
          minDist = faceDst;
          }
        }
      else             // check all boundaries
        {
        double tmpDist[6];
        tmpDist[0] = point[0] - thisMin[0];
        tmpDist[1] = thisMax[0] - point[0];
        tmpDist[2] = point[1] - thisMin[1];
        tmpDist[3] = thisMax[1] - point[1];
        tmpDist[4] = point[2] - thisMin[2];
        tmpDist[5] = thisMax[2] - point[2];
      
        for ( int i = 0; i < 6; i ++ )
          {
          if ( tmpDist[i] < minDist )
            {
            minFace = i;
            minDist = tmpDist[i];
            }
          }
        }
      
      // no square operation if no any inner boundary
      if ( minDist != VTK_DOUBLE_MAX )
        {
        minDist *= minDist;
        }

      closest[0] = point[0]; 
      closest[1] = point[1]; 
      closest[2] = point[2];
      
      // minFace: the quad with the min distance to the point
      // 0: x-min face  ===>  xyzIndx = 0:  x  and  minFace & 1 = 0:  thisMin 
      // 1: x-max face  ===>  xyzIndx = 0:  x  and  minFace & 1 = 1:  thisMax
      // 2: y-min face  ===>  xyzIndx = 1:  y  and  minFace & 1 = 0:  thisMin
      // 3: y-max face  ===>  xyzIndx = 1:  y  and  minFace & 1 = 1:  thisMax
      // 4: z-min face  ===>  xyzIndx = 2:  z  and  minFace & 1 = 0:  thisMin
      // 5: z-max face  ===>  xyzIndx = 2:  z  and  minFace & 1 = 1:  thisMax
      double * pMinMax[2] = { thisMin, thisMax };
      int      xyzIndx    = ( minFace >> 1 );
      closest[ xyzIndx ]  = pMinMax[ minFace & 1 ][ xyzIndx ];
      pMinMax[0] = pMinMax[1] = NULL;

      break;
    }

  thisMin = NULL;
  thisMax = NULL;
  rootMin = NULL;
  rootMax = NULL;
  
  return minDist;
}

//----------------------------------------------------------------------------
double vtkIncrementalOctreeNode::GetDistance2ToBoundary
  ( const double point[3], vtkIncrementalOctreeNode * rootNode, int checkData )
{
  double  dumbPnt[3];
  return
    ( checkData == 1 && this->GetNumberOfPoints() == 0 )
  ? VTK_DOUBLE_MAX
  : this->GetDistance2ToBoundary( point, dumbPnt, 0, rootNode, checkData );
}

//----------------------------------------------------------------------------
double vtkIncrementalOctreeNode::GetDistance2ToBoundary( const double point[3],
  double closest[3], vtkIncrementalOctreeNode * rootNode, int checkData )
{ 
  return
    ( checkData == 1 && this->GetNumberOfPoints() == 0 )
  ? VTK_DOUBLE_MAX
  : this->GetDistance2ToBoundary( point, closest, 0, rootNode, checkData );
}

//----------------------------------------------------------------------------
double vtkIncrementalOctreeNode::GetDistance2ToInnerBoundary
  ( const double point[3], vtkIncrementalOctreeNode * rootNode )
{
  double dumbPnt[3];
  return this->GetDistance2ToBoundary( point, dumbPnt, 1, rootNode, 0 );
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreeNode::ExportAllPointIdsByInsertion( vtkIdList * idList )
{
  if ( this->Children == NULL )
    {
    for ( vtkIdType localId = 0; localId < this->NumberOfPoints; localId ++ )
      {
      idList->InsertNextId(  this->PointIdSet->GetId( localId )  );
      }
    }
  else
    {
    for ( int i = 0; i < 8; i ++ )
      {
      this->Children[i]->ExportAllPointIdsByInsertion( idList );
      }
    }
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreeNode::ExportAllPointIdsByDirectSet
  ( vtkIdType * pntIdx, vtkIdList * idList )
{
  if ( this->Children == NULL )
    {
    for ( vtkIdType localId = 0; localId < this->NumberOfPoints; localId ++ )
      {
      idList->SetId(  ( *pntIdx ),  this->PointIdSet->GetId( localId )  );
      ( *pntIdx ) ++;
      }
    }
  else
    {
    for ( int i = 0; i < 8; i ++ )
      {
      this->Children[i]->ExportAllPointIdsByDirectSet( pntIdx, idList );
      }
    }
}

//----------------------------------------------------------------------------
void vtkIncrementalOctreeNode::PrintSelf( ostream & os, vtkIndent indent )
{ 
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Parent: "         << this->Parent         << endl;
  os << indent << "Children: "       << this->Children       << endl;
  os << indent << "PointIdSet: "     << this->PointIdSet     << endl;
  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;
  os << indent << "MinBounds: "      << this->MinBounds[0]
     << " "    << this->MinBounds[1]
     << " "    << this->MinBounds[2] << endl;
  os << indent << "MaxBounds: "      << this->MaxBounds[0]
     << " "    << this->MaxBounds[1]
     << " "    << this->MaxBounds[2] << endl;
  os << indent << "MinDataBounds: "  << this->MinDataBounds[0] 
     << " "    << this->MinDataBounds[1]
     << " "    << this->MinDataBounds[2] << endl;
  os << indent << "MaxDataBounds: "  << this->MaxDataBounds[0]
     << " "    << this->MaxDataBounds[1]
     << " "    << this->MaxDataBounds[2] << endl;
}
