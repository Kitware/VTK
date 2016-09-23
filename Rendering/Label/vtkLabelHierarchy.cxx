/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelHierarchy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkLabelHierarchy.h"

#include "vtkCamera.h"
#include "vtkCellType.h"
#include "vtkCoordinate.h"
#include "vtkDataArray.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkIdTypeArray.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkLabelHierarchyIterator.h"
#include "vtkLabelHierarchyPrivate.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkCoincidentPoints.h"
#include "vtkPlanes.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPythagoreanQuadruples.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"

#include <octree/octree>
#include <deque>
#include <set>
#include <vector>
#include <map>

#include <cstdlib>

// WORKAROUND:
//
// This is the "comment near declaration of Current"
//
// Workaround for the lack of proper assignment of multiset-derived objects
// in the Borland 5.5 compiler's STL Implementation. This must be set prior
// to any possible calls to the PriorityComparator's default constructor so
// that it can have access to a vtkLabelHierarchy object in order to do proper
// comparisons. (Even though assignments are done after the default construction,
// with the Borland 5.5 multiset, the comparators for the multiset do not get
// set properly during those assignments.) So... we use this ha-ha-ha ckish
// workaround to get it to work.
//
// In practical terms, this means if a method in this file calls add_children,
// it should set vtkLabelHierarchy::Implementation::Current to a non-NULL
// vtkLabelHierarchy prior to calling add_children.
//
// Be warned: there is some global/static state here that may bite somebody
// in the future. But, for now, while we are still supporting Borland 5.5
// in the VTK code base, this is one way to do it. Feel free to change it
// if you have a better solution. But make sure it works on Borland 5.5...
//
vtkLabelHierarchy* vtkLabelHierarchy::Implementation::Current;

//----------------------------------------------------------------------------
// vtkLabelHierarchyFrustumIterator - an iterator with no-initial processing
//
// An iterator that has no initial processing, but looks for possible
// ocrtree nodes based on permutations of pythagorean triples.

class vtkLabelHierarchyFrustumIterator : public vtkLabelHierarchyIterator
{
public:
  vtkTypeMacro(vtkLabelHierarchyFrustumIterator,vtkLabelHierarchyIterator);
  static vtkLabelHierarchyFrustumIterator* New();
  void Prepare( vtkLabelHierarchy* hier, vtkCamera* cam, double frustumPlanes[24] );
  void Begin( vtkIdTypeArray* lastPlaced ) VTK_OVERRIDE;
  virtual void BeginOctreeTraversal();
  void Next() VTK_OVERRIDE;
  bool IsAtEnd() VTK_OVERRIDE;
  vtkIdType GetLabelId() VTK_OVERRIDE;
  void GetNodeGeometry( double center[3], double& sz ) VTK_OVERRIDE;
protected:
  vtkLabelHierarchyFrustumIterator();
  ~vtkLabelHierarchyFrustumIterator() VTK_OVERRIDE;

  bool IsCursorInFrustum();
  virtual void SetCamera( vtkCamera* camera );

  vtkCoordinate* Projector;
  double* Frustum;
  vtkCamera* Camera;
  int Level;
  int NodeCount;
  int HitCount;
  int QuadrupleId;
  int SignFlip;
  int Permutation;
  int Work;
  int IjkG[3];
  int Ijk0[3];
  int IjkS[3];
  int IjkP[3];
  int Ijk[3];
  vtkLabelHierarchy::Implementation::LabelSet::iterator LabelIterator;
  vtkLabelHierarchy::Implementation::HierarchyCursor3 Cursor;
  std::vector<int> Path;
  int AtEnd;
  vtkSmartPointer<vtkIdTypeArray> PreviousLabels;
  vtkIdType PreviousLabelIter;
};

vtkStandardNewMacro(vtkLabelHierarchyFrustumIterator);
vtkCxxSetObjectMacro(vtkLabelHierarchyFrustumIterator, Camera, vtkCamera);
vtkLabelHierarchyFrustumIterator::vtkLabelHierarchyFrustumIterator()
{
  this->Projector = vtkCoordinate::New();
  this->Projector->SetCoordinateSystemToWorld();
  this->Camera = 0;
  this->Level = 0;
  this->QuadrupleId = 0;
  this->Permutation = 0;
  this->Work = 0;
}

vtkLabelHierarchyFrustumIterator::~vtkLabelHierarchyFrustumIterator()
{
  this->Projector->Delete();
  if ( this->Camera )
    this->Camera->Delete();
}

void vtkLabelHierarchyFrustumIterator::Prepare(
  vtkLabelHierarchy* hier, vtkCamera* cam, double frustumPlanes[24] )
{
  this->SetHierarchy( hier );
  this->SetCamera( cam );
  this->Frustum = frustumPlanes;
  this->Level = -1;
  this->SignFlip = 8;
  this->Permutation = 6;
  this->QuadrupleId = vtkMaxPythagoreanQuadrupleId;
  this->Work = 0;
}

void vtkLabelHierarchyFrustumIterator::Begin( vtkIdTypeArray* lastPlaced )
{
  // First, we'll iterate over labels we placed last frame.
  this->PreviousLabels = lastPlaced;
  this->PreviousLabelIter = 0;
  this->AtEnd = -1;
  if ( ! this->PreviousLabels->GetNumberOfTuples() )
  {
    // No previously placed labels? Look in octree:
    this->BeginOctreeTraversal();
  }
}

void vtkLabelHierarchyFrustumIterator::BeginOctreeTraversal()
{
  // Inject new labels from the hierarchy, starting with
  // the highest priority labels nearest the camera
  this->AtEnd = 0;
  this->Cursor = vtkLabelHierarchy::Implementation::HierarchyCursor3(
    this->Hierarchy->Impl->Hierarchy3 );
  this->LabelIterator = this->Cursor->value().end(); // Force the label iterator test in Next() to fail.
  this->Level = -1; // when we increment the level we'll be at the beginning
  this->SignFlip = 8; // force the sign flip to get bypassed
  this->Permutation = 6; // force the permutation to get bypassed
  this->QuadrupleId = vtkMaxPythagoreanQuadrupleId; // when we increment the index into the quadruples, we'll be at the beginning
  this->Work = 0;
  this->NodeCount = 0;
  this->HitCount = 0;

  this->Next();
}

