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
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkCoincidentPoints.h"
#include "vtkPlanes.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPythagoreanQuadruples.h"
#include "vtkSmartPointer.h"

#include <octree/octree>
#include <vtkstd/deque>
#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtkstd/map>

//----------------------------------------------------------------------------
// vtkLabelHierarchy::implementation

class vtkLabelHierarchy::implementation
{
public:
  implementation()
  {
    this->Hierarchy = 0;
    this->ActualDepth = 5;
  }

  ~implementation()
  {
    if ( this->Hierarchy )
      {
      delete this->Hierarchy;
      }

  }

  bool ComparePriorities( vtkIdType a, vtkIdType b )
  {
    return this->Self->Priorities ?
      this->Self->Priorities->GetTuple1( a ) > this->Self->Priorities->GetTuple1( b ) :
      a < b;
  }

  struct PriorityComparator
  {
    vtkLabelHierarchy* Hierarchy;

    PriorityComparator()
      {
      // See comment near declaration of Current for more info:
      this->Hierarchy = vtkLabelHierarchy::implementation::Current;
      }

    PriorityComparator( vtkLabelHierarchy* h )
      {
      this->Hierarchy = h;
      }

    PriorityComparator( const PriorityComparator& src )
      {
      this->Hierarchy = src.Hierarchy;
      }

    PriorityComparator& operator=(const PriorityComparator& rhs)
      {
      if (this != &rhs)
        {
        this->Hierarchy = rhs.Hierarchy;
        }
      return *this;
      }

    ~PriorityComparator()
      {
      }

    bool operator () ( const vtkIdType& a, const vtkIdType& b )
      {
      if (0 == this->Hierarchy)
        {
        vtkGenericWarningMacro( "error: NULL this->Hierarchy in PriorityComparator" );
        return a < b;
        }

      if (0 == this->Hierarchy->GetImplementation())
        {
        vtkGenericWarningMacro( "error: NULL this->Hierarchy->GetImplementation() in PriorityComparator" );
        return a < b;
        }

      return this->Hierarchy->GetImplementation()->ComparePriorities( a, b );
      }
  };

  class LabelSet : public vtkstd::multiset<vtkIdType,PriorityComparator>
  {
  public: 
    LabelSet( vtkLabelHierarchy* hierarchy )
      : vtkstd::multiset<vtkIdType,PriorityComparator>( PriorityComparator(hierarchy) )
      {
      this->TotalArea = 0;
      }

    LabelSet( const LabelSet& src )
      : vtkstd::multiset<vtkIdType,PriorityComparator>( src )
      {
      this->TotalArea = src.TotalArea;
      }

    LabelSet()
      : vtkstd::multiset<vtkIdType,PriorityComparator>()
      {
      this->TotalArea = 0;
      }

    LabelSet& operator=(const LabelSet& rhs)
      {
      if (this != &rhs)
        {
#if ! ( defined(_MSC_VER) && (_MSC_VER < 1300) )
        vtkstd::multiset<vtkIdType,PriorityComparator>::operator=(rhs);
#endif
        this->TotalArea = rhs.TotalArea;
        }
      return *this;
      }

    double TotalArea;
  };

  typedef octree<LabelSet> HierarchyType;
  typedef octree<LabelSet>::cursor HierarchyCursor;
  typedef octree<LabelSet>::iterator HierarchyIterator;

  // Description:
  // Computes the depth of the generated hierarchy.
  void ComputeActualDepth();

  // Description:
  // Routines called by ComputeHierarchy()
  void BinAnchorsToLevel( int level );
  void PromoteAnchors();
  void DemoteAnchors( int level );
  void RecursiveNodeDivide( HierarchyCursor& cursor );

  // Description:
  // Routines called by ComputeHierarchy()
  void PrepareSortedAnchors( LabelSet& anchors );
  void FillHierarchyRoot( LabelSet& anchors );
  void DropAnchor( vtkIdType anchor );
  void SmudgeAnchor( HierarchyCursor& cursor, vtkIdType anchor, double* x );