void vtkLabelHierarchyFrustumIterator::Next()
{
  if ( this->AtEnd < 0 )
  {
    vtkDebugMacro( "In strange next. Have previous labels" );
    ++ this->PreviousLabelIter;
    if ( this->PreviousLabelIter < this->PreviousLabels->GetNumberOfTuples() )
    {
      return;
    }
    this->BeginOctreeTraversal(); // sets this->AtEnd = 0;
  }
  else if ( this->AtEnd == 0 )
  {
    // Invalid LabelIterator occurs only when called from BeginOctreeTraversal():
    if ( this->LabelIterator != this->Cursor->value().end() )
    {
      ++ this->LabelIterator;
      if ( this->LabelIterator != this->Cursor->value().end() )
      { // Still have anchors left at this node...
        return;
      }
    }
    vtkDebugMacro( "In next. Level: " << this->Level << " SgnFlp: " << this->SignFlip << " Perm: " << this->Permutation << " QuadId: " << this->QuadrupleId );
    // Either starting traversal or out of anchors at current node.
    // Find next valid node at this level or skip to next.
    bool gotNode = false;
    int lvlMax = 1 << this->Level;
    double sz = this->Hierarchy->Impl->Hierarchy3->root()->value().GetSize() / 2.;
    double eye[3];
    //double vaMax = atan( vtkMath::Pi()/2. - 0.2 * vtkMath::RadiansFromDegrees(t his->Camera->GetViewAngle() ) );
    double vaMin = atan( vtkMath::Pi()/2. - 2.0 * vtkMath::RadiansFromDegrees( this->Camera->GetViewAngle() ) );
    this->Camera->GetPosition( eye );
    while ( ! gotNode )
    {
      ++ this->Work;
      // 1. Is there a sign flip of the current quadruple we can do?
      if ( this->SignFlip < 8 )
      {
        bool flippable;
        do
        {
          flippable = true;
          ++ this->SignFlip;
          int flipCoord;
          for ( int i = 0; i < 3; ++ i )
          {
            flipCoord = this->SignFlip & (1 << i);
            if ( ( this->IjkP[i] == 0 ) && flipCoord )
            {
              flippable = false;
              break; // this->SignFlip won't work... try the next one.
            }
            else
            {
              this->IjkS[i] = flipCoord ? - this->IjkP[i] : this->IjkP[i];
            }
          }
        }
        while ( ! flippable && this->SignFlip < 8 );
        gotNode = ( flippable && this->SignFlip < 8 ); // skip down and see if the node exists
      }
      // 2. Is there a permutation of the current quadruple we can do?
      if ( ! gotNode && this->Permutation < 6 )
      {
        bool goodPerm = false;
        while ( ! goodPerm && ( ++ this->Permutation < 6 ) )
        {
          switch ( this->Permutation )
          {
          case 0: // ijk
            this->IjkP[0] = this->Ijk[0]; this->IjkP[1] = this->Ijk[1]; this->IjkP[2] = this->Ijk[2];
            goodPerm = true; // no perm is always a good perm. (This means you Mr. Brady!)
            break;
          case 1: // ikj (swap j,k) but not if j == k
            if ( this->Ijk[1] == this->Ijk[2] )
            {
              goodPerm = false;
            }
            else
            {
              this->IjkP[0] = this->Ijk[0]; this->IjkP[1] = this->Ijk[2]; this->IjkP[2] = this->Ijk[1];
              goodPerm = true;
            }
            break;
          case 2: // jki (rotate ijk to the left once but not if right neighbors are repeats)
            if ( this->Ijk[0] == this->Ijk[1] && this->Ijk[1] == this->Ijk[2] )
            {
              goodPerm = false;
            }
            else
            {
              this->IjkP[0] = this->Ijk[1]; this->IjkP[1] = this->Ijk[2]; this->IjkP[2] = this->Ijk[0];
              goodPerm = true;
            }
            break;
          case 3: // jik (swap i,j) but not if i == j
            if ( this->Ijk[0] == this->Ijk[1] )
            {
              goodPerm = false;
            }
            else
            {
              this->IjkP[0] = this->Ijk[1]; this->IjkP[1] = this->Ijk[0]; this->IjkP[2] = this->Ijk[2];
              goodPerm = true;
            }
            break;
          case 4: // kij (rotate ijk to the right once but not if left neighbors are repeats)
            if ( this->Ijk[0] == this->Ijk[1] && this->Ijk[1] == this->Ijk[2] )
            {
              goodPerm = false;
            }
            else
            {
              this->IjkP[0] = this->Ijk[2]; this->IjkP[1] = this->Ijk[0]; this->IjkP[2] = this->Ijk[1];
              goodPerm = true;
            }
            break;
          case 5: // kji (swap i,k) but not if i == k
            if ( this->Ijk[0] == this->Ijk[2] )
            {
              goodPerm = false;
            }
            else
            {
              this->IjkP[0] = this->Ijk[2]; this->IjkP[1] = this->Ijk[1]; this->IjkP[2] = this->Ijk[0];
              goodPerm = true;
            }
            break;
          }
        }
        if ( goodPerm )
        {
          this->SignFlip = -1;
          continue; // jump above and set this->IjkS
        }
      }
      // 3. Are there more pseudo-Pythagorean quadruples to try?
      if ( ! gotNode && ( this->QuadrupleId < 0 || vtkPythagoreanQuadruples[this->QuadrupleId * 4] >= 0 ) )
      {
        int R2 = vtkPythagoreanQuadruples[(++ this->QuadrupleId) * 4];
        double R = VTK_DOUBLE_MAX;
        if ( R2 >= 0 )
        {
          // Check that r/R is in [tan(theta_h/5),tan(min(2*theta_h,pi/2))[
          // First: Will these nodes be too close to the camera?
          bool tooClose = true; // a large octree node too close to the camera should be ignored.
          while ( tooClose )
          {
              R = sqrt(static_cast<double>(R2));
            if ( R >= sz / lvlMax * vaMin * 0 )
            {
              tooClose = false;
            }
            else
            {
              R2 = vtkPythagoreanQuadruples[(++ this->QuadrupleId) * 4];
              if ( R2 < 0 )
              {
                vtkDebugMacro( "Panic: too far from camera for cached tuples!" );
                tooClose = false; // exit the loop...
              }
            }
          }
          // Second: See if we're too far from the camera
          //if ( R2 >= 0 && ( R < sz / lvlMax * vaMax * 2 ) ) // uncomment this for speed and coherence at the cost of completeness
          if ( R2 >= 0 ) // uncomment this for completeness at the cost of speed and some popping
          {
            // If we're in the habitable zone, set this->Ijk and reset SignFlip and Permutation...
            for ( int i = 0; i < 3; ++ i )
              this->Ijk[i] = vtkPythagoreanQuadruples[this->QuadrupleId * 4 + i + 1];
            this->SignFlip = 8;
            this->Permutation = -1;
            continue; // jump above and set this->IjkP
          }
          else
          {
          // Force the radius to be -1 and continue on to step 4.
          this->QuadrupleId = vtkMaxPythagoreanQuadrupleId;
          }
        }
      }
      // 4. Can we descend a level in the hierarchy?
      if ( ! gotNode )
      {
        if ( ++ this->Level < static_cast<int>( this->Hierarchy->Impl->ActualDepth ) )
        {
          // Figure out the new "center"
          lvlMax = 1 << this->Level;
          this->Hierarchy->GetDiscreteNodeCoordinatesFromWorldPoint( this->Ijk0, eye, this->Level );
          if ( this->Level == 1 )
          {
            vtkDebugMacro( "i: " << this->Ijk0[0] << " j: " << this->Ijk0[1] << " k: " << this->Ijk0[2] << " l: " << this->Level );
          }
          this->QuadrupleId = -1;
          this->SignFlip = 8;
          this->Permutation = 6;
          continue; // find the first quadruple in the "habitable zone"
        }
      }
      if ( gotNode )
      {
        int R2 = 0;
        for ( int i = 0; i < 3; ++ i )
        {
          this->IjkG[i] = this->Ijk0[i] + this->IjkS[i];
          R2 += this->IjkS[i] * this->IjkS[i];
          if ( this->IjkG[i] < 0 || this->IjkG[i] >= lvlMax )
          { // out of bounds
            gotNode = false;
          }
        }
        if ( this->Debug )
        {
          if (! this->Level )
          {
            if ( ! this->IjkG[0] && ! this->IjkG[1] && ! this->IjkG[2] )
            {
              vtkDebugMacro( "Camera:  i: " << this->Ijk0[0] << " j: " << this->Ijk0[1] << " k: " << this->Ijk0[2] );
              vtkDebugMacro( "SgnPrm:  i: " << this->IjkS[0] << " j: " << this->IjkS[1] << " k: " << this->IjkS[2] );
            }
          }
        }
        if ( gotNode )
        {
          this->NodeCount++;
          // OK, we have nodal coordinates... see if the node exists in the hierarchy
          // First, translate nodal coordinates into a "path" down the tree
          if ( this->Level )
          {
            this->Path.resize( this->Level );
            this->Hierarchy->GetPathForNodalCoordinates( &this->Path[0], this->IjkG, this->Level );
          }
          else
          {
            this->Path.clear();
          }
          // Now see if we can visit it
          if ( this->Cursor.visit( this->Path ) )
          {
            if (this->Debug)
            {
              vtkDebugMacro( "l: " << this->Level << " i: " << this->IjkG[0] << " j: " << this->IjkG[1] << " k: " << this->IjkG[2] << " (");
              for ( std::vector<int>::iterator cit = this->Cursor._M_indices.begin(); cit != this->Cursor._M_indices.end(); ++ cit )
              {
                vtkDebugMacro( " " << *cit );
              }
              vtkDebugMacro( ", " << this->Cursor->value().GetLocalAnchorCount() << ")" );
            }
            this->BoxNode();
            if ( this->Cursor->value().GetLocalAnchorCount() )
            {
              this->HitCount++;
              this->LabelIterator = this->Cursor->value().begin();
              vtkDebugMacro( "        *Level: " << this->Level << " SgnFlp: " << this->SignFlip << " Perm: " << this->Permutation << " QuadId: " << this->QuadrupleId );
              return;
            }
          }
        }
        gotNode = false; // no node at this place in hierarchy... move to next.
      }
      else
      {
        // At end of iteration
        vtkDebugMacro( "I did all I could!" );
        vtkDebugMacro( "Nodes attempted: " << this->NodeCount );
        vtkDebugMacro( "Hits: " << this->HitCount );
        this->AtEnd = 1;
        return;
      }
    }
  }
}

bool vtkLabelHierarchyFrustumIterator::IsAtEnd()
{
  return this->AtEnd > 0;
}

vtkIdType vtkLabelHierarchyFrustumIterator::GetLabelId()
{
  if ( this->AtEnd < 0 )
  {
    return this->PreviousLabels->GetValue( this->PreviousLabelIter );
  }
  return *this->LabelIterator;
}

void vtkLabelHierarchyFrustumIterator::GetNodeGeometry( double center[3], double& sz )
{
  const double* x = this->Cursor->value().GetCenter();
  for ( int i = 0; i < 3; ++ i )
  {
    center[i] = x[i];
  }
  sz = this->Cursor->value().GetSize();
}

bool vtkLabelHierarchyFrustumIterator::IsCursorInFrustum()
{
  return true;
}

//----------------------------------------------------------------------------
// vtkLabelHierarchyFullSortIterator - a simple up-front-sorting iterator
//
// An iterator that first sorts the octree nodes based on level and
// distance to the camera.

class vtkLabelHierarchyFullSortIterator : public vtkLabelHierarchyIterator
{
public:
  vtkTypeMacro(vtkLabelHierarchyFullSortIterator,vtkLabelHierarchyIterator);
  static vtkLabelHierarchyFullSortIterator* New();

  void Prepare( vtkLabelHierarchy* hier, vtkCamera* cam,
      double frustumPlanes[24], bool positionsAsNormals );
  void Begin( vtkIdTypeArray* lastPlaced ) VTK_OVERRIDE;
  void Next() VTK_OVERRIDE;
  bool IsAtEnd() VTK_OVERRIDE;
  vtkIdType GetLabelId() VTK_OVERRIDE;
  void GetNodeGeometry( double center[3], double& sz ) VTK_OVERRIDE;

  // Give internal class access to this protected type.
  typedef vtkLabelHierarchy::Implementation::HierarchyType3::octree_node_pointer NodePointer;

  struct vtkHierarchyNode
  {
    int Level;
    double DistanceToCamera;
    NodePointer Node;
    bool TotallyInside;
  };

  class vtkHierarchyNodeSorter
  {
  public:
    bool operator()(const vtkHierarchyNode & a,
                    const vtkHierarchyNode & b)
    {
      if (a.Level != b.Level)
      {
        return a.Level < b.Level;
      }
      return a.DistanceToCamera < b.DistanceToCamera;
    }
  };

protected:
  vtkLabelHierarchyFullSortIterator();
  ~vtkLabelHierarchyFullSortIterator() VTK_OVERRIDE;

  std::set<vtkHierarchyNode, vtkHierarchyNodeSorter> NodeSet;
  std::set<vtkHierarchyNode, vtkHierarchyNodeSorter>::iterator NodeIterator;

  virtual void SetCamera(vtkCamera* camera);
  vtkCamera* Camera;
  vtkExtractSelectedFrustum* FrustumExtractor;
  bool PositionsAsNormals;
  vtkLabelHierarchy::Implementation::LabelSet::iterator LabelIterator;
  bool AtStart;
  bool AtEnd;
  int NodesTraversed;
};

vtkStandardNewMacro(vtkLabelHierarchyFullSortIterator);
vtkCxxSetObjectMacro(vtkLabelHierarchyFullSortIterator, Camera, vtkCamera);
void vtkLabelHierarchyFullSortIterator::Prepare( vtkLabelHierarchy* hier, vtkCamera* cam,
  double frustumPlanes[24], bool positionsAsNormals )
{
  this->SetHierarchy( hier );
  this->SetCamera( cam );
  vtkSmartPointer<vtkPlanes> frustum = vtkSmartPointer<vtkPlanes>::New();
  frustum->SetFrustumPlanes( frustumPlanes );
  this->FrustumExtractor->SetFrustum( frustum );
  this->PositionsAsNormals = positionsAsNormals;
}

void vtkLabelHierarchyFullSortIterator::Begin( vtkIdTypeArray* vtkNotUsed(lastPlaced) )
{
  double cameraPos[3];
  this->Camera->GetPosition(cameraPos);

  int maxLevel = 1;
  std::deque<vtkHierarchyNode> s;
  vtkHierarchyNode root;
  root.Level = 0;
  root.Node = this->Hierarchy->Impl->Hierarchy3->root();
  root.DistanceToCamera = vtkMath::Distance2BetweenPoints(cameraPos, root.Node->value().GetCenter());
  root.TotallyInside = false;
  s.push_back(root);
  int numNodes = 0;
  int numLeaf = 0;
  int totalLeafDepth = 0;
  size_t numLabels = 0;
  int maxLabels = 10000;
  while (!s.empty())
  {
    vtkHierarchyNode node = s.front();
    s.pop_front();

    this->NodeSet.insert(node);
    numLabels += node.Node->value().GetLocalAnchorCount();
    if ( numLabels > static_cast<size_t>(maxLabels) )
    {
      break;
    }
    int level = node.Level;
    ++numNodes;
    if ( node.Node->num_children() > 0 )
    {
      ++level;
      if ( level > maxLevel )
      {
        maxLevel = level;
      }
      for ( int c = 0; c < 8; ++c )
      {
        vtkHierarchyNode child;
        child.Node = &( *node.Node )[c];
        child.Level = level;
        child.DistanceToCamera = vtkMath::Distance2BetweenPoints(cameraPos, child.Node->value().GetCenter());

        if ( !node.TotallyInside )
        {
          // First check if the box is on the other side of the world.
          // This is for the 3D world view only.
          if ( this->PositionsAsNormals && vtkMath::Dot( cameraPos, child.Node->value().GetCenter() ) < 0.0 )
          {
            continue;
          }
          // Determine if box is offscreen. If so, skip node and children.
          double nodeSize = node.Node->value().GetSize() / 2.0;
          double bbox[6] = {
            child.Node->value().GetCenter()[0] - nodeSize,
            child.Node->value().GetCenter()[0] + nodeSize,
            child.Node->value().GetCenter()[1] - nodeSize,
            child.Node->value().GetCenter()[1] + nodeSize,
            child.Node->value().GetCenter()[2] - nodeSize,
            child.Node->value().GetCenter()[2] + nodeSize};
          int ret = this->FrustumExtractor->OverallBoundsTest(bbox);
          child.TotallyInside = false;
          if ( ret == 0 )
          {
            // Totally outside, no need to visit this node.
            continue;
          }
          else if ( ret == 2 )
          {
            // Totally inside, no need to check children.
            child.TotallyInside = true;
          }
        }
        else
        {
          child.TotallyInside = true;
        }

        s.push_back(child);
      }
    }
    else
    {
      ++numLeaf;
      totalLeafDepth += level;
    }
  }
  vtkDebugMacro( "max level is " << maxLevel );
  vtkDebugMacro( "num nodes " << numNodes );
  vtkDebugMacro( "avg leaf depth " << static_cast<double>(totalLeafDepth) / numLeaf );

  this->NodesTraversed = 0;
  this->NodeIterator = this->NodeSet.begin();
  this->AtStart = true;
  this->AtEnd = false;
  this->Next();
}

void vtkLabelHierarchyFullSortIterator::Next()
{
  if ( !this->AtStart && this->LabelIterator != this->NodeIterator->Node->value().end() )
  {
    ++ this->LabelIterator;
    if ( this->LabelIterator != this->NodeIterator->Node->value().end() )
    {
      vtkDebugMacro( "Still have anchors at the node" );
      return;
    }
  }

  // Move to next octree node
  if ( !this->AtStart )
  {
    ++ this->NodeIterator;
  }
  else
  {
    this->AtStart = false;
  }
  while ( this->NodeIterator != this->NodeSet.end() )
  {
    this->BoxNode();
    if ( this->NodeIterator->Node->value().GetLocalAnchorCount() > 0 )
    {
      this->LabelIterator = this->NodeIterator->Node->value().begin();
      ++ this->NodesTraversed;
      vtkDebugMacro( "At the beginning of a new node" );
      return;
    }
    ++ this->NodeIterator;
  }

  // Done
  vtkDebugMacro( << this->NodesTraversed << " nodes traversed." );
  this->AtEnd = true;
}

bool vtkLabelHierarchyFullSortIterator::IsAtEnd()
{
  return this->AtEnd;
}

vtkIdType vtkLabelHierarchyFullSortIterator::GetLabelId()
{
  if (!(this->IsAtEnd()))
  {
    return *this->LabelIterator;
  }
  else
    return 0;
}

void vtkLabelHierarchyFullSortIterator::GetNodeGeometry( double center[3], double& sz )
{
  const double* x = this->NodeIterator->Node->value().GetCenter();
  for ( int i = 0; i < 3; ++ i )
  {
    center[i] = x[i];
  }
  sz = this->NodeIterator->Node->value().GetSize() / 2.;
}

vtkLabelHierarchyFullSortIterator::vtkLabelHierarchyFullSortIterator()
{
  this->Camera = 0;
  this->FrustumExtractor = vtkExtractSelectedFrustum::New();
}

vtkLabelHierarchyFullSortIterator::~vtkLabelHierarchyFullSortIterator()
{
  if ( this->Camera )
  {
    this->Camera->Delete();
  }
  if ( this->FrustumExtractor )
  {
    this->FrustumExtractor->Delete();
  }
}

//----------------------------------------------------------------------------
// vtkLabelHierarchyQuadtreeIterator - a simple breadth-first iterator
//
// This iterator maintains a queue of nodes to be visited. When a node is
// popped off the front, any of its children that are in the view frustum
// are sorted by distance to the camera and then pushed onto the back.
// This forces the iterator to perform a breadth-first traversal of nodes
// that are roughly ordered by their distance to the camera.
// Unlike the FULL_SORT iterator, it does not traverse and sort all the nodes
// up front; instead nodes are added as their parents are removed.
//
// The total number of nodes to be processed is limited by the
// MAXIMUM_NODES_TRAVERSED constant.
// The number of nodes processed is roughly proportional to the amount of work
// required to place labels, so this is a good way to maintain interactive
// framerates.
// In the future, it might be useful to weight the number of nodes
// queued by the number of label anchors stored at the node.

class vtkLabelHierarchyQuadtreeIterator : public vtkLabelHierarchyIterator
{
public:
  vtkTypeMacro(vtkLabelHierarchyQuadtreeIterator,vtkLabelHierarchyIterator);
  static vtkLabelHierarchyQuadtreeIterator* New();

  typedef vtkLabelHierarchy::Implementation::HierarchyType2::octree_node_pointer NodePointer;

  void Prepare( vtkLabelHierarchy* hier, vtkCamera* cam, double frustumPlanes[24], vtkRenderer* ren, float bucketSize[2] );
  void Begin( vtkIdTypeArray* lastPlaced ) VTK_OVERRIDE;
  void Next() VTK_OVERRIDE;
  bool IsAtEnd() VTK_OVERRIDE;
  vtkIdType GetLabelId() VTK_OVERRIDE;
  void GetNodeGeometry( double center[3], double& sz ) VTK_OVERRIDE;
  bool IsNodeInFrustum( NodePointer node );
  vtkGetObjectMacro(Camera,vtkCamera);
  vtkGetObjectMacro(Renderer,vtkRenderer);
  enum Constants
  {
    MAXIMUM_NODES_QUEUED = 128  // See notes at QueueChildren() before changing.
  };

protected:
  vtkLabelHierarchyQuadtreeIterator();
  ~vtkLabelHierarchyQuadtreeIterator() VTK_OVERRIDE;

  virtual void SetCamera( vtkCamera* camera );
  virtual void SetRenderer( vtkRenderer* renderer );
  void QueueChildren();

  vtkCamera* Camera;
  vtkRenderer* Renderer;
  vtkExtractSelectedFrustum* FrustumExtractor;
  vtkLabelHierarchy::Implementation::LabelSet::iterator LabelIterator;
  NodePointer Node;
  std::deque<NodePointer> Queue; // Queue of nodes to be traversed.
  float BucketSize[2]; // size of label placer buckets in pixels
  double SizeLimit; // square of smallest allowable distance-normalized octree node size.

  bool AtEnd;
  int NodesQueued;
};

vtkStandardNewMacro(vtkLabelHierarchyQuadtreeIterator);
vtkCxxSetObjectMacro(vtkLabelHierarchyQuadtreeIterator,Camera,vtkCamera);
vtkCxxSetObjectMacro(vtkLabelHierarchyQuadtreeIterator,Renderer,vtkRenderer);

vtkLabelHierarchyQuadtreeIterator::vtkLabelHierarchyQuadtreeIterator()
{
  this->AtEnd = true;
  this->Camera = 0;
  this->Renderer = 0;
  this->FrustumExtractor = vtkExtractSelectedFrustum::New();
  this->SizeLimit = 0.;
  this->NodesQueued = 0;
}

vtkLabelHierarchyQuadtreeIterator::~vtkLabelHierarchyQuadtreeIterator()
{
  this->FrustumExtractor->Delete();
  if ( this->Camera )
  {
    this->Camera->Delete();
  }
  if ( this->Renderer )
  {
    this->Renderer->Delete();
  }
}

void vtkLabelHierarchyQuadtreeIterator::Prepare(
  vtkLabelHierarchy* hier,
  vtkCamera* cam,
  double frustumPlanes[24],
  vtkRenderer* ren,
  float bucketSize[2] )
{
  this->NodesQueued = 0;
  this->SetHierarchy( hier );
  this->SetCamera( cam );
  vtkSmartPointer<vtkPlanes> frustum = vtkSmartPointer<vtkPlanes>::New();
  frustum->SetFrustumPlanes( frustumPlanes );
  this->FrustumExtractor->SetFrustum( frustum );
  for ( int i = 0; i < 2; ++ i )
  {
    this->BucketSize[i] = bucketSize[i];
  }
  this->SetRenderer( ren );
#if 0
  if ( cam->GetParallelProjection() )
  { // Compute threshold for quadtree nodes too small to visit using parallel projection
    //cout << "SizeLimit ParallelProj ps: " << cam->GetParallelScale() << "\n";
    //this->SizeLimit = 0.0001 * cam->GetParallelScale(); // FIXME: Should be set using cam->ParallelScale and pixel size
  }
  else
  { // Compute threshold for quadtree nodes too small to visit using perspective projection
    //double va = vtkMath::RadiansFromDegrees( cam->GetViewAngle() );
    //double tva = 2. * tan( va / 2. );
    double vsr;
    if ( cam->GetUseHorizontalViewAngle() )
    {
      double vs = ren->GetSize()[0];
      vsr = this->BucketSize[0] ? ( vs / this->BucketSize[0] ) : VTK_DOUBLE_MAX;
    }
    else
    {
      double vs = ren->GetSize()[1];
      vsr = this->BucketSize[1] ? ( vs / this->BucketSize[1] ) : VTK_DOUBLE_MAX;
    }
    //double fac = vsr ? ( 0.1 * tva / vsr ) : 0.;
    //cout << "SizeLimit  va: " << va << " tva: " << tva << " vsr: " << vsr << " fac: " << fac << " slim: " << fac * fac << "\n";
    //this->SizeLimit = fac * fac;
  }
#endif
}

void vtkLabelHierarchyQuadtreeIterator::Begin( vtkIdTypeArray* vtkNotUsed(lastPlaced) )
{
  if ( this->Hierarchy->GetImplementation()->Hierarchy2 )
  {
    this->Node = this->Hierarchy->GetImplementation()->Hierarchy2->root();
    if ( this->IsNodeInFrustum( this->Node ) )
    {
      this->QueueChildren();
      this->BoxNode();
      ++ this->NodesQueued;
      this->AtEnd = false;
      this->LabelIterator = this->Node->value().begin();
      if ( this->LabelIterator == this->Node->value().end() )
      {
        this->Next();
      }
    }
    else
    {
      this->AtEnd = true;
    }
  }
  else
  {
    this->AtEnd = true;
  }
}

struct vtkQuadtreeNodeDistCompare
{
  double Eye[3];
  void SetEye( const double* eye )
  {
    for ( int i = 0; i < 3; ++ i )
    {
      this->Eye[i] = eye[i];
    }
  }
  bool operator () (
    const vtkLabelHierarchyQuadtreeIterator::NodePointer& a,
    const vtkLabelHierarchyQuadtreeIterator::NodePointer& b ) const
  {
    const double* xa = a->value().GetCenter();
    const double* xb = b->value().GetCenter();
    double da = 0., db = 0.;
    for ( int i = 0; i < 3; ++ i )
    {
      double va, vb;
      va = Eye[i] - xa[i];
      vb = Eye[i] - xb[i];
      da += va * va;
      db += vb * vb;
    }
    return ( da < db ? true : ( da == db ? ( a < b ? true : false ) : false ) );
  }
};

typedef std::set<vtkLabelHierarchyQuadtreeIterator::NodePointer,vtkQuadtreeNodeDistCompare> vtkQuadtreeOrderedChildren;

void vtkLabelHierarchyQuadtreeIterator::Next()
{
  //const int maxNumChildren = ( 1 << 2 );
  ++ this->LabelIterator;
  if ( this->LabelIterator == this->Node->value().end() )
  {
    this->BoxNode();
    while ( this->Queue.size() )
    {
      this->Node = this->Queue.front();
      this->Queue.pop_front();
      this->QueueChildren();
      this->LabelIterator = this->Node->value().begin();
      if ( this->LabelIterator != this->Node->value().end() )
      {
        // We have some labels, stop looking for more nodes
        return;
      }
    }
    // We must be done traversing the tree.
    this->AtEnd = true;
  }
  else
  {
    /* *
    cout << "Label: " << *this->LabelIterator << "\n";
    * */
  }
}