  HierarchyType* Hierarchy;
  vtkTimeStamp HierarchyTime;
  int ActualDepth;
  vtkLabelHierarchy* Self;
  static vtkLabelHierarchy* Current;
};


// WORKAROUND:
//
// This is the "comment near declaration of Current"
//
// Workaround for the lack of proper assignment of multiset-derived objects
// in the Borland 5.5 compiler's STL implementation. This must be set prior
// to any possible calls to the PriorityComparator's default constructor so
// that it can have access to a vtkLabelHierarchy object in order to do proper
// comparisons. (Even though assignments are done after the default construction,
// with the Borland 5.5 multiset, the comparators for the multiset do not get
// set properly during those assignments.) So... we use this ha-ha-ha ckish
// workaround to get it to work.
//
// In practical terms, this means if a method in this file calls add_children,
// it should set vtkLabelHierarchy::implementation::Current to a non-NULL
// vtkLabelHierarchy prior to calling add_children.
//
// Be warned: there is some global/static state here that may bite somebody
// in the future. But, for now, while we are still supporting Borland 5.5
// in the VTK code base, this is one way to do it. Feel free to change it
// if you have a better solution. But make sure it works on Borland 5.5...
//
vtkLabelHierarchy* vtkLabelHierarchy::implementation::Current;


//----------------------------------------------------------------------------
// vtkLabelHierarchyFrustumIterator - an iterator with no-initial processing
//
// An iterator that has no initial processing, but looks for possible
// ocrtree nodes based on permutations of pythagorean triples.

class vtkLabelHierarchyFrustumIterator : public vtkLabelHierarchyIterator
{
public:
  vtkTypeRevisionMacro(vtkLabelHierarchyFrustumIterator,vtkLabelHierarchyIterator);
  static vtkLabelHierarchyFrustumIterator* New();
  void Prepare( vtkLabelHierarchy* hier, vtkCamera* cam, double frustumPlanes[24] );
  virtual void SetTraversedBounds( vtkPolyData* pd );
  virtual void EnumerateHierarchy();
  virtual void Begin( vtkIdTypeArray* lastPlaced );
  virtual void BeginOctreeTraversal();
  virtual void Next();
  virtual bool IsAtEnd();
  virtual vtkIdType GetLabelId();
protected:
  vtkLabelHierarchyFrustumIterator();
  virtual ~vtkLabelHierarchyFrustumIterator();

  bool IsCursorInFrustum();
  void BoxCursor();
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
  vtkLabelHierarchy::implementation::LabelSet::iterator LabelIterator;
  vtkLabelHierarchy::implementation::HierarchyCursor Cursor;
  vtkstd::vector<int> Path;
  int AtEnd;
  vtkSmartPointer<vtkIdTypeArray> PreviousLabels;
  vtkIdType PreviousLabelIter;
  vtkPolyData* TraversedBounds;
  double BoundsFactor;
};

vtkCxxRevisionMacro(vtkLabelHierarchyFrustumIterator,"1.25");
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
  this->TraversedBounds = 0;
  this->BoundsFactor = 0.9;
  this->Work = 0;
}

void vtkLabelHierarchyFrustumIterator::SetTraversedBounds( vtkPolyData* pd )
{
  this->TraversedBounds = pd;
}

void vtkLabelHierarchyFrustumIterator::EnumerateHierarchy()
{
  vtkLabelHierarchy::implementation::HierarchyIterator iter;
  vtkLabelHierarchy::implementation::HierarchyCursor oldCurs = this->Cursor;
  for ( iter = this->Hierarchy->Implementation->Hierarchy->begin( true );
        iter != this->Hierarchy->Implementation->Hierarchy->end( true );
        ++ iter )
    {
    this->Cursor = iter;
    this->BoxCursor();
    }
  this->Cursor = oldCurs;
}