bool vtkLabelHierarchyQuadtreeIterator::IsAtEnd()
{
  return this->AtEnd;
}

vtkIdType vtkLabelHierarchyQuadtreeIterator::GetLabelId()
{
  if (!(this->IsAtEnd()))
  {
    return *this->LabelIterator;
  }
  else
    return 0;
}

void vtkLabelHierarchyQuadtreeIterator::GetNodeGeometry( double center[3], double& sz )
{
  const double* x = this->Node->value().GetCenter();
  for ( int i = 0; i < 2; ++ i )
    center[i] = x[i];
  center[2] = this->Hierarchy->GetImplementation()->Z2;
  sz = this->Node->value().GetSize() / 2.;
}

bool vtkLabelHierarchyQuadtreeIterator::IsNodeInFrustum( NodePointer node )
{
  double nodeSize = node->value().GetSize() / 2.;
  const double* x = node->value().GetCenter();
  double bbox[6] = {
    x[0] - nodeSize, x[0] + nodeSize,
    x[1] - nodeSize, x[1] + nodeSize,
    x[2], x[2]
  };

  if ( ! this->FrustumExtractor->OverallBoundsTest(bbox) )
  {
    return false;
  }

  // Is the node too small? If so, pretend it's not in the frustum.
  const double* eye = this->Camera->GetPosition();
  double d = 0;
  for ( int i = 0; i < 3; ++ i )
  {
    double dx = ( eye[i] - x[i] );
    d += dx * dx;
  }
  if ( nodeSize * nodeSize < d * this->SizeLimit )
  {
    return false;
  }

  return true;
}

/**\brief Queue octree children for traversal after the current level has been traversed.
  *
  * In order to perform a breadth-first traversal, we must either save state
  * or traverse the octree many times. Since traversal can be hard on the
  * CPU cache, we will save state. That state is a list of octree nodes that
  * are the visible (i.e., in the view frustum) children of nodes in the
  * current level. If the entire octree is in the frustum and all the children
  * of nodes at level M exist, this means the list of children will be
  * (2**D)**(M+1) long.
  * For a quadtree, D = 2.
  *
  * Instead of limiting the Queue size, we limit the total number of nodes queued.
  * Since nodes are popped off the front of the queue as they are pushed onto the
  * back, this is a stricter limit. It is also more closely related to the actual
  * amount of time spent processing labels.
  */
void vtkLabelHierarchyQuadtreeIterator::QueueChildren()
{
  int nc = this->Node->num_children();
  if ( nc <= 0 || this->NodesQueued >= MAXIMUM_NODES_QUEUED )
  {
    return;
  }

  // Sort children of this node by distance to eye ...
  int i;
  vtkQuadtreeNodeDistCompare dcomp;
  dcomp.SetEye( this->Camera->GetPosition() );
  vtkQuadtreeOrderedChildren children( dcomp );
  for ( i = 0; i < nc; ++ i )
  {
    NodePointer child = &((*this->Node)[i]);
    if ( this->IsNodeInFrustum( child ) )
    { // only add visible children
      children.insert( child );
    }
  }
  // ... and add those in the frustum to the back of the queue.
  vtkQuadtreeOrderedChildren::iterator cit;
  for ( cit = children.begin(); cit != children.end() && this->NodesQueued < MAXIMUM_NODES_QUEUED; ++ cit )
  {
    this->Queue.push_back( *cit );
    ++ this->NodesQueued;
  }
}

//----------------------------------------------------------------------------
// vtkLabelHierarchyOctreeQueueIterator - a simple breadth-first iterator
//
// This iterator maintains a queue of nodes to be visited. When a node is
// popped off the front, any of its children that are in the view frustum
// are sorted by distance to the camera and then pushed onto the back.
// This forces the iterator to perform a breadth-first traversal of nodes
// that are roughly ordered by their distance to the camera.
// Unlike the FULL_SORT iterator, it does not traverse and sort all the nodes
// up front; instead nodes are added as their parents are removed.
//
// The total number of nodes to be processed is limited by the
// MAXIMUM_NODES_TRAVERSED constant.
// The number of nodes processed is roughly proportional to the amount of work
// required to place labels, so this is a good way to maintain interactive
// framerates.
// In the future, it might be useful to weight the number of nodes
// queued by the number of label anchors stored at the node.

class vtkLabelHierarchyOctreeQueueIterator : public vtkLabelHierarchyIterator
{
public:
  vtkTypeMacro(vtkLabelHierarchyOctreeQueueIterator,vtkLabelHierarchyIterator);
  static vtkLabelHierarchyOctreeQueueIterator* New();

  typedef vtkLabelHierarchy::Implementation::HierarchyType3::octree_node_pointer NodePointer;

  void Prepare( vtkLabelHierarchy* hier, vtkCamera* cam, double frustumPlanes[24], vtkRenderer* ren, float bucketSize[2] );
  void Begin( vtkIdTypeArray* lastPlaced ) VTK_OVERRIDE;
  void Next() VTK_OVERRIDE;
  bool IsAtEnd() VTK_OVERRIDE;
  vtkIdType GetLabelId() VTK_OVERRIDE;
  void GetNodeGeometry( double center[3], double& sz ) VTK_OVERRIDE;
  bool IsNodeInFrustum( NodePointer node );
  vtkGetObjectMacro(Camera,vtkCamera);
  vtkGetObjectMacro(Renderer,vtkRenderer);
  enum Constants
  {
    MAXIMUM_NODES_QUEUED = 128  // See notes at QueueChildren() before changing.
  };

protected:
  vtkLabelHierarchyOctreeQueueIterator();
  ~vtkLabelHierarchyOctreeQueueIterator() VTK_OVERRIDE;

  virtual void SetCamera( vtkCamera* camera );
  virtual void SetRenderer( vtkRenderer* renderer );
  void QueueChildren();

  vtkCamera* Camera;
  vtkRenderer* Renderer;
  vtkExtractSelectedFrustum* FrustumExtractor;
  vtkLabelHierarchy::Implementation::LabelSet::iterator LabelIterator;
  NodePointer Node;
  std::deque<NodePointer> Queue; // Queue of nodes to be traversed.
  float BucketSize[2]; // size of label placer buckets in pixels
  double SizeLimit; // square of smallest allowable distance-normalized octree node size.
  vtkIdTypeArray* LastPlaced; // Labels placed in the previous frame
  vtkIdType LastPlacedIndex; // Index into LastPlaced for the current frame

  bool AtEnd;
  int NodesQueued;
};

vtkStandardNewMacro(vtkLabelHierarchyOctreeQueueIterator);
vtkCxxSetObjectMacro(vtkLabelHierarchyOctreeQueueIterator,Camera,vtkCamera);
vtkCxxSetObjectMacro(vtkLabelHierarchyOctreeQueueIterator,Renderer,vtkRenderer);

vtkLabelHierarchyOctreeQueueIterator::vtkLabelHierarchyOctreeQueueIterator()
{
  this->AtEnd = true;
  this->Camera = 0;
  this->Renderer = 0;
  this->FrustumExtractor = vtkExtractSelectedFrustum::New();
  this->SizeLimit = 0.;
  this->NodesQueued = 0;
}

vtkLabelHierarchyOctreeQueueIterator::~vtkLabelHierarchyOctreeQueueIterator()
{
  this->FrustumExtractor->Delete();
  if ( this->Camera )
  {
    this->Camera->Delete();
  }
  if ( this->Renderer )
  {
    this->Renderer->Delete();
  }
}

void vtkLabelHierarchyOctreeQueueIterator::Prepare(
  vtkLabelHierarchy* hier,
  vtkCamera* cam,
  double frustumPlanes[24],
  vtkRenderer* ren,
  float bucketSize[2] )
{
  this->NodesQueued = 0;
  this->SetHierarchy( hier );
  this->SetCamera( cam );
  vtkSmartPointer<vtkPlanes> frustum = vtkSmartPointer<vtkPlanes>::New();
  frustum->SetFrustumPlanes( frustumPlanes );
  this->FrustumExtractor->SetFrustum( frustum );
  for ( int i = 0; i < 2; ++ i )
  {
    this->BucketSize[i] = bucketSize[i];
  }
  this->SetRenderer( ren );
#if 0
  if ( cam->GetParallelProjection() )
  { // Compute threshold for quadtree nodes too small to visit using parallel projection
    //cout << "SizeLimit ParallelProj ps: " << cam->GetParallelScale() << "\n";
    //this->SizeLimit = 0.0001 * cam->GetParallelScale(); // FIXME: Should be set using cam->ParallelScale and pixel size
  }
  else
  { // Compute threshold for quadtree nodes too small to visit using perspective projection
    //double va = vtkMath::RadiansFromDegrees( cam->GetViewAngle() );
    //double tva = 2. * tan( va / 2. );
    double vsr;
    if ( cam->GetUseHorizontalViewAngle() )
    {
      double vs = ren->GetSize()[0];
      vsr = this->BucketSize[0] ? ( vs / this->BucketSize[0] ) : VTK_DOUBLE_MAX;
    }
    else
    {
      double vs = ren->GetSize()[1];
      vsr = this->BucketSize[1] ? ( vs / this->BucketSize[1] ) : VTK_DOUBLE_MAX;
    }
    //double fac = vsr ? ( 0.1 * tva / vsr ) : 0.;
    //cout << "SizeLimit  va: " << va << " tva: " << tva << " vsr: " << vsr << " fac: " << fac << " slim: " << fac * fac << "\n";
    //this->SizeLimit = fac * fac;
  }
#endif
}

void vtkLabelHierarchyOctreeQueueIterator::Begin( vtkIdTypeArray* lastPlaced )
{
  this->LastPlaced = lastPlaced;
  this->LastPlacedIndex = ( lastPlaced && lastPlaced->GetNumberOfTuples() > 0 )? 0 : -1; // don't try to traverse what's not there

  // Skip over invalid label indices
  if ( this->LastPlacedIndex >= 0 )
  {
    vtkIdType numLabels = this->Hierarchy->GetPointData()->GetAbstractArray( "Type" )->GetNumberOfTuples();
    while ( this->LastPlacedIndex < this->LastPlaced->GetNumberOfTuples() &&
            this->LastPlaced->GetValue( this->LastPlacedIndex ) >= numLabels )
    {
      ++ this->LastPlacedIndex;
    }
    if ( this->LastPlacedIndex >= this->LastPlaced->GetNumberOfTuples() )
    {
      this->LastPlacedIndex = -1;
    }
  }

  if ( this->Hierarchy->GetImplementation()->Hierarchy3 )
  {
    this->Node = this->Hierarchy->GetImplementation()->Hierarchy3->root();
    if ( this->IsNodeInFrustum( this->Node ) )
    {
      this->QueueChildren();
      this->BoxNode();
      ++ this->NodesQueued;
      this->AtEnd = false;
      this->LabelIterator = this->Node->value().begin();
      if ( this->LabelIterator == this->Node->value().end() )
      {
        this->Next();
      }
    }
    else
    {
      this->AtEnd = true;
    }
  }
  else
  {
    this->AtEnd = true;
  }
}