void vtkLabelHierarchyFrustumIterator::BoxCursor()
{
  if ( ! this->TraversedBounds )
    return;
  const double* ctr = this->Cursor->center();
  double sz = this->Cursor->size() / 2.;
  vtkPoints* pts = this->TraversedBounds->GetPoints();
  vtkIdType conn[8];
  double tf = this->BoundsFactor;
  conn[0] = pts->InsertNextPoint( ctr[0] - tf * sz, ctr[1] - tf * sz, ctr[2] - tf * sz );
  conn[1] = pts->InsertNextPoint( ctr[0] + tf * sz, ctr[1] - tf * sz, ctr[2] - tf * sz );
  conn[2] = pts->InsertNextPoint( ctr[0] + tf * sz, ctr[1] + tf * sz, ctr[2] - tf * sz );
  conn[3] = pts->InsertNextPoint( ctr[0] - tf * sz, ctr[1] + tf * sz, ctr[2] - tf * sz );
  conn[4] = pts->InsertNextPoint( ctr[0] - tf * sz, ctr[1] - tf * sz, ctr[2] + tf * sz );
  conn[5] = pts->InsertNextPoint( ctr[0] + tf * sz, ctr[1] - tf * sz, ctr[2] + tf * sz );
  conn[6] = pts->InsertNextPoint( ctr[0] + tf * sz, ctr[1] + tf * sz, ctr[2] + tf * sz );
  conn[7] = pts->InsertNextPoint( ctr[0] - tf * sz, ctr[1] + tf * sz, ctr[2] + tf * sz );
  vtkIdType econn[2];
  static const int edgeIds[12][2] =
    {
      {  0,  1 },
      {  1,  2 },
      {  2,  3 },
      {  3,  0 },

      {  4,  5 },
      {  5,  6 },
      {  6,  7 },
      {  7,  4 },

      {  0,  4 },
      {  1,  5 },
      {  2,  6 },
      {  3,  7 },
    };
  for ( int i = 0; i < 12; ++ i )
    {
    econn[0] = conn[edgeIds[i][0]];
    econn[1] = conn[edgeIds[i][1]];
    this->TraversedBounds->InsertNextCell( VTK_LINE, 2, econn );
    }
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
  this->Cursor = vtkLabelHierarchy::implementation::HierarchyCursor(
    this->Hierarchy->Implementation->Hierarchy );
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
    double sz = this->Hierarchy->Implementation->Hierarchy->root()->size() / 2.;
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
        if ( ++ this->Level < this->Hierarchy->Implementation->ActualDepth )
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
            this->Path.empty();
            }
          // Now see if we can visit it
          if ( this->Cursor.visit( this->Path ) )
            {
            if (this->Debug)
              {
              vtkDebugMacro( "l: " << this->Level << " i: " << this->IjkG[0] << " j: " << this->IjkG[1] << " k: " << this->IjkG[2] << " (");
              for ( vtkstd::vector<int>::iterator cit = this->Cursor._M_indices.begin(); cit != this->Cursor._M_indices.end(); ++ cit )
                {
                vtkDebugMacro( " " << *cit );
                }
              vtkDebugMacro( ", " << this->Cursor->value().size() << ")" );
              }
            this->BoxCursor();
            if ( this->Cursor->value().size() )
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
  vtkTypeRevisionMacro(vtkLabelHierarchyFullSortIterator,vtkLabelHierarchyIterator);
  static vtkLabelHierarchyFullSortIterator* New();

  void Prepare( vtkLabelHierarchy* hier, vtkCamera* cam,
      double frustumPlanes[24], bool positionsAsNormals );
  virtual void SetTraversedBounds( vtkPolyData* pd );
  virtual void Begin( vtkIdTypeArray* lastPlaced );
  virtual void Next();
  virtual bool IsAtEnd();
  virtual vtkIdType GetLabelId();
  void BoxNode();

  // Give internal class access to this protected type.
  typedef vtkLabelHierarchy::implementation::HierarchyType::octree_node_pointer NodePointer;

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
  virtual ~vtkLabelHierarchyFullSortIterator();

  vtkstd::set<vtkHierarchyNode, vtkHierarchyNodeSorter> NodeSet;
  vtkstd::set<vtkHierarchyNode, vtkHierarchyNodeSorter>::iterator NodeIterator;

  virtual void SetCamera(vtkCamera* camera);
  vtkCamera* Camera;
  vtkExtractSelectedFrustum* FrustumExtractor;
  bool PositionsAsNormals;
  vtkPolyData* TraversedBounds;
  double BoundsFactor;
  vtkLabelHierarchy::implementation::LabelSet::iterator LabelIterator;
  bool AtStart;
  bool AtEnd;
  int NodesTraversed;
};

vtkCxxRevisionMacro(vtkLabelHierarchyFullSortIterator,"1.25");
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

void vtkLabelHierarchyFullSortIterator::SetTraversedBounds( vtkPolyData* pd )
{
  this->TraversedBounds = pd;
}

void vtkLabelHierarchyFullSortIterator::Begin( vtkIdTypeArray* vtkNotUsed(lastPlaced) )
{
  double cameraPos[3];
  this->Camera->GetPosition(cameraPos);

  int maxLevel = 1;
  vtkstd::deque<vtkHierarchyNode> s;
  vtkHierarchyNode root;
  root.Level = 0;
  root.Node = this->Hierarchy->Implementation->Hierarchy->root();
  root.DistanceToCamera = vtkMath::Distance2BetweenPoints(cameraPos, root.Node->center());
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
    numLabels += node.Node->value().size();
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
        child.DistanceToCamera = vtkMath::Distance2BetweenPoints(cameraPos, child.Node->center());

        if ( !node.TotallyInside )
          {
          // First check if the box is on the other side of the world.
          // This is for the 3D world view only.
          if ( this->PositionsAsNormals && vtkMath::Dot( cameraPos, child.Node->center() ) < 0.0 )
            {
            continue;
            }
          // Determine if box is offscreen. If so, skip node and children.
          double nodeSize = node.Node->size() / 2.0;
          double bbox[6] = {
            child.Node->center()[0] - nodeSize,
            child.Node->center()[0] + nodeSize,
            child.Node->center()[1] - nodeSize,
            child.Node->center()[1] + nodeSize,
            child.Node->center()[2] - nodeSize,
            child.Node->center()[2] + nodeSize};
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
    if ( this->NodeIterator->Node->value().size() > 0 )
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
  return *this->LabelIterator;
}

void vtkLabelHierarchyFullSortIterator::BoxNode()
{
  if ( ! this->TraversedBounds )
    return;
  const double* ctr = this->NodeIterator->Node->center();
  double sz = this->NodeIterator->Node->size() / 2.;
  vtkPoints* pts = this->TraversedBounds->GetPoints();
  vtkIdType conn[8];
  double tf = this->BoundsFactor;
  conn[0] = pts->InsertNextPoint( ctr[0] - tf * sz, ctr[1] - tf * sz, ctr[2] - tf * sz );
  conn[1] = pts->InsertNextPoint( ctr[0] + tf * sz, ctr[1] - tf * sz, ctr[2] - tf * sz );
  conn[2] = pts->InsertNextPoint( ctr[0] + tf * sz, ctr[1] + tf * sz, ctr[2] - tf * sz );
  conn[3] = pts->InsertNextPoint( ctr[0] - tf * sz, ctr[1] + tf * sz, ctr[2] - tf * sz );
  conn[4] = pts->InsertNextPoint( ctr[0] - tf * sz, ctr[1] - tf * sz, ctr[2] + tf * sz );
  conn[5] = pts->InsertNextPoint( ctr[0] + tf * sz, ctr[1] - tf * sz, ctr[2] + tf * sz );
  conn[6] = pts->InsertNextPoint( ctr[0] + tf * sz, ctr[1] + tf * sz, ctr[2] + tf * sz );
  conn[7] = pts->InsertNextPoint( ctr[0] - tf * sz, ctr[1] + tf * sz, ctr[2] + tf * sz );
  vtkIdType econn[2];
  static const int edgeIds[12][2] =
    {
      {  0,  1 },
      {  1,  2 },
      {  2,  3 },
      {  3,  0 },

      {  4,  5 },
      {  5,  6 },
      {  6,  7 },
      {  7,  4 },

      {  0,  4 },
      {  1,  5 },
      {  2,  6 },
      {  3,  7 },
    };
  for ( int i = 0; i < 12; ++ i )
    {
    econn[0] = conn[edgeIds[i][0]];
    econn[1] = conn[edgeIds[i][1]];
    this->TraversedBounds->InsertNextCell( VTK_LINE, 2, econn );
    }
}

vtkLabelHierarchyFullSortIterator::vtkLabelHierarchyFullSortIterator()
{
  this->TraversedBounds = 0;
  this->BoundsFactor = 0.9;
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
// vtkSpiralkVertices - used to calculate points along a spiral using a circle with
// radius = 1.0? centered about the origin.

void vtkSpiralkVertices(vtkIdType num, vtkstd::vector<vtkstd::pair<double,double> >& offsets)
{
  int maxIter = 10;
  double pi = vtkMath::Pi();
  double a = 1/(4*pi*pi);
  offsets.clear();
  for (vtkIdType i = offsets.size(); i < num; i++)
    {
    double d = 2.0*i/sqrt(3.0);
    // We are looking for points at regular intervals along the parametric spiral
    // x = t*cos(2*pi*t)
    // y = t*sin(2*pi*t)
    // We cannot solve this equation exactly, so we use newton's method.
    // Using an Excel trendline, we find that 
    // t = 0.553*d^0.502
    // is an excellent starting point./g 
    double t = 0.553*pow(d, 0.502);
    for (int iter = 0; iter < maxIter; iter++)
      {
      double r = sqrt(t*t+a*a);
      double f = pi*(t*r+a*a*log(t+r)) - d;
      double df = 2*pi*r;
      t = t - f/df;
      }
    double x = t*cos(2*pi*t);
    double y = t*sin(2*pi*t);
    offsets.push_back(vtkstd::pair<double,double>(x, y));
    }
}

//----------------------------------------------------------------------------
// vtkLabelHierarchy

vtkStandardNewMacro(vtkLabelHierarchy);
vtkCxxRevisionMacro(vtkLabelHierarchy,"1.25");
vtkCxxSetObjectMacro(vtkLabelHierarchy,Priorities,vtkDataArray);
vtkLabelHierarchy::vtkLabelHierarchy()
{
  this->Implementation = new implementation();
  this->Implementation->Self = this;
  this->Priorities = 0;
  this->TargetLabelCount = 16;
  this->MaximumDepth = 5;

  this->CenterPts = vtkPoints::New();
  this->CoincidentPoints = vtkCoincidentPoints::New();
}

vtkLabelHierarchy::~vtkLabelHierarchy()
{
  delete this->Implementation;
  if ( this->Priorities )
    {
    this->Priorities->Delete();
    }

  this->CenterPts->Delete();
  this->CoincidentPoints->Delete();
}

void vtkLabelHierarchy::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "MaximumDepth: " << this->MaximumDepth << "\n";
  os << indent << "TargetLabelCount: " << this->TargetLabelCount << "\n";
  os << indent << "Hierarchy: " << this->Implementation->Hierarchy << "\n";
  os << indent << "HierarchyTime: " << this->Implementation->HierarchyTime << "\n";
  os << indent << "Priorities: " << this->Priorities << "\n";
  os << indent << "CoincidentPoints: " << this->CoincidentPoints << "\n";
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

// A technique for populating a label hierarchy.
//
// This method requires sorting all labels by priority before inserting them into the hierarchy but
// does fully populate all levels of the hierarchy from the top down.
// The exact procedure involves sorting all labels in descending priority, filling the root of
// the label octree with the highest priority labels, and then inserting the remaining labels
// in the highest possible level of octree which is not already full.
void vtkLabelHierarchy::ComputeHierarchy()
{
  if ( this->Implementation->Hierarchy )
    {
    delete this->Implementation->Hierarchy;
    }

  double bounds[6];
  double center[3];
  double maxDim = -1.;
  double delta;
  this->Points->GetBounds( bounds );
  for ( int i = 0; i < 3; ++ i )
    {
    center[i] = ( bounds[2 * i] + bounds[2 * i + 1] ) / 2.;
    delta = fabs( bounds[2 * i + 1] - bounds[2 * i] );
    if ( delta > maxDim )
      maxDim = delta;
    }
  implementation::LabelSet allAnchors( this );
  this->Implementation->Hierarchy = 
    new implementation::HierarchyType( center, maxDim, allAnchors /* currently empty */ );

  this->Implementation->PrepareSortedAnchors( allAnchors );
  this->Implementation->FillHierarchyRoot( allAnchors );

  for ( implementation::LabelSet::iterator it = allAnchors.begin(); it != allAnchors.end(); ++ it )
    {
    this->Implementation->DropAnchor( *it ); // Ha!!!
    }

  implementation::HierarchyCursor curs( this->Implementation->Hierarchy );
  double scale = curs->size()/(1 << this->MaximumDepth);

  double point[3];
  vtkstd::vector<vtkstd::pair<double,double> > offsets;
  int numCoincidentPoints = 0;

  this->CoincidentPoints->RemoveNonCoincidentPoints();
  this->CoincidentPoints->InitTraversal();
  vtkIdList * coincidentPoints = this->CoincidentPoints->GetNextCoincidentPointIds();
  vtkIdType Id = 0;
  while(coincidentPoints != NULL)
    {
    // Iterate over all coincident point ids and perturb them
    numCoincidentPoints = coincidentPoints->GetNumberOfIds();
    vtkSpiralkVertices( numCoincidentPoints + 1, offsets );
    for(int i = 0; i < numCoincidentPoints; ++i)
      {
      Id = coincidentPoints->GetId(i);
      this->Points->GetPoint(Id, point);
      // save center points for drawing spokes.
      /*this->Implementation->CoincidenceMap[i] = 
      this->CenterPts->InsertNextPoint(point);*/
      this->Points->SetPoint(Id,
        point[0] + offsets[i + 1].first * scale,
        point[1] + offsets[i + 1].second * scale,
        point[2] );
      }

    coincidentPoints = this->CoincidentPoints->GetNextCoincidentPointIds();
    }

  // cleanup coordMap

  this->Implementation->HierarchyTime.Modified();

  this->Implementation->ComputeActualDepth();
}

void vtkLabelHierarchy::implementation::ComputeActualDepth()
{
  // Find the number of levels in the hierarchy
  this->ActualDepth = 1;
  vtkstd::deque<vtkstd::pair<HierarchyType::octree_node_pointer, int> > queue;
  queue.push_front(vtkstd::make_pair(this->Hierarchy->root(), 1));
  int numNodes = 0;
  int numLeaf = 0;
  int totalLeafDepth = 0;
  while (!queue.empty())
    {
    vtkstd::pair<HierarchyType::octree_node_pointer,int> p = queue.front();
    HierarchyType::octree_node_pointer n = p.first;
    int level = p.second;
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
        queue.push_front(vtkstd::make_pair(&(*n)[c], level));
        }
      }
    else
      {
      ++numLeaf;
      totalLeafDepth += level;
      }
    }
  vtkDebugWithObjectMacro( this->Self, "max level is " << this->ActualDepth );
  vtkDebugWithObjectMacro( this->Self, "num nodes " << numNodes );
  vtkDebugWithObjectMacro( this->Self, "avg leaf depth " << static_cast<double>(totalLeafDepth) / numLeaf );
}