struct vtkOctreeNodeDistCompare
{
  double Eye[3];
  void SetEye( const double* eye )
  {
    for ( int i = 0; i < 3; ++ i )
    {
      this->Eye[i] = eye[i];
    }
  }
  bool operator () (
    const vtkLabelHierarchyOctreeQueueIterator::NodePointer& a,
    const vtkLabelHierarchyOctreeQueueIterator::NodePointer& b ) const
  {
    const double* xa = a->value().GetCenter();
    const double* xb = b->value().GetCenter();
    double da = 0., db = 0.;
    for ( int i = 0; i < 3; ++ i )
    {
      double va, vb;
      va = Eye[i] - xa[i];
      vb = Eye[i] - xb[i];
      da += va * va;
      db += vb * vb;
    }
    return ( da < db ? true : ( da == db ? ( a < b ? true : false ) : false ) );
  }
};

typedef std::set<vtkLabelHierarchyOctreeQueueIterator::NodePointer,vtkOctreeNodeDistCompare> vtkOctreeOrderedChildren;

void vtkLabelHierarchyOctreeQueueIterator::Next()
{
  if ( this->LastPlacedIndex >= 0 )
  {
    ++ this->LastPlacedIndex;

    // Skip over invalid label indices
    vtkIdType numLabels = this->Hierarchy->GetPointData()->GetAbstractArray( "Type" )->GetNumberOfTuples();
    while ( this->LastPlacedIndex < this->LastPlaced->GetNumberOfTuples() &&
            this->LastPlaced->GetValue( this->LastPlacedIndex ) >= numLabels )
    {
      ++ this->LastPlacedIndex;
    }

    if ( this->LastPlacedIndex < this->LastPlaced->GetNumberOfTuples() )
    {
      return; // Done
    }
    else
    {
      this->LastPlacedIndex = -1;
      if ( this->AtEnd )
      {
        return;
      }
    }
  }

  //const int maxNumChildren = ( 1 << 2 );
  if ( this->LabelIterator != this->Node->value().end() )
  {
    ++ this->LabelIterator;
  }
  if ( this->LabelIterator == this->Node->value().end() )
  {
    this->BoxNode();
    while ( this->Queue.size() )
    {
      this->Node = this->Queue.front();
      this->Queue.pop_front();
      this->QueueChildren();
      this->LabelIterator = this->Node->value().begin();
      if ( this->LabelIterator != this->Node->value().end() )
      {
        // We have some labels, stop looking for more nodes
        return;
      }
    }
    // We must be done traversing the tree.
    this->AtEnd = true;
  }
  else
  {
    /* *
    cout << "Label: " << *this->LabelIterator << "\n";
    * */
  }
}

bool vtkLabelHierarchyOctreeQueueIterator::IsAtEnd()
{
  return this->LastPlacedIndex < 0 && this->AtEnd;
}

vtkIdType vtkLabelHierarchyOctreeQueueIterator::GetLabelId()
{
  vtkIdType myId;
  if ( this->LastPlacedIndex >= 0 )
  {
    myId = this->LastPlaced->GetValue( this->LastPlacedIndex );
  }
  else
  {
    if (!(this->IsAtEnd()))
    {
      myId = *this->LabelIterator;
    }
    else
      myId = 0;
  }
  return myId;
}

void vtkLabelHierarchyOctreeQueueIterator::GetNodeGeometry( double center[3], double& sz )
{
  const double* x = this->Node->value().GetCenter();
  for ( int i = 0; i < 3; ++ i )
    center[i] = x[i];
  sz = this->Node->value().GetSize() / 2.;
}

bool vtkLabelHierarchyOctreeQueueIterator::IsNodeInFrustum( NodePointer node )
{
  double nodeSize = node->value().GetSize() / 2.;
  const double* x = node->value().GetCenter();
  double bbox[6] = {
    x[0] - nodeSize, x[0] + nodeSize,
    x[1] - nodeSize, x[1] + nodeSize,
    x[2] - nodeSize, x[2] + nodeSize
  };

  if ( ! this->FrustumExtractor->OverallBoundsTest(bbox) )
  {
    return false;
  }

  // Is the node too small? If so, pretend it's not in the frustum.
  const double* eye = this->Camera->GetPosition();
  double d = 0;
  for ( int i = 0; i < 3; ++ i )
  {
    double dx = ( eye[i] - x[i] );
    d += dx * dx;
  }
  if ( nodeSize * nodeSize < d * this->SizeLimit )
  {
    return false;
  }

  return true;
}

/**\brief Queue octree children for traversal after the current level has been traversed.
  *
  * In order to perform a breadth-first traversal, we must either save state
  * or traverse the octree many times. Since traversal can be hard on the
  * CPU cache, we will save state. That state is a list of octree nodes that
  * are the visible (i.e., in the view frustum) children of nodes in the
  * current level. If the entire octree is in the frustum and all the children
  * of nodes at level M exist, this means the list of children will be
  * (2**D)**(M+1) long.
  * For an octree, D = 3.
  *
  * Instead of limiting the Queue size, we limit the total number of nodes queued.
  * Since nodes are popped off the front of the queue as they are pushed onto the
  * back, this is a stricter limit. It is also more closely related to the actual
  * amount of time spent processing labels.
  */
void vtkLabelHierarchyOctreeQueueIterator::QueueChildren()
{
  int nc = this->Node->num_children();
  if ( nc <= 0 || this->NodesQueued >= MAXIMUM_NODES_QUEUED )
  {
    return;
  }

  // Sort children of this node by distance to eye ...
  int i;
  vtkOctreeNodeDistCompare dcomp;
  dcomp.SetEye( this->Camera->GetPosition() );
  //cout << "Eye " << dcomp.Eye[0] << ", " << dcomp.Eye[1] << ", " << dcomp.Eye[2] << "\n";
  vtkOctreeOrderedChildren children( dcomp );
  for ( i = 0; i < nc; ++ i )
  {
    NodePointer child = &((*this->Node)[i]);
    if ( this->IsNodeInFrustum( child ) )
    { // only add visible children
      children.insert( child );
    }
  }
  // ... and add those in the frustum to the back of the queue.
  vtkOctreeOrderedChildren::iterator cit;
  for ( cit = children.begin(); cit != children.end() && this->NodesQueued < MAXIMUM_NODES_QUEUED; ++ cit )
  {
#if 0
    const double* xa = (*cit)->value().GetCenter();
    double dst = 0.;
    double dx;
    for ( i = 0; i < 3; ++ i )
    {
      dx = dcomp.Eye[i] - xa[i];
      dst += dx * dx;
    }
    cout << "  " << this->NodesQueued << ": " << dst << " to " << xa[0] << ", " << xa[1] << ", " << xa[2] << "\n";
#endif // 0
    this->Queue.push_back( *cit );
    ++ this->NodesQueued;
  }
}

//----------------------------------------------------------------------------
// vtkLabelHierarchy3DepthFirstIterator - a simple up-front-sorting iterator
//
// An iterator that first sorts the octree nodes based on level and
// distance to the camera.

class vtkLabelHierarchy3DepthFirstIterator : public vtkLabelHierarchyIterator
{
public:
  vtkTypeMacro(vtkLabelHierarchy3DepthFirstIterator,vtkLabelHierarchyIterator);
  static vtkLabelHierarchy3DepthFirstIterator* New();

  void Prepare( vtkLabelHierarchy* hier, vtkCamera* cam, double frustumPlanes[24], vtkRenderer* ren, float bucketSize[2] );
  void Begin( vtkIdTypeArray* lastPlaced ) VTK_OVERRIDE;
  void Next() VTK_OVERRIDE;
  bool IsAtEnd() VTK_OVERRIDE;
  vtkIdType GetLabelId() VTK_OVERRIDE;
  void GetNodeGeometry( double center[3], double& sz ) VTK_OVERRIDE;
  bool IsNodeInFrustum();
  void ReorderChildrenForView( int order[8] );
  vtkGetObjectMacro(Camera,vtkCamera);
  vtkGetObjectMacro(Renderer,vtkRenderer);

protected:
  vtkLabelHierarchy3DepthFirstIterator();
  ~vtkLabelHierarchy3DepthFirstIterator() VTK_OVERRIDE;

  virtual void SetCamera( vtkCamera* camera );
  virtual void SetRenderer( vtkRenderer* renderer );

  vtkCamera* Camera;
  vtkRenderer* Renderer;
  vtkExtractSelectedFrustum* FrustumExtractor;
  vtkLabelHierarchy::Implementation::LabelSet::iterator LabelIterator;
  vtkLabelHierarchy::Implementation::HierarchyCursor3 Cursor;
  std::vector<int> Path;
  std::vector<std::vector<int> > Order; // visibility sorted order of children at each level of the tree.
  float BucketSize[2]; // size of label placer buckets in pixels
  double SizeLimit; // square of smallest allowable distance-normalized octree node size.

  bool AtEnd;
  int NodesTraversed;
  int DidRoot;
};

vtkStandardNewMacro(vtkLabelHierarchy3DepthFirstIterator);
vtkCxxSetObjectMacro(vtkLabelHierarchy3DepthFirstIterator,Camera,vtkCamera);
vtkCxxSetObjectMacro(vtkLabelHierarchy3DepthFirstIterator,Renderer,vtkRenderer);

vtkLabelHierarchy3DepthFirstIterator::vtkLabelHierarchy3DepthFirstIterator()
{
  this->AtEnd = true;
  this->DidRoot = 0;
  this->Camera = 0;
  this->Renderer = 0;
  this->FrustumExtractor = vtkExtractSelectedFrustum::New();
  this->SizeLimit = 0.;
}

vtkLabelHierarchy3DepthFirstIterator::~vtkLabelHierarchy3DepthFirstIterator()
{
  this->FrustumExtractor->Delete();
  if ( this->Camera )
  {
    this->Camera->Delete();
  }
  if ( this->Renderer )
  {
    this->Renderer->Delete();
  }
}

void vtkLabelHierarchy3DepthFirstIterator::Prepare(
  vtkLabelHierarchy* hier,
  vtkCamera* cam,
  double frustumPlanes[24],
  vtkRenderer* ren,
  float bucketSize[2] )
{
  this->SetHierarchy( hier );
  this->SetCamera( cam );
  vtkSmartPointer<vtkPlanes> frustum = vtkSmartPointer<vtkPlanes>::New();
  frustum->SetFrustumPlanes( frustumPlanes );
  this->FrustumExtractor->SetFrustum( frustum );
  for ( int i = 0; i < 2; ++ i )
  {
    this->BucketSize[i] = bucketSize[i];
  }
  this->SetRenderer( ren );
#if 0
  if ( cam->GetParallelProjection() )
  { // Compute threshold for quadtree nodes too small to visit using parallel projection
    //cout << "SizeLimit ParallelProj ps: " << cam->GetParallelScale() << "\n";
    //this->SizeLimit = 0.0001; // FIXME: Should be set using cam->ParallelScale
  }
  else
  { // Compute threshold for quadtree nodes too small to visit using perspective projection
    //double va = vtkMath::RadiansFromDegrees( cam->GetViewAngle() );
    //double tva = 2. * tan( va / 2. );
    double vsr;
    if ( cam->GetUseHorizontalViewAngle() )
    {
      double vs = ren->GetSize()[0];
      vsr = vs / this->BucketSize[0];
    }
    else
    {
      double vs = ren->GetSize()[1];
      vsr = vs / this->BucketSize[1];
    }
    //double fac = 0.1 * tva / vsr;
    //cout << "SizeLimit  va: " << va << " tva: " << tva << " vsr: " << vsr << " fac: " << fac << " slim: " << fac * fac << "\n";
    //this->SizeLimit = fac * fac;
  }
#endif
}

void vtkLabelHierarchy3DepthFirstIterator::Begin( vtkIdTypeArray* vtkNotUsed(lastPlaced) )
{
  this->Path.clear();
  this->Order.clear();
  this->DidRoot = false;
  if ( this->Hierarchy->GetImplementation()->Hierarchy3 )
  {
    this->Cursor = vtkLabelHierarchy::Implementation::HierarchyCursor3( this->Hierarchy->GetImplementation()->Hierarchy3 );
    if ( this->IsNodeInFrustum() )
    {
      this->BoxNode();
      this->AtEnd = false;
      this->LabelIterator = this->Cursor->value().begin();
      if ( this->LabelIterator == this->Cursor->value().end() )
      {
        this->Next();
      }
    }
    else
    {
      this->AtEnd = true;
    }
  }
  else
  {
    this->AtEnd = true;
  }
}

void vtkLabelHierarchy3DepthFirstIterator::Next()
{
  //const int maxNumChildren = ( 1 << 2 );
  ++ this->LabelIterator;
  if ( this->LabelIterator == this->Cursor->value().end() )
  {
    this->BoxNode();
    while ( ! this->Path.empty() || ! this->DidRoot )
    {
      this->DidRoot = true;
      // I. Try children of this node.
      if ( this->Cursor->num_children() )
      {
        std::vector<int> emptyVec;
        this->Order.push_back( emptyVec );
        for ( int i = 0; i < this->Cursor->num_children(); ++ i )
        {
          this->Order.back().push_back( i );
        }
        this->ReorderChildrenForView( &(this->Order.back()[0]) );
        this->Cursor.down( this->Order.back()[0] );
        this->Path.push_back( 0 );
        if ( this->IsNodeInFrustum() )
        {
          this->LabelIterator = this->Cursor->value().begin();
          if ( this->LabelIterator != this->Cursor->value().end() )
          { // We found a non-empty node.
            /* *
            cout << "Path:";
            for ( unsigned p = 0; p < this->Path.size(); ++ p )
              cout << " " << this->Path[p];
            cout << "\n";
            cout << "Label: " << *this->LabelIterator << "\n";
            * */
            return;
          }
        }
      }
      // If the root node has no children, the path might be empty here... check it.
      if ( this->Path.empty() )
      {
        this->AtEnd = true;
        return;
      }
      // II. Try siblings of this node.
      while ( this->Path.back() < static_cast<int>( this->Order.back().size() ) )
      {
        if ( ++ this->Path.back() < static_cast<int>( this->Order.back().size() ) )
        {
          this->Cursor.over( this->Order.back()[this->Path.back()] );
        }
        else
        {
          // III. Move up and over to the sibling of our parent.
          this->Path.pop_back();
          this->Order.pop_back();
          this->Cursor.up();
          if ( this->Path.empty() )
          {
            this->AtEnd = true;
            return;
          }
          continue;
        }
        if ( this->IsNodeInFrustum() )
        {
          this->LabelIterator = this->Cursor->value().begin();
          if ( this->LabelIterator != this->Cursor->value().end() )
          { // We found a non-empty node.
            /* *
            cout << "Path:";
            for ( unsigned p = 0; p < this->Path.size(); ++ p )
              cout << " " << this->Path[p];
            cout << "\n";
            cout << "Label: " << *this->LabelIterator << "\n";
            * */
            return;
          }
        }
      }
    }
    // We are done traversing the tree...
    this->AtEnd = true;
  }
  else
  {
    /* *
    cout << "Label: " << *this->LabelIterator << "\n";
    * */
  }
}

bool vtkLabelHierarchy3DepthFirstIterator::IsAtEnd()
{
  return this->AtEnd;
}

vtkIdType vtkLabelHierarchy3DepthFirstIterator::GetLabelId()
{
  if (!(this->IsAtEnd()))
  {
    return *this->LabelIterator;
  }
  else
    return 0;
}

void vtkLabelHierarchy3DepthFirstIterator::GetNodeGeometry( double center[3], double& sz )
{
  const double* x = this->Cursor->value().GetCenter();
  for ( int i = 0; i < 3; ++ i )
    center[i] = x[i];
  sz = this->Cursor->value().GetSize() / 2.;
}

bool vtkLabelHierarchy3DepthFirstIterator::IsNodeInFrustum()
{
  double nodeSize = this->Cursor->value().GetSize() / 2.;
  const double* x = this->Cursor->value().GetCenter();
  double bbox[6] = {
    x[0] - nodeSize, x[0] + nodeSize,
    x[1] - nodeSize, x[1] + nodeSize,
    x[2] - nodeSize, x[2] + nodeSize
  };

  if ( ! this->FrustumExtractor->OverallBoundsTest(bbox) )
  {
    return false;
  }

  // Is the node too small? If so, pretend it's not in the frustum.
  const double* eye = this->Camera->GetPosition();
  double d = 0;
  for ( int i = 0; i < 3; ++ i )
  {
    double dx = ( eye[i] - x[i] );
    d += dx * dx;
  }
  if ( nodeSize * nodeSize < d * this->SizeLimit )
  {
    return false;
  }

  return true;
}

struct vtkDistNodeStruct
{
  int NodeNum;
  double Distance;
};

extern "C" {
static int vtkCompareDist( const void* va, const void* vb )
{
  const vtkDistNodeStruct* da = static_cast<const vtkDistNodeStruct*>( va );
  const vtkDistNodeStruct* db = static_cast<const vtkDistNodeStruct*>( vb );
  return
    ( da->Distance < db->Distance ? -1 :
      ( da->Distance > db->Distance ? 1 : 0 ) );
}
}

void vtkLabelHierarchy3DepthFirstIterator::ReorderChildrenForView( int* order )
{
  vtkIdType nc = this->Cursor->num_children();
  if ( nc <= 0 )
  {
    return;
  }

  struct vtkDistNodeStruct* nodeDistances = new struct vtkDistNodeStruct[nc];
  const double* eye = this->Camera->GetPosition();
  const double* x;
  for ( vtkIdType i = 0; i < nc; ++ i )
  {
    this->Cursor.down( i );
    x = this->Cursor->value().GetCenter();
    nodeDistances[i].NodeNum = i;
    nodeDistances[i].Distance = 0;
    for ( int j = 0; j < 3; ++ j )
    {
      double dx = eye[j] - x[j];
      nodeDistances[i].Distance += dx * dx;
    }
    this->Cursor.up();
  }
  qsort( nodeDistances, nc, sizeof(vtkDistNodeStruct), vtkCompareDist );
  for ( vtkIdType i = 0; i < nc; ++ i )
  {
    order[i] = nodeDistances[i].NodeNum;
  }
  delete [] nodeDistances;
}

//----------------------------------------------------------------------------
// vtkLabelHierarchy

vtkStandardNewMacro(vtkLabelHierarchy);
vtkCxxSetObjectMacro(vtkLabelHierarchy,Priorities,vtkDataArray);
vtkCxxSetObjectMacro(vtkLabelHierarchy,Labels,vtkAbstractArray);
vtkCxxSetObjectMacro(vtkLabelHierarchy,IconIndices,vtkIntArray);
vtkCxxSetObjectMacro(vtkLabelHierarchy,Orientations,vtkDataArray);
vtkCxxSetObjectMacro(vtkLabelHierarchy,Sizes,vtkDataArray);
vtkCxxSetObjectMacro(vtkLabelHierarchy,BoundedSizes,vtkDataArray);
vtkCxxSetObjectMacro(vtkLabelHierarchy,TextProperty,vtkTextProperty);
vtkLabelHierarchy::vtkLabelHierarchy()
{
  this->Impl = new Implementation();
  this->Impl->Husk = this;
  this->Priorities = 0;
  this->Labels = 0;
  this->IconIndices = 0;
  this->Orientations = 0;
  this->Sizes = 0;
  this->BoundedSizes = 0;
  this->TargetLabelCount = 16;
  this->MaximumDepth = 5;
  this->TextProperty = vtkTextProperty::New();

  this->CenterPts = vtkPoints::New();
  this->CoincidentPoints = vtkCoincidentPoints::New();
}

vtkLabelHierarchy::~vtkLabelHierarchy()
{
  delete this->Impl;
  if ( this->Priorities )
  {
    this->Priorities->Delete();
  }
  if ( this->Labels )
  {
    this->Labels->Delete();
  }
  if ( this->IconIndices )
  {
    this->IconIndices->Delete();
  }
  if ( this->Orientations )
  {
    this->Orientations->Delete();
  }
  if ( this->Sizes )
  {
    this->Sizes->Delete();
  }
  if ( this->BoundedSizes )
  {
    this->BoundedSizes->Delete();
  }
  if ( this->TextProperty )
  {
    this->TextProperty->Delete();
  }

  this->CenterPts->Delete();
  this->CoincidentPoints->Delete();
}

void vtkLabelHierarchy::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "MaximumDepth: " << this->MaximumDepth << "\n";
  os << indent << "TargetLabelCount: " << this->TargetLabelCount << "\n";
  os << indent << "Implementation: " << this->Impl << "\n";
  os << indent << "Hierarchy2: " << this->Impl->Hierarchy2 << "\n";
  os << indent << "Hierarchy3: " << this->Impl->Hierarchy3 << "\n";
  os << indent << "HierarchyTime: " << this->Impl->HierarchyTime << "\n";
  os << indent << "Priorities: " << this->Priorities << "\n";
  os << indent << "Labels: " << this->Labels << "\n";
  os << indent << "IconIndices: " << this->IconIndices << "\n";
  os << indent << "Orientations: " << this->Orientations << "\n";
  os << indent << "Sizes: " << this->Sizes << "\n";
  os << indent << "BoundedSizes: " << this->BoundedSizes << "\n";
  os << indent << "CoincidentPoints: " << this->CoincidentPoints << "\n";
  os << indent << "CenterPts: " << this->CenterPts << "\n";
  os << indent << "TextProperty: " << this->TextProperty << "\n";
}

void vtkLabelHierarchy::SetPoints( vtkPoints* src )
{
  if ( src == this->Points )
  {
    return;
  }

  this->Superclass::SetPoints( src );

  if ( src )
  {
    //this->ComputeHierarchy( this->CoincidentPts, this->CoincidenceMap );
  }
}


#if 0
template< class T >
void vtkLabelHierarchyBuildCoincidenceMap(
  vtkLabelHierarchy::Implementation* impl,
  vtkLabelHierarchy* lh,
  T* hier )
{
  typename T::cursor curs( hier );
  int setCount = 0;
  double scale = curs->value().GetSize() / ( 1 << lh->GetMaximumDepth() );
  //cout << "Scale: " << scale << endl;
  double point[3];
  std::vector<std::pair<double,double> > offsets;

  vtkLabelHierarchy::Implementation::MapCoordIter mapIter = impl->CoordMap.begin();
  for( ; mapIter != impl->CoordMap.end(); ++mapIter )
  {
    if( (*mapIter).second.second.size() > 1 )
    {
      point[0] = (*mapIter).first.coord[0];
      point[1] = (*mapIter).first.coord[1];
      point[2] = (*mapIter).first.coord[2];

      vtkSpiralkVertices( (*mapIter).second.second.size() + 1, offsets );
      std::set<vtkIdType>::iterator setIter = (*mapIter).second.second.begin();
      setCount = 0;
      for( ; setIter != (*mapIter).second.second.end(); ++setIter )
      {
        impl->CoincidenceMap[(*setIter)] =
          lh->GetCenterPts()->InsertNextPoint( point );
        lh->GetPoints()->SetPoint( (*setIter),
          point[0] + offsets[setCount + 1].first * scale,
          point[1] + offsets[setCount + 1].second * scale,
          point[2] );
        //cout << "Point: " << point[0] + offsets[setCount].first*scale << " " <<
        //  point[1] + offsets[setCount].second*scale << endl;
        ++setCount;
      }
    }
  }
  // cleanup CoordMap
}
#endif // 0