vtkLabelHierarchyIterator* vtkLabelHierarchy::NewIterator(
  int type,
  vtkCamera* cam,
  double frustumPlanes[24],
  bool positionsAsNormals)
{
  vtkLabelHierarchyIterator* iter = 0;
  if ( type == FULL_SORT )
    {
    vtkLabelHierarchyFullSortIterator* fs = vtkLabelHierarchyFullSortIterator::New();
    fs->Prepare( this, cam, frustumPlanes, positionsAsNormals );
    iter = fs;
    }
  else
    {
    vtkLabelHierarchyFrustumIterator* f = vtkLabelHierarchyFrustumIterator::New();
    f->Prepare( this, cam, frustumPlanes );
    iter = f;
    }
  return iter;
}

void vtkLabelHierarchy::GetDiscreteNodeCoordinatesFromWorldPoint( int ijk[3], double pt[3], int level )
{
  implementation::HierarchyType::octree_node_pointer root = this->Implementation->Hierarchy->root();
  double delta;
  const double* rootCenter;
  double sz;
  rootCenter = root->center();
  sz = root->size() / 2.;
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

void vtkLabelHierarchy::implementation::BinAnchorsToLevel( int level )
{
  // See comment near declaration of Current for more info:
  vtkLabelHierarchy::implementation::Current = this->Self;

  LabelSet emptyNode( this->Self );
  HierarchyCursor cursor;
  HierarchyCursor root = HierarchyCursor( this->Hierarchy );
  const double* ctr = root->center();
  double sz = root->size();
  double x[3];
  vtkIdType i;
  vtkIdType npts = this->Self->Points->GetNumberOfPoints();
  int m[3]; // "branch selector" for each axis (mX is 0 if rX < 0.5 or 1 otherwise)
  int child; // Always set to m0 + 2 * ( m1 + 2 * m2 ), offset into array of node children
  int j;
  for ( i = 0; i < npts; ++ i )
    {
    // Retrieve the point coordinates and node center
    this->Self->Points->GetPoint( i, x );
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

void vtkLabelHierarchy::implementation::PromoteAnchors()
{
  HierarchyCursor cursor;
  HierarchyCursor root = HierarchyCursor( this->Hierarchy );
  HierarchyIterator it;
  //vtkIdType cnt;
  vtkstd::vector<vtkIdType> promotionList;
  // Step 1. Iterate over all leaf nodes. We'll ascend to the root from each leaf, promoting anchors as we go.
  //         Outer loop is O(N) since the number of leaf nodes is proportional to the number of anchors.
  for ( it = this->Hierarchy->begin( true ); it != this->Hierarchy->end( true ); ++it )
    {
    vtkDebugWithObjectMacro( this->Self, "o " << it.level() << "(" << it->value().size() << ")" );
    cursor = it;
    size_t promotionCount = compute_number_to_promote( this->Self->TargetLabelCount, cursor.level(), 3, cursor->value().size() );
    LabelSet::iterator cit = cursor->value().begin();
    LabelSet::iterator eit;
    // Step 1a. Remove all the label anchors from the leaf that we're going to promote to *all* nodes above.
    //          This is o(TargetLabelCount/(2^d - 1)), which is O(1)
    for ( size_t i = 0; i < promotionCount; ++i )
      {
      if (cit == cursor->value().end())
        {
        vtkErrorWithObjectMacro( this->Self, "error: dereferencing iterator at end()" );
        }
      promotionList.push_back( *cit );
      vtkDebugWithObjectMacro( this->Self, "Promoting " << *cit << " ( " << cursor->value().key_comp().Hierarchy->GetPriorities()->GetTuple1( *cit ) << ")" );
      eit = cit;
      ++cit;
      cursor->value().erase( eit );
      }
    // FIXME: If we erase all the entries at this level, check to see if all siblings are empty as well.
    //        If so, delete children of parent node. This is complicated by fact that we must have a valid cursor to climb.
    vtkstd::vector<vtkIdType>::size_type start = 0;
    vtkstd::vector<vtkIdType>::size_type psize = promotionList.size();
    // Step 1b. While we have anchors left to distribute, climb the tree
    //          This loop is O(log(N)) since the tree is log(N) deep.
    while ( cursor.level() && ( start < psize ) )
      {
      cursor.up();
      // How many of our available anchors do we leave at this tree level?
      if ( cursor.level() )
        promotionCount = compute_number_to_promote( this->Self->TargetLabelCount, cursor.level(), 3, psize - start );
      else
        promotionCount = psize - start;
      vtkDebugWithObjectMacro( this->Self, " " << cursor.level() << "(" << promotionCount << ")" );
      // Insert them. This is O(1) since the list is O(1) in length at maximum
      cursor->value().insert( promotionList.begin() + start, promotionList.begin() + start + promotionCount );
      start += promotionCount;
      }
    promotionList.clear();
    vtkDebugWithObjectMacro( this->Self, "\n" );
    }
  // Total complexity is O(N*log(N))
}

void vtkLabelHierarchy::implementation::DemoteAnchors( int level )
{
  (void)level;
}

void vtkLabelHierarchy::implementation::RecursiveNodeDivide( HierarchyCursor& cursor )
{
  (void)cursor;
}

void vtkLabelHierarchy::implementation::PrepareSortedAnchors( LabelSet& anchors )
{
  anchors.clear();
  vtkIdType npts = this->Self->Points->GetNumberOfPoints();
  for ( vtkIdType i = 0; i < npts; ++ i )
    {
    anchors.insert( i );
    }
}

void vtkLabelHierarchy::implementation::FillHierarchyRoot( LabelSet& anchors )
{
  LabelSet::iterator endRootAnchors;
  if ( static_cast<int>( anchors.size() ) < this->Self->TargetLabelCount )
    {
    endRootAnchors = anchors.end();
    }
  else
    {
    endRootAnchors = anchors.begin();
    for ( int i = 0; i < this->Self->TargetLabelCount; ++ i )
      {
      ++ endRootAnchors;
      }
    }
  #if ! ( defined(_MSC_VER) && (_MSC_VER < 1300) )
    this->Hierarchy->root()->value().insert( anchors.begin(), endRootAnchors );
  #else
    for ( LabelSet::iterator it = anchors.begin(); it != endRootAnchors; ++ it )
      {
      this->Hierarchy->root()->value().insert( *it );
      }
  #endif
  anchors.erase( anchors.begin(), endRootAnchors );
}

void vtkLabelHierarchy::implementation::DropAnchor( vtkIdType anchor )
{
  // See comment near declaration of Current for more info:
  vtkLabelHierarchy::implementation::Current = this->Self;

  LabelSet emptyNode( this->Self );
  HierarchyCursor curs( this->Hierarchy );
  const double* ctr = curs->center();
  double x[3];
  double sz = curs->size();
  int m[3]; // "branch selector" for each axis (mX is 0 if rX < 0.5 or 1 otherwise)
  int child; // Always set to m0 + 2 * ( m1 + 2 * m2 ), offset into array of node children
  int j;
  // Retrieve the point coordinates
  this->Self->Points->GetPoint( anchor, x );

  //Coord coord(x);
  this->Self->CoincidentPoints->AddPoint(anchor, x);

  // Convert into "octree" coordinates (x[i] in [0,1[ for easy descent).
  for ( j = 0; j < 3; ++ j )
    {
    x[j] = ( x[j] - ctr[j] ) / sz + 0.5;
    }
  double thresh = 1.;
  while ( static_cast<int>( curs->value().size() ) >= this->Self->TargetLabelCount )
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
      curs->add_children( emptyNode );
      }
    curs.down( child );
    }
  curs->value().insert( anchor );

  this->SmudgeAnchor( curs, anchor, x );
}

// If an anchor is near any octree boundaries, copy it to neighbors at the same level.
// This will create neighbors if neccessary.
void vtkLabelHierarchy::implementation::SmudgeAnchor( HierarchyCursor& cursor, vtkIdType anchor, double* x )
{
  (void)cursor;
  (void)anchor;
  (void)x;
}