// A technique for populating a label hierarchy.
//
// This method requires sorting all labels by priority before inserting them into the hierarchy but
// does fully populate all levels of the hierarchy from the top down.
// The exact procedure involves sorting all labels in descending priority, filling the root of
// the label octree with the highest priority labels, and then inserting the remaining labels
// in the highest possible level of octree which is not already full.
void vtkLabelHierarchy::ComputeHierarchy()
{
  delete this->Impl->Hierarchy3;
  delete this->Impl->Hierarchy2;
  this->Impl->ActualDepth = 0;

  double bounds[6];
  double center[3];
  double maxDim = -1.;
  double delta = 0.; // MSVC brain damage requires this initialization.
  this->Points->GetBounds( bounds );
  for ( int i = 0; i < 3; ++ i )
  {
    center[i] = ( bounds[2 * i] + bounds[2 * i + 1] ) / 2.;
    delta = fabs( bounds[2 * i + 1] - bounds[2 * i] );
    if ( delta > maxDim )
      maxDim = delta;
  }
  //Implementation::PriorityComparator comparator( this );
  //Implementation::LabelSet allAnchors( comparator );
  Implementation::LabelSet allAnchors( this );
  if ( delta == 0. ) // no change in z values
  {
    this->Impl->Hierarchy2 =
      new Implementation::HierarchyType2( center, maxDim, allAnchors /* currently empty */ );
    this->Impl->Hierarchy2->root()->value().SetGeometry( center, maxDim );
    this->Impl->Hierarchy3 = 0;
    this->Impl->Z2 = center[2]; // remember z coordinate for later
  }
  else
  {
    this->Impl->Hierarchy2 = 0;
    this->Impl->Hierarchy3 =
      new Implementation::HierarchyType3( center, maxDim, allAnchors /* currently empty */ );
    this->Impl->Hierarchy3->root()->value().SetGeometry( center, maxDim );
  }

  this->Impl->PrepareSortedAnchors( allAnchors );
  //this->Impl->FillHierarchyRoot( allAnchors );

  double scale = 1.;
  if ( this->Impl->Hierarchy3 )
  {
    for ( Implementation::LabelSet::iterator it = allAnchors.begin(); it != allAnchors.end(); ++ it )
    {
      this->Impl->DropAnchor3( *it ); // Ha!!!
    }
    //vtkLabelHierarchyBuildCoincidenceMap( this->Impl, this, this->Impl->Hierarchy3 );
    vtkLabelHierarchy::Implementation::HierarchyCursor3 curs( this->Impl->Hierarchy3 );
    scale = curs->value().GetSize()/(1 << this->MaximumDepth);
  }
  else if ( this->Impl->Hierarchy2 )
  {
    for ( Implementation::LabelSet::iterator it = allAnchors.begin(); it != allAnchors.end(); ++ it )
    {
      this->Impl->DropAnchor2( *it ); // Ha!!!
    }
    //vtkLabelHierarchyBuildCoincidenceMap( this->Impl, this, this->Impl->Hierarchy2 );
    vtkLabelHierarchy::Implementation::HierarchyCursor2 curs( this->Impl->Hierarchy2 );
    scale = curs->value().GetSize()/(1 << this->MaximumDepth);
  }

  double point[3];
  double spiralPoint[3];
  vtkSmartPointer<vtkPoints> offsets = vtkSmartPointer<vtkPoints>::New();
  int numCoincidentPoints = 0;

  this->CoincidentPoints->RemoveNonCoincidentPoints();
  this->CoincidentPoints->InitTraversal();
  vtkIdList* coincidentPoints = this->CoincidentPoints->GetNextCoincidentPointIds();
  vtkIdType Id = 0;
  while ( coincidentPoints != NULL )
  {
    // Iterate over all coincident point ids and perturb them
    numCoincidentPoints = coincidentPoints->GetNumberOfIds();
    vtkCoincidentPoints::SpiralPoints( numCoincidentPoints + 1, offsets );
    for(int i = 0; i < numCoincidentPoints; ++i)
    {
      Id = coincidentPoints->GetId( i );
      this->Points->GetPoint( Id, point );
      // save center points for drawing spokes.
      /*this->Implementation->CoincidenceMap[i] =
        this->CenterPts->InsertNextPoint(point);*/
      offsets->GetPoint( i + 1, spiralPoint );
      this->Points->SetPoint( Id,
        point[0] + spiralPoint[0] * scale,
        point[1] + spiralPoint[1] * scale,
        point[2] );
    }

    coincidentPoints = this->CoincidentPoints->GetNextCoincidentPointIds();
  }

  this->Impl->HierarchyTime.Modified();
}

// FIXME: Currently this is unused but we might like to collect statistics
//        on the actual distribution of label anchors...
#if 0
void vtkLabelHierarchy::Implementation::ComputeActualDepth()
{
  // Find the number of levels in the hierarchy
  this->ActualDepth = 1;
  std::deque<std::pair<HierarchyType3::octree_node_pointer, size_t> > queue;
  queue.push_front( std::make_pair( this->Hierarchy3->root(), 1 ) );
  int numNodes = 0;
  int numLeaf = 0;
  int totalLeafDepth = 0;
  while (!queue.empty())
  {
    std::pair<HierarchyType3::octree_node_pointer,int> p = queue.front();
    HierarchyType3::octree_node_pointer n = p.first;
    size_t level = p.second;
    ++numNodes;
    queue.pop_front();
    if (n->num_children() > 0)
    {
      ++level;
      if ( level > this->ActualDepth )
      {
        this->ActualDepth = level;
      }
      for (int c = 0; c < 8; ++c)
      {
        queue.push_front(std::make_pair(&(*n)[c], level));
      }
    }
    else
    {
      ++numLeaf;
      totalLeafDepth += level;
    }
  }
  vtkDebugWithObjectMacro( this->Husk, "max level is " << this->ActualDepth );
  vtkDebugWithObjectMacro( this->Husk, "num nodes " << numNodes );
  vtkDebugWithObjectMacro( this->Husk, "avg leaf depth " << static_cast<double>(totalLeafDepth) / numLeaf );
}
#endif // 0

vtkLabelHierarchyIterator* vtkLabelHierarchy::NewIterator(
  int type,
  vtkRenderer* ren,
  vtkCamera* cam,
  double frustumPlanes[24],
  bool positionsAsNormals,
  float bucketSize[2] )
{
  vtkLabelHierarchyIterator* iter = 0;
  if ( this->Impl->Hierarchy3 )
  {
    if ( type == FULL_SORT )
    {
      vtkLabelHierarchyFullSortIterator* fs = vtkLabelHierarchyFullSortIterator::New();
      fs->Prepare( this, cam, frustumPlanes, positionsAsNormals );
      iter = fs;
    }
    else if ( type == QUEUE )
    {
      vtkLabelHierarchyOctreeQueueIterator* f = vtkLabelHierarchyOctreeQueueIterator::New();
      f->Prepare( this, cam, frustumPlanes, ren, bucketSize );
      iter = f;
    }
    else if ( type == DEPTH_FIRST )
    {
      vtkLabelHierarchy3DepthFirstIterator* f = vtkLabelHierarchy3DepthFirstIterator::New();
      f->Prepare( this, cam, frustumPlanes, ren, bucketSize );
      iter = f;
    }
    else
    {
      vtkLabelHierarchyFrustumIterator* f = vtkLabelHierarchyFrustumIterator::New();
      f->Prepare( this, cam, frustumPlanes );
      iter = f;
    }
  }
  else
  {
    vtkLabelHierarchyQuadtreeIterator* q = vtkLabelHierarchyQuadtreeIterator::New();
    q->Prepare( this, cam, frustumPlanes, ren, bucketSize );
    iter = q;
  }
  return iter;
}

void vtkLabelHierarchy::GetDiscreteNodeCoordinatesFromWorldPoint( int ijk[3], double pt[3], int level )
{
  Implementation::HierarchyType3::octree_node_pointer root = this->Impl->Hierarchy3->root();
  double delta;
  const double* rootCenter;
  double sz;
  rootCenter = root->value().GetCenter();
  sz = root->value().GetSize() / 2.;
  int m = 1 << level; // max value for any ijk entry
  for ( int i = 0; i < 3; ++ i )
  {
    // The first expression won't work for level 0 because m/2 rounds to 0.
    if ( level )
      delta = ( pt[i] - rootCenter[i] ) * m / 2. / sz + ( m / 2 - 0.5 );
    else
      delta = ( pt[i] - rootCenter[i] ) * m / 2. / sz;
    ijk[i] = static_cast<int>(delta);
  }
}

bool vtkLabelHierarchy::GetPathForNodalCoordinates( int* path, int ijk[3], int level )
{
  int i;
  int m = 1 << level;
  // Don't take any wooden nickels (ijk out of bounds)
  for ( i = 0; i < 3; ++ i )
    if ( ijk[i] < 0 || ijk[i] >= m )
      return false;

  m >>= 1; // each level's midpoint is at half the total node count along each edge
  // For each level, we examine w
  for ( i = 0; i < level; ++ i, m >>= 1 )
  {
    path[i] = 0;
    for ( int j = 0; j < 3; ++ j )
    {
      if ( ijk[j] >= m )
      {
        path[i] += (1 << j);
        ijk[j] -= m;
      }
    }
  }
  return true;
}

vtkIdType vtkLabelHierarchy::GetNumberOfCells()
{
  return 0;
}

vtkCell* vtkLabelHierarchy::GetCell( vtkIdType )
{
  return 0;
}

void vtkLabelHierarchy::GetCell( vtkIdType, vtkGenericCell* )
{
}

int vtkLabelHierarchy::GetCellType( vtkIdType )
{
  return VTK_VERTEX;
}

void vtkLabelHierarchy::GetCellPoints( vtkIdType, vtkIdList* )
{
}

void vtkLabelHierarchy::GetPointCells( vtkIdType, vtkIdList* )
{
}

vtkIdType vtkLabelHierarchy::FindCell( double*, vtkCell*, vtkIdType, double, int&, double*, double* )
{
  return -1;
}

vtkIdType vtkLabelHierarchy::FindCell( double*, vtkCell*, vtkGenericCell*, vtkIdType, double, int&, double*, double* )
{
  return -1;
}

int vtkLabelHierarchy::GetMaxCellSize()
{
  return 1;
}

void vtkLabelHierarchy::Implementation::BinAnchorsToLevel( int level )
{
  //PriorityComparator comparator( this->Husk );
  //LabelSet emptyNode( comparator );
  //HierarchyCursor3 cursor;
  //HierarchyCursor3 root = HierarchyCursor3( this->Hierarchy3 );
  // See comment near declaration of Current for more info:
  vtkLabelHierarchy::Implementation::Current = this->Husk;

  LabelSet emptyNode( this->Husk );
  HierarchyCursor3 cursor;
  HierarchyCursor3 root = HierarchyCursor3( this->Hierarchy3 );
  const double* ctr = root->value().GetCenter();
  double sz = root->value().GetSize();
  double x[3];
  vtkIdType i;
  vtkIdType npts = this->Husk->GetPoints()->GetNumberOfPoints();
  int m[3]; // "branch selector" for each axis (mX is 0 if rX < 0.5 or 1 otherwise)
  int child; // Always set to m0 + 2 * ( m1 + 2 * m2 ), offset into array of node children
  int j;
  for ( i = 0; i < npts; ++ i )
  {
    // Retrieve the point coordinates and node center
    this->Husk->GetPoints()->GetPoint( i, x );
    for ( j = 0; j < 3; ++ j )
    {
      x[j] = ( x[j] - ctr[j] ) / sz + 0.5;
    }

    // Start descending the tree, creating children as necessary.
    cursor = root;
    double thresh = 1.;
    for ( int curlev = 0; curlev < level; ++ curlev )
    {
      thresh *= 0.5;
      for ( j = 0; j < 3; ++ j )
      {
        if ( x[j] < thresh )
        {
          m[j] = 0;
        }
        else
        {
          m[j] = 1;
          x[j] -= thresh;
        }
      }
      child = m[0] + 2 * ( m[1] + 2 * m[2] );
      if ( cursor->is_leaf_node() )
      {
        cursor->add_children( emptyNode );
        cursor->value().SetChildGeometry( &(*cursor) );
      }
      cursor.down( child );
    }
    cursor->value().insert( i );
  }
}

static size_t compute_number_to_promote( int t, size_t L, int d, size_t max )
{
  int tdl = 1 << ( d * L ); // 2^(dL)
  int tdm = ( 1 << d ) - 1; // 2^d - 1
  double n = static_cast<double>(t)*(tdl-1.)/tdl/static_cast<double>(tdm);
  size_t nr = static_cast<size_t>(floor( n )); // truncate...
  double rem = n - nr;
  if ( rem > 0. )
  {
    if ( vtkMath::Random() <= rem )
      ++ nr; // ... and round up some percentage of the time proportional to the remainder.
  }
  return nr > max ? max : nr;
}

void vtkLabelHierarchy::Implementation::PromoteAnchors()
{
  HierarchyCursor3 cursor;
  HierarchyCursor3 root = HierarchyCursor3( this->Hierarchy3 );
  HierarchyIterator3 it;
  //vtkIdType cnt;
  std::vector<vtkIdType> promotionList;
  // Step 1. Iterate over all leaf nodes. We'll ascend to the root from each leaf, promoting anchors as we go.
  //         Outer loop is O(N) since the number of leaf nodes is proportional to the number of anchors.
  for ( it = this->Hierarchy3->begin( true ); it != this->Hierarchy3->end( true ); ++it )
  {
    vtkDebugWithObjectMacro( this->Husk, "o " << it.level() << "(" << it->value().GetLocalAnchorCount() << ")" );
    cursor = it;
    size_t promotionCount = compute_number_to_promote(
      this->Husk->GetTargetLabelCount(),
      static_cast<size_t>( cursor.level() ),
      3, cursor->value().GetLocalAnchorCount() );
    LabelSet::iterator cit = cursor->value().begin();
    LabelSet::iterator eit;
    // Step 1a. Remove all the label anchors from the leaf that we're going to promote to *all* nodes above.
    //          This is o(TargetLabelCount/(2^d - 1)), which is O(1)
    for ( size_t i = 0; i < promotionCount; ++i )
    {
      if (cit == cursor->value().end())
      {
        vtkErrorWithObjectMacro( this->Husk, "error: dereferencing iterator at end()" );
      }
      promotionList.push_back( *cit );
      vtkDebugWithObjectMacro( this->Husk, "Promoting " << *cit << " ( " << cursor->value().key_comp().Hierarchy->GetPriorities()->GetTuple1( *cit ) << ")" );
      eit = cit;
      ++cit;
      cursor->value().erase( eit );
    }
    // FIXME: If we erase all the entries at this level, check to see if all siblings are empty as well.
    //        If so, delete children of parent node. This is complicated by fact that we must have a valid cursor to climb.
    std::vector<vtkIdType>::size_type start = 0;
    std::vector<vtkIdType>::size_type psize = promotionList.size();
    // Step 1b. While we have anchors left to distribute, climb the tree
    //          This loop is O(log(N)) since the tree is log(N) deep.
    while ( cursor.level() && ( start < psize ) )
    {
      cursor.up();
      // How many of our available anchors do we leave at this tree level?
      if ( cursor.level() )
      {
        promotionCount = compute_number_to_promote(
          this->Husk->GetTargetLabelCount(),
          static_cast<size_t>( cursor.level() ),
          3, psize - start );
      }
      else
      {
        promotionCount = psize - start;
      }
      vtkDebugWithObjectMacro( this->Husk, " " << cursor.level() << "(" << promotionCount << ")" );
      // Insert them. This is O(1) since the list is O(1) in length at maximum
      cursor->value().insert( promotionList.begin() + start, promotionList.begin() + start + promotionCount );
      start += promotionCount;
    }
    promotionList.clear();
    vtkDebugWithObjectMacro( this->Husk, "\n" );
  }
  // Total complexity is O(N*log(N))
}

void vtkLabelHierarchy::Implementation::DemoteAnchors( int level )
{
  (void)level;
}

void vtkLabelHierarchy::Implementation::RecursiveNodeDivide( HierarchyCursor3& cursor )
{
  (void)cursor;
}

void vtkLabelHierarchy::Implementation::RecursiveNodeDivide( HierarchyCursor2& cursor )
{
  (void)cursor;
}

void vtkLabelHierarchy::Implementation::PrepareSortedAnchors( LabelSet& anchors )
{
  anchors.clear();
  vtkIdType npts = this->Husk->GetPoints()->GetNumberOfPoints();
  for ( vtkIdType i = 0; i < npts; ++ i )
  {
    anchors.insert( i );
  }
}

void vtkLabelHierarchy::Implementation::FillHierarchyRoot( LabelSet& anchors )
{
  LabelSet::iterator endRootAnchors;
  if ( static_cast<int>( anchors.size() ) < this->Husk->GetTargetLabelCount() )
  {
    endRootAnchors = anchors.end();
  }
  else
  {
    endRootAnchors = anchors.begin();
    for ( int i = 0; i < this->Husk->GetTargetLabelCount(); ++ i )
    {
      ++ endRootAnchors;
    }
  }
  this->Hierarchy3->root()->value().insert( anchors.begin(), endRootAnchors );
  anchors.erase( anchors.begin(), endRootAnchors );
}

void vtkLabelHierarchy::Implementation::DropAnchor2( vtkIdType anchor )
{
  //HierarchyCursor3 curs( this->Hierarchy3 );
  //PriorityComparator comparator( this->Husk );
  //LabelSet emptyNode( comparator );
  // See comment near declaration of Current for more info:
  vtkLabelHierarchy::Implementation::Current = this->Husk;

  LabelSet emptyNode( this->Husk );
  HierarchyCursor2 curs( this->Hierarchy2 );
  const double* ctr = curs->value().GetCenter();
  double x[3];
  double sz = curs->value().GetSize();
  int m[3]; // "branch selector" for each axis (mX is 0 if rX < 0.5 or 1 otherwise)
  int child; // Always set to m0 + 2 * ( m1 + 2 * m2 ), offset into array of node children
  int j;
  // Retrieve the point coordinates
  this->Husk->GetPoints()->GetPoint( anchor, x );

  //Coord coord( x );
  this->Husk->GetCoincidentPoints()->AddPoint( anchor, x );

  // Convert into "octree" coordinates (x[i] in [0,1[ for easy descent).
  for ( j = 0; j < 2; ++ j )
  {
    x[j] = ( x[j] - ctr[j] ) / sz + 0.5;
  }
  double thresh = 1.;
  while ( static_cast<int>( curs->value().GetLocalAnchorCount() ) >= this->Husk->GetTargetLabelCount() )
  { // descend the tree or make children as required.
    thresh *= 0.5;
    for ( j = 0; j < 2; ++ j )
    {
      if ( x[j] < thresh )
      {
        m[j] = 0;
      }
      else
      {
        m[j] = 1;
        x[j] -= thresh;
      }
    }
    child = m[0] + 2 * m[1];
    if ( curs->is_leaf_node() )
    {
      curs->value().AddChildren( &(*curs), emptyNode );
    }
    curs->value().Increment(); // Increment the count of labels in this portion of the tree.
    curs.down( child );
  }
  curs->value().Insert( anchor );
  if ( curs.level() > this->ActualDepth )
  {
    this->ActualDepth = curs.level();
  }

  this->SmudgeAnchor2( curs, anchor, x );
}

void vtkLabelHierarchy::Implementation::DropAnchor3( vtkIdType anchor )
{
  //HierarchyCursor3 curs( this->Hierarchy3 );
  //PriorityComparator comparator( this->Husk );
  //LabelSet emptyNode( comparator );
  // See comment near declaration of Current for more info:
  vtkLabelHierarchy::Implementation::Current = this->Husk;

  LabelSet emptyNode( this->Husk );
  HierarchyCursor3 curs( this->Hierarchy3 );
  const double* ctr = curs->value().GetCenter();
  double x[3];
  double sz = curs->value().GetSize();
  int m[3]; // "branch selector" for each axis (mX is 0 if rX < 0.5 or 1 otherwise)
  int child; // Always set to m0 + 2 * ( m1 + 2 * m2 ), offset into array of node children
  int j;
  // Retrieve the point coordinates
  this->Husk->GetPoints()->GetPoint( anchor, x );

  //Coord coord(x);
  this->Husk->GetCoincidentPoints()->AddPoint( anchor, x );

  // Convert into "octree" coordinates (x[i] in [0,1[ for easy descent).
  for ( j = 0; j < 3; ++ j )
  {
    x[j] = ( x[j] - ctr[j] ) / sz + 0.5;
  }
  double thresh = 1.;
  while ( static_cast<int>( curs->value().GetLocalAnchorCount() ) >= this->Husk->GetTargetLabelCount() )
  { // descend the tree or make children as required.
    thresh *= 0.5;
    for ( j = 0; j < 3; ++ j )
    {
      if ( x[j] < thresh )
      {
        m[j] = 0;
      }
      else
      {
        m[j] = 1;
        x[j] -= thresh;
      }
    }
    child = m[0] + 2 * ( m[1] + 2 * m[2] );
    if ( curs->is_leaf_node() )
    {
      curs->value().AddChildren( &(*curs), emptyNode );
    }
    curs->value().Increment();
    curs.down( child );
  }
  curs->value().Insert( anchor );
  if ( curs.level() > this->ActualDepth )
  {
    this->ActualDepth = curs.level();
  }

  this->SmudgeAnchor3( curs, anchor, x );
}

// If an anchor is near any octree boundaries, copy it to neighbors at the same level.
// This will create neighbors if necessary.
void vtkLabelHierarchy::Implementation::SmudgeAnchor2( HierarchyCursor2& cursor, vtkIdType anchor, double* x )
{
  (void)cursor;
  (void)anchor;
  (void)x;
}

void vtkLabelHierarchy::Implementation::SmudgeAnchor3( HierarchyCursor3& cursor, vtkIdType anchor, double* x )
{
  (void)cursor;
  (void)anchor;
  (void)x;
}
