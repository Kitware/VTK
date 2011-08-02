/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkModifiedBSPTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCellTreeLocator.h>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <stack>
#include <vector>
#include <limits>
#include <algorithm>
#include "vtkObjectFactory.h"
#include "vtkGenericCell.h"
#include "vtkIdListCollection.h"
#include "vtkSmartPointer.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkBoundingBox.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkCellTreeLocator);

namespace
{
  const double EPSILON_= 1E-8;
  enum { POS_X, NEG_X, POS_Y, NEG_Y, POS_Z, NEG_Z };
  #define CELLTREE_MAX_DEPTH 32
}

// -------------------------------------------------------------------------
// This class is the basic building block of the cell tree.  There is a node per dimension. i.e. There are 3 vtkCellTreeNode
// in x,y,z directions.  In contrast, vtkModifiedBSPTree class stores the bounding planes for all 3 dimensions in a single node.
// LeftMax and rm defines the bounding planes.
// start is the location in the cell tree. e.g. for root node start is zero.
// size is the number of the nodes under the tree  
inline void vtkCellTreeLocator::vtkCellTreeNode::MakeNode( unsigned int left, unsigned int d, float b[2] ) // b is an array containing left max and right min values
{
  Index = (d & 3) | (left << 2);
  LeftMax = b[0];
  RightMin = b[1];	
}
//----------------------------------------------------------------------------
inline void vtkCellTreeLocator::vtkCellTreeNode::SetChildren( unsigned int left )
{
  Index = GetDimension() | (left << 2); // In index 2 LSBs (Least Significant Bits) store the dimension. MSBs store the position
}
//----------------------------------------------------------------------------
inline bool vtkCellTreeLocator::vtkCellTreeNode::IsNode() const
{
  return (Index & 3) != 3;    // For a leaf 2 LSBs in index is 3
}
//----------------------------------------------------------------------------
inline unsigned int vtkCellTreeLocator::vtkCellTreeNode::GetLeftChildIndex() const
{
  return (Index >> 2);
}
//----------------------------------------------------------------------------
inline unsigned int vtkCellTreeLocator::vtkCellTreeNode::GetRightChildIndex() const
{
  return (Index >> 2) + 1;  // Right child node is adjacent to the Left child node in the data structure
}
//----------------------------------------------------------------------------
inline unsigned int vtkCellTreeLocator::vtkCellTreeNode::GetDimension() const
{
  return Index & 3;
}
//----------------------------------------------------------------------------
inline const float& vtkCellTreeLocator::vtkCellTreeNode::GetLeftMaxValue() const
{
  return LeftMax;
}
//----------------------------------------------------------------------------
inline const float& vtkCellTreeLocator::vtkCellTreeNode::GetRightMinValue() const
{
  return RightMin;
}
//----------------------------------------------------------------------------
inline void vtkCellTreeLocator::vtkCellTreeNode::MakeLeaf( unsigned int start, unsigned int size )
{
  Index = 3;
  Sz = size;
  St = start;
}
//----------------------------------------------------------------------------
bool vtkCellTreeLocator::vtkCellTreeNode::IsLeaf() const
{
  return Index == 3;
}
//----------------------------------------------------------------------------
unsigned int vtkCellTreeLocator::vtkCellTreeNode::Start() const
{
  return St;
}
//----------------------------------------------------------------------------
unsigned int vtkCellTreeLocator::vtkCellTreeNode::Size() const
{
  return Sz;
}
//----------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------
class vtkCellPointTraversal
{
  private:
    const vtkCellTreeLocator::vtkCellTree& CellTreeToBeTraversed;
    unsigned int    CellTreeNodeStack[CELLTREE_MAX_DEPTH]; 
    unsigned int*   CellTreeStackPointer; // stack pointer
    const float*    PointPosition; //3-D coordinates of the points

  protected:
    friend class vtkCellTreeLocator::vtkCellTree;
    friend class vtkCellTreeLocator::vtkCellTreeNode;
    friend class vtkCellTreeBuilder;

  public:
    vtkCellPointTraversal( const vtkCellTreeLocator::vtkCellTree& ct, const float* pos ) :
        CellTreeToBeTraversed(ct), PointPosition(pos)
          {
          this->CellTreeNodeStack[0] = 0; // first element is set to zero
          this->CellTreeStackPointer = this->CellTreeNodeStack + 1; //this points to the second element of the stack
          }

        const vtkCellTreeLocator::vtkCellTreeNode* Next()  // this returns n (the location in the CellTree) if it is a leaf or 0 if the point doesnt contain in the data domain
          {
          while( true )
            {
            if( this->CellTreeStackPointer == this->CellTreeNodeStack ) //This means the point is not within the domain
              {
              return 0;
              }

            const vtkCellTreeLocator::vtkCellTreeNode* n = &this->CellTreeToBeTraversed.Nodes.front() + *(--this->CellTreeStackPointer);

            if( n->IsLeaf() )
              {
              return n;
              }

            const float p = PointPosition[n->GetDimension()];
            const unsigned int left = n->GetLeftChildIndex();

            bool l = p <= n->GetLeftMaxValue(); // Check if the points is within the left sub tree
            bool r = p >= n->GetRightMinValue(); // Check if the point is within the right sub tree

            if( l && r ) //  This means if there is a overlap region both left and right sub trees should be traversed
              {
              if( n->GetLeftMaxValue()-p < p-n->GetRightMinValue() )
                {
                *(this->CellTreeStackPointer++) = left;
                *(this->CellTreeStackPointer++) = left+1;
                }
              else
                {
                *(this->CellTreeStackPointer++) = left+1;
                *(this->CellTreeStackPointer++) = left;
                }
              }
            else if( l )
              {
              *(this->CellTreeStackPointer++) = left;
              }
            else if( r )
              {
              *(this->CellTreeStackPointer++) = left+1;
              }
            }
          }
};
//----------------------------------------------------------------------------
// This class builds the CellTree according to the algorithm given in the paper.
//----------------------------------------------------------------------------
class vtkCellTreeBuilder
{
  private:

    struct Bucket
      {
      float  Min;
      float  Max;
      unsigned int Cnt;

      Bucket()
        {
        Cnt = 0;
        Min =  std::numeric_limits<float>::max();
        Max = -std::numeric_limits<float>::max();
        }

      void Add( const float _min, const float _max )
        {
        ++Cnt;

        if( _min < Min )
          {
          Min = _min;
          }

        if( _max > Max )
          {
          Max = _max;
          }
        }
      };

    struct PerCell 
      {
      float  CellMin[3];
      float  CellMax[3];
      unsigned int Ind;
      };

    struct CenterOrder
      {
      unsigned int d;
      CenterOrder( unsigned int _d ) : d(_d) {}

      bool operator()( const PerCell& pc0, const PerCell& pc1 )
        {
        return (pc0.CellMin[d] + pc0.CellMax[d]) < (pc1.CellMin[d] + pc1.CellMax[d]);
        }
      };

    struct LeftPredicate
      {
      unsigned int d;
      float  p;
      LeftPredicate( unsigned int _d, float _p ) :  d(_d), p(2.0f*_p) {}

      bool operator()( const PerCell& pc )
        {
        return (pc.CellMin[d] + pc.CellMax[d]) < p;
        }
      };


    // -------------------------------------------------------------------------

    void FindMinMax( const PerCell* begin, const PerCell* end,  
      float* min, float* max )
      {
      if( begin == end )
        {
        return;
        }

      for( unsigned int d=0; d<3; ++d )
        {
        min[d] = begin->CellMin[d];
        max[d] = begin->CellMax[d];
        }


      while( ++begin != end )
        {
        for( unsigned int d=0; d<3; ++d )
          {
          if( begin->CellMin[d] < min[d] )    min[d] = begin->CellMin[d];
          if( begin->CellMax[d] > max[d] )    max[d] = begin->CellMax[d];
          }
        }
      }

    //----------------------------------------------------------------------------

    void FindMinD( const PerCell* begin, const PerCell* end,  
      unsigned int d, float& min )
      {
      min = begin->CellMin[d];

      while( ++begin != end )
        {
        if( begin->CellMin[d] < min )
          {    
          min = begin->CellMin[d];
          }
        }
      }

    void FindMaxD( const PerCell* begin, const PerCell* end,  
      unsigned int d, float& max )
      {
      max = begin->CellMax[d];

      while( ++begin != end )
        {
        if( begin->CellMax[d] > max )    
          {
          max = begin->CellMax[d];
          }
        }
      }

    // -------------------------------------------------------------------------

    void Split( unsigned int index, float min[3], float max[3] )
      {
      unsigned int start = this->BuiltNodes[index].Start();
      unsigned int size  = this->BuiltNodes[index].Size();

      if( size < this->LeafSize )
        {
        return;
        }

      PerCell* begin = this->SingleCell + start;
      PerCell* end   = this->SingleCell + start + size;
      PerCell* mid = begin;

      const int nbuckets = 6;

      const float ext[3] = { max[0]-min[0], max[1]-min[1], max[2]-min[2] };
      const float iext[3] = { nbuckets/ext[0], nbuckets/ext[1], nbuckets/ext[2] };

      Bucket b[3][nbuckets];

      for( const PerCell* pc=begin; pc!=end; ++pc )
        {
        for( unsigned int d=0; d<3; ++d )
          {
          float cen = (pc->CellMin[d] + pc->CellMax[d])/2.0f;
          int   ind = (int)( (cen-min[d])*iext[d] );

          if( ind<0 )
            {
            ind = 0;
            }

          if( ind>=nbuckets )
            {
            ind = nbuckets-1;
            }

          b[d][ind].Add( pc->CellMin[d], pc->CellMax[d] );
          }
        }

      float cost = std::numeric_limits<float>::max();
      float plane;
      unsigned int dim;

      for( unsigned int d=0; d<3; ++d )    
        {
        unsigned int sum = 0;

        for( unsigned int n=0; n< (unsigned int)nbuckets-1; ++n )
          {
          float lmax = -std::numeric_limits<float>::max();
          float rmin =  std::numeric_limits<float>::max();

          for( unsigned int m=0; m<=n; ++m )
            {
            if( b[d][m].Max > lmax )
              {
              lmax = b[d][m].Max;
              }
            }

          for( unsigned int m=n+1; m< (unsigned int) nbuckets; ++m )
            {
            if( b[d][m].Min < rmin )
              {
              rmin = b[d][m].Min;
              }
            }

          //
          // JB : added if (...) to stop floating point error if rmin is unset
          // this happens when some buckets are empty (bad volume calc)
          //
          if (lmax != -std::numeric_limits<float>::max() &&
              rmin !=  std::numeric_limits<float>::max())
            {
              sum += b[d][n].Cnt;

              float lvol = (lmax-min[d])/ext[d];
              float rvol = (max[d]-rmin)/ext[d];

              float c = lvol*sum + rvol*(size-sum);

              if( sum > 0 && sum < size && c < cost )
                {
                cost    = c;
                dim     = d;
                plane   = min[d] + (n+1)/iext[d];
                }
            }
          }
        }

      if( cost != std::numeric_limits<float>::max() )
        {
        mid = std::partition( begin, end, LeftPredicate( dim, plane ) );
        }

      // fallback
      if( mid == begin || mid == end )
        {
        dim = std::max_element( ext, ext+3 ) - ext;

        mid = begin + (end-begin)/2;
        std::nth_element( begin, mid, end, CenterOrder( dim ) );
        }

      float lmin[3], lmax[3], rmin[3], rmax[3];

      FindMinMax( begin, mid, lmin, lmax );
      FindMinMax( mid,   end, rmin, rmax );

      float clip[2] = { lmax[dim], rmin[dim]}; 

      vtkCellTreeLocator::vtkCellTreeNode child[2];
      child[0].MakeLeaf( begin - this->SingleCell, mid-begin );
      child[1].MakeLeaf( mid   - this->SingleCell, end-mid );

      this->BuiltNodes[index].MakeNode( (int)BuiltNodes.size(), dim, clip );
      this->BuiltNodes.insert( BuiltNodes.end(), child, child+2 );

      Split( this->BuiltNodes[index].GetLeftChildIndex(), lmin, lmax );
      Split( this->BuiltNodes[index].GetRightChildIndex(), rmin, rmax );
      }

  public:

    vtkCellTreeBuilder()
      {
      this->Buckets =  5;
      this->LeafSize = 8;
      }

    void Build( vtkCellTreeLocator *ctl, vtkCellTreeLocator::vtkCellTree& ct, vtkDataSet* ds )
      {
      const vtkIdType size = ds->GetNumberOfCells();
      assert( size <= std::numeric_limits<unsigned int>::max() );
      double cellBounds[6];
      this->SingleCell = new PerCell[size];

      float min[3] = 
        { 
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
        };

      float max[3] = 
        { 
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        };

      for( unsigned int i=0; i<size; ++i )
        {
        this->SingleCell[i].Ind = i;

        double *boundsPtr = cellBounds;
        if (ctl->CellBounds) 
          {
          boundsPtr = ctl->CellBounds[i];
          }
        else 
          {
          ds->GetCellBounds(i, boundsPtr);
          }

        for( int d=0; d<3; ++d )
          {
          this->SingleCell[i].CellMin[d] = boundsPtr[2*d+0];
          this->SingleCell[i].CellMax[d] = boundsPtr[2*d+1];

          if( this->SingleCell[i].CellMin[d] < min[d] )
            {
            min[d] = this->SingleCell[i].CellMin[d];
            }

          if( this->SingleCell[i].CellMax[d] > max[d] )  /// This can be SingleCell[i].max[d] instead of min
            {
            max[d] = this->SingleCell[i].CellMax[d];
            }
          }
        }

      ct.DataBBox[0] = min[0];
      ct.DataBBox[1] = max[0];
      ct.DataBBox[2] = min[1];
      ct.DataBBox[3] = max[1];
      ct.DataBBox[4] = min[2];
      ct.DataBBox[5] = max[2];

      vtkCellTreeLocator::vtkCellTreeNode root;
      root.MakeLeaf( 0, size );
      this->BuiltNodes.push_back( root );

      Split( 0, min, max );

      ct.Nodes.resize( this->BuiltNodes.size() );
      ct.Nodes[0] = this->BuiltNodes[0];

      std::vector<vtkCellTreeLocator::vtkCellTreeNode>::iterator ni = ct.Nodes.begin();
      std::vector<vtkCellTreeLocator::vtkCellTreeNode>::iterator nn = ct.Nodes.begin()+1;

      for( ; ni!=ct.Nodes.end(); ++ni )
        {
        if( ni->IsLeaf() )
          {
          continue;
          }

        *(nn++) = this->BuiltNodes[ni->GetLeftChildIndex()];
        *(nn++) = this->BuiltNodes[ni->GetRightChildIndex()];

        ni->SetChildren( nn-ct.Nodes.begin()-2 );
        }

      // --- final 

      ct.Leaves.resize( size );

      for( int i=0; i<size; ++i )
        {
        ct.Leaves[i] = this->SingleCell[i].Ind;
        }

      delete[] this->SingleCell;
      }

  public:

    unsigned int     Buckets;
    unsigned int     LeafSize;
    PerCell*   SingleCell;
    std::vector<vtkCellTreeLocator::vtkCellTreeNode>    BuiltNodes;
};

//----------------------------------------------------------------------------

typedef std::stack<vtkCellTreeLocator::vtkCellTreeNode*, std::vector<vtkCellTreeLocator::vtkCellTreeNode*> > nodeStack;

vtkCellTreeLocator::vtkCellTreeLocator( ) 
{
  this->MaxCellsPerLeaf = 8;
  this->NumberOfBuckets = 5;
  this->Tree            = NULL;
}

//----------------------------------------------------------------------------

vtkCellTreeLocator::~vtkCellTreeLocator()
{
  this->FreeSearchStructure();
}

//----------------------------------------------------------------------------
void vtkCellTreeLocator::BuildLocatorIfNeeded()
{
  if (this->LazyEvaluation)
    {
    if (!this->Tree || (this->Tree && (this->MTime>this->BuildTime)))
      {
      this->Modified();
      vtkDebugMacro(<< "Forcing BuildLocator");
      this->ForceBuildLocator();
      }
    }
}
//----------------------------------------------------------------------------
void vtkCellTreeLocator::ForceBuildLocator()
{
  //
  // don't rebuild if build time is newer than modified and dataset modified time
  if ( (this->Tree) &&
    (this->BuildTime>this->MTime) &&
    (this->BuildTime>DataSet->GetMTime()))
    {
    return;
    }
  // don't rebuild if UseExistingSearchStructure is ON and a tree structure already exists
  if ( (this->Tree) && this->UseExistingSearchStructure)
    {
    this->BuildTime.Modified();
    vtkDebugMacro(<< "BuildLocator exited - UseExistingSearchStructure");
    return;
    }
  this->BuildLocatorInternal();
}
//----------------------------------------------------------------------------
void vtkCellTreeLocator::BuildLocatorInternal()
{
  this->FreeSearchStructure();
  if ( !this->DataSet || (this->DataSet->GetNumberOfCells() < 1) )
    {
    vtkErrorMacro( << " No Cells in the data set\n");
    return;
    }
  //
  if (this->CacheCellBounds) 
    {
    this->StoreCellBounds();
    }
  //
  this->Tree = new vtkCellTree;
  vtkCellTreeBuilder builder;
  builder.LeafSize = MaxCellsPerLeaf;
  builder.Buckets  = NumberOfBuckets;
  builder.Build( this, *(Tree), this->DataSet );
  this->BuildTime.Modified();
}

void vtkCellTreeLocator::BuildLocator()
{
  if (this->LazyEvaluation)
    {
    return;
    }
  this->ForceBuildLocator();
}

//----------------------------------------------------------------------------

vtkIdType vtkCellTreeLocator::FindCell( double pos[3], double , vtkGenericCell *cell,  double pcoords[3], 
  double* weights )
{
  if( this->Tree == 0 )
    {
    return -1;
    }

  double closestPoint[3], dist2;
  int subId;

  const float _pos[3] = { pos[0], pos[1], pos[2] };
  vtkCellPointTraversal pt( *(this->Tree), _pos );

  //bool found = false;

  while( const vtkCellTreeNode* n = pt.Next() )
    {
    const unsigned int* begin = &(this->Tree->Leaves[n->Start()]);
    const unsigned int* end   = begin + n->Size();

    for( ; begin!=end; ++begin )
      {
      this->DataSet->GetCell(*begin, cell);
      if( cell->EvaluatePosition(pos, closestPoint, subId, pcoords, dist2, weights)==1 )
        {
        return *begin;
        }
      }
    }

  return -1;
  }

//----------------------------------------------------------------------------

namespace
{
  double _getMinDistPOS_X(const double origin[3], const double dir[3], const double B[6])
  {
    return ((B[0] - origin[0]) / dir[0]);
  }
  double _getMinDistNEG_X(const double origin[3], const double dir[3], const double B[6])
  {
    return ((B[1] - origin[0]) / dir[0]);
  }
  double _getMinDistPOS_Y(const double origin[3], const double dir[3], const double B[6])
  {
    return ((B[2] - origin[1]) / dir[1]);
  }
  double _getMinDistNEG_Y(const double origin[3], const double dir[3], const double B[6])
  {
    return ((B[3] - origin[1]) / dir[1]);
  }
  double _getMinDistPOS_Z(const double origin[3], const double dir[3], const double B[6])
  {
    return ((B[4] - origin[2]) / dir[2]);
  }
  double _getMinDistNEG_Z(const double origin[3], const double dir[3], const double B[6])
  {
    return ((B[5] - origin[2]) / dir[2]);
  }

}
typedef std::pair<double, int> Intersection;

int vtkCellTreeLocator::IntersectWithLine(double p1[3],
                                          double p2[3],
                                          double tol,
                                          double &t,
                                          double x[3],
                                          double pcoords[3],
                                          int &subId,
                                          vtkIdType &cellId,
                                          vtkGenericCell *cell)
{
  int hit = this->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId, cellId);
  if (hit)
    {
    this->DataSet->GetCell(cellId, cell);
    }
  return hit;
}

int vtkCellTreeLocator::IntersectWithLine(double p1[3], double p2[3], double tol,
  double& t, double x[3], double pcoords[3],
  int &subId, vtkIdType &cellIds)
{
  //
  vtkCellTreeNode  *node, *near, *far;
  double    ctmin, ctmax, tmin, tmax, _tmin, _tmax, tDist;
  double    ray_vec[3] = { p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2] };

  double *boundsPtr;
  double cellBounds[6]; 

  this->BuildLocatorIfNeeded();

  // Does ray pass through root BBox
  tmin = 0; tmax = 1;

  if (!RayMinMaxT(p1, ray_vec, tmin, tmax)) // 0 for root node
    {
    return false;
    }
  // Ok, setup a stack and various params
  nodeStack  ns;
  double    closest_intersection = VTK_LARGE_FLOAT;
  bool     HIT = false;
  // setup our axis optimized ray box edge stuff
  int axis = getDominantAxis(ray_vec);
  double (*_getMinDist)(const double origin[3], const double dir[3], const double B[6]);
  switch (axis) 
    {
    case POS_X: _getMinDist = _getMinDistPOS_X; break;
    case NEG_X: _getMinDist = _getMinDistNEG_X; break;
    case POS_Y: _getMinDist = _getMinDistPOS_Y; break;
    case NEG_Y: _getMinDist = _getMinDistNEG_Y; break;
    case POS_Z: _getMinDist = _getMinDistPOS_Z; break;
    default:    _getMinDist = _getMinDistNEG_Z; break;
    }


  //
  // OK, lets walk the tree and find intersections
  //
  vtkCellTreeNode* n = &this->Tree->Nodes.front();
  ns.push(n);
  while (!ns.empty())
    {
    node = ns.top();
    ns.pop();
    // We do as few tests on the way down as possible, because our BBoxes
    // can be quite tight and we want to reject as many boxes as possible without
    // testing them at all - mainly because we quickly get to a leaf node and
    // test candidates, once we've found a hit, we note the intersection t val,
    // as soon as we pull a BBox of the stack that has a closest point further
    // than the t val, we know we can stop.

    int mustCheck = 0;  // to check if both left and right sub trees need to be checked

    //
    while (!node->IsLeaf())
      { // this must be a parent node
      // Which child node is closest to ray origin - given direction
      Classify(p1, ray_vec, tDist, near, node, far, mustCheck);
      // if the distance to the far edge of the near box is > tmax, no need to test far box
      // (we still need to test Mid because it may overlap slightly)
      if(mustCheck)
        {
        ns.push(far);
        node = near;
        }
      else if ((tDist > tmax) || (tDist <= 0) )
        { //<=0 for ray on edge
        node = near;
        }
      // if the distance to the far edge of the near box is < tmin, no need to test near box
      else if (tDist < tmin)
        { 
        ns.push(near);  
        node = far;
        }
      // All the child nodes may be candidates, keep near, push far then mid
      else
        {
        ns.push(far);
        node = near;
        }
      }
    double t_hit, ipt[3];
    // Ok, so we're a leaf node, first check the BBox against the ray
    // then test the candidates in our sorted ray direction order
    _tmin = tmin; _tmax = tmax;
    //    if (node->RayMinMaxT(p1, ray_vec, _tmin, _tmax)) {
    // Was the closest point on the box > intersection point
    //if (_tmax>closest_intersection) break;
    //
    for (int i=0; i< (int)node->Size(); i++)
      {
      vtkIdType cell_ID = this->Tree->Leaves[node->Start()+i];
      //

      boundsPtr = cellBounds;
      if (this->CellBounds)
        {
        boundsPtr = this->CellBounds[cell_ID];
        }
      else
        {
        this->DataSet->GetCellBounds(cell_ID, cellBounds);
        }
      if (_getMinDist(p1, ray_vec, cellBounds) > closest_intersection)
        {
        break;
        }
      //
      ctmin = _tmin; ctmax = _tmax;
      if (RayMinMaxT(cellBounds, p1, ray_vec, ctmin, ctmax))
        {
        if (this->IntersectCellInternal(cell_ID, p1, p2, tol, t_hit, ipt, pcoords, subId))
          {
          if (t_hit<closest_intersection)
            {
            HIT = true;
            closest_intersection = t_hit;
            cellIds = cell_ID;
            x[0] = ipt[0];
            x[1] = ipt[1];
            x[2] = ipt[2];
            }


          }
        }
      }
    //    }
    }
  if (HIT)
    {
    t = closest_intersection;
    }
  //
  return HIT;

}
//----------------------------------------------------------------------------
bool vtkCellTreeLocator::RayMinMaxT(const double origin[3],
  const double dir[3],
  double &rTmin,
  double &rTmax) 
{
  double tT;
  // X-Axis
  float bounds[6]; 

  bounds[0] = this->Tree->DataBBox[0];
  bounds[1] = this->Tree->DataBBox[1];
  bounds[2] = this->Tree->DataBBox[2];
  bounds[3] = this->Tree->DataBBox[3];
  bounds[4] = this->Tree->DataBBox[4];
  bounds[5] = this->Tree->DataBBox[5];

  if (dir[0] < -EPSILON_)
    {    // ray travelling in -x direction
    tT = (bounds[0] - origin[0]) / dir[0];
    if (tT < rTmin)
      {
      return (false);  // ray already left of box. Can't hit
      }
    if (tT <= rTmax)
      {
      rTmax = tT;     // update new tmax
      }
    tT = (bounds[1] - origin[0]) / dir[0]; // distance to right edge
    if (tT >= rTmin)
      {   // can't see this ever happening
      if (tT > rTmax)
        {
        return false;  // clip start of ray to right edge
        }
      rTmin = tT;
      }
    }
  else if (dir[0] > EPSILON_)
    {
    tT = (bounds[1] - origin[0]) / dir[0];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[0] - origin[0]) / dir[0];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (origin[0] < bounds[0] || origin[0] > bounds[1])
    {
    return (false);
    }
  // Y-Axis
  if (dir[1] < -EPSILON_)
    {
    tT = (bounds[2] - origin[1]) / dir[1];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[3] - origin[1]) / dir[1];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (dir[1] > EPSILON_)
    {
    tT = (bounds[3] - origin[1]) / dir[1];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[2] - origin[1]) / dir[1];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (origin[1] < bounds[2] || origin[1] > bounds[3])
    {
    return (false);
    }
  // Z-Axis
  if (dir[2] < -EPSILON_)
    {
    tT = (bounds[4] - origin[2]) / dir[2];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[5] - origin[2]) / dir[2];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (dir[2] > EPSILON_)
    {
    tT = (bounds[5] - origin[2]) / dir[2];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[4] - origin[2]) / dir[2];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (origin[2] < bounds[4] || origin[2] > bounds[5])
    {
    return (false);
    }
  return (true);
}
//----------------------------------------------------------------------------
bool vtkCellTreeLocator::RayMinMaxT(const double bounds[6],
  const double origin[3],
  const double dir[3],
  double &rTmin,
  double &rTmax)
{
  double tT;
  // X-Axis
  if (dir[0] < -EPSILON_)
    {    // ray travelling in -x direction
    tT = (bounds[0] - origin[0]) / dir[0]; // Ipoint less than minT - ray outside box!
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;     // update new tmax
      }
    tT = (bounds[1] - origin[0]) / dir[0]; // distance to right edge
    if (tT >= rTmin)
      {   // can't see this ever happening
      if (tT > rTmax)
        {
        return false;  // clip start of ray to right edge
        }
      rTmin = tT;
      }
    }
  else if (dir[0] > EPSILON_)
    {
    tT = (bounds[1] - origin[0]) / dir[0];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[0] - origin[0]) / dir[0];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (origin[0] < bounds[0] || origin[0] > bounds[1])
    {
    return (false);
    }
  // Y-Axis
  if (dir[1] < -EPSILON_)
    {
    tT = (bounds[2] - origin[1]) / dir[1];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax) rTmax = tT;
    tT = (bounds[3] - origin[1]) / dir[1];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (dir[1] > EPSILON_)
    {
    tT = (bounds[3] - origin[1]) / dir[1];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[2] - origin[1]) / dir[1];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (origin[1] < bounds[2] || origin[1] > bounds[3])
    {
    return (false);
    }
  // Z-Axis
  if (dir[2] < -EPSILON_)
    {
    tT = (bounds[4] - origin[2]) / dir[2];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[5] - origin[2]) / dir[2];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (dir[2] > EPSILON_)
    {
    tT = (bounds[5] - origin[2]) / dir[2];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[4] - origin[2]) / dir[2];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (origin[2] < bounds[4] || origin[2] > bounds[5])
    {
    return (false);
    }
  return (true);
}
//----------------------------------------------------------------------------
int vtkCellTreeLocator::getDominantAxis(const double dir[3])
{
  double tX = (dir[0]>0) ? dir[0] : -dir[0];
  double tY = (dir[1]>0) ? dir[1] : -dir[1];
  double tZ = (dir[2]>0) ? dir[2] : -dir[2];
  if (tX > tY && tX > tZ)
    {
    return ((dir[0] > 0) ? POS_X : NEG_X);
    }
  else if ( tY > tZ )
    {
    return ((dir[1] > 0) ? POS_Y : NEG_Y);
    }
  else
    {
    return ((dir[2] > 0) ? POS_Z : NEG_Z);
    }
}
//----------------------------------------------------------------------------
void vtkCellTreeLocator::Classify(const double origin[3],
  const double dir[3],
  double &rDist,
  vtkCellTreeNode *&near, vtkCellTreeNode *&parent,
  vtkCellTreeNode *&far, int& mustCheck)
{
  double tOriginToDivPlane = parent->GetLeftMaxValue() - origin[parent->GetDimension()];
  double tOriginToDivPlane2 = parent->GetRightMinValue() - origin[parent->GetDimension()];
  double tDivDirection   = dir[parent->GetDimension()];
  if ( tOriginToDivPlane2 > 0 )  // origin is right of the rmin
    {
    near = &this->Tree->Nodes.at(parent->GetLeftChildIndex());
    far  = &this->Tree->Nodes.at(parent->GetLeftChildIndex()+1);
    rDist = (tDivDirection) ? tOriginToDivPlane2 / tDivDirection : VTK_LARGE_FLOAT;
    }
  else if (tOriginToDivPlane < 0)  // origin is left of the lm
    {
    far  = &this->Tree->Nodes.at(parent->GetLeftChildIndex());
    near = &this->Tree->Nodes.at(parent->GetLeftChildIndex()+1);
    rDist = (tDivDirection) ? tOriginToDivPlane / tDivDirection : VTK_LARGE_FLOAT;
    }


  else 
    {
    if(tOriginToDivPlane > 0 && tOriginToDivPlane2 < 0)
      {
      mustCheck = 1;  // The point is within right min and left max. both left and right subtrees must be checked
      }

    if ( tDivDirection < 0)
      {
      near = &this->Tree->Nodes.at(parent->GetLeftChildIndex());
      far  = &this->Tree->Nodes.at(parent->GetLeftChildIndex()+1);
      if(!(tOriginToDivPlane > 0 || tOriginToDivPlane < 0))
        {
        mustCheck=1;  // Ray was exactly on edge left max box.
        }
      rDist = (tDivDirection) ? 0 / tDivDirection : VTK_LARGE_FLOAT;
      }
    else
      {
      far  = &this->Tree->Nodes.at(parent->GetLeftChildIndex());
      near = &this->Tree->Nodes.at(parent->GetLeftChildIndex()+1);
      if(!(tOriginToDivPlane2 > 0 || tOriginToDivPlane2 < 0))
        {
        mustCheck=1; // Ray was exactly on edge right min box.
        }
      rDist = (tDivDirection) ? 0 / tDivDirection : VTK_LARGE_FLOAT;
      }
    }

}
//----------------------------------------------------------------------------
int vtkCellTreeLocator::IntersectCellInternal(
  vtkIdType cell_ID,
  const double p1[3],
  const double p2[3],
  const double tol,
  double &t,
  double ipt[3],
  double pcoords[3],
  int &subId)
{
  this->DataSet->GetCell(cell_ID, this->GenericCell);
  return this->GenericCell->IntersectWithLine(const_cast<double*>(p1), const_cast<double*>(p2), tol, t, ipt, pcoords, subId);
}
//----------------------------------------------------------------------------
void vtkCellTreeLocator::FreeSearchStructure(void)
{
  if( this->Tree )
    {
    delete this->Tree;
    this->Tree = NULL;
    }
  this->FreeCellBounds();
}
//---------------------------------------------------------------------------
// For drawing coloured boxes, we want the level/depth
typedef std::pair<vtkBoundingBox, int> boxLevel;
typedef std::vector<boxLevel> boxlist;
// For testing bounds of the tree we need node/box
typedef std::pair<vtkCellTreeLocator::vtkCellTreeNode*, boxLevel> nodeBoxLevel;
typedef std::stack<nodeBoxLevel, std::vector<nodeBoxLevel> > nodeinfostack;
//---------------------------------------------------------------------------
void SplitNodeBox(vtkCellTreeLocator::vtkCellTreeNode *n, vtkBoundingBox &b, vtkBoundingBox &l, vtkBoundingBox &r)
{
  double minpt[3], maxpt[3];
  // create a box for left node
  vtkBoundingBox ll(b);
  ll.GetMaxPoint(maxpt[0], maxpt[1], maxpt[2]);
  maxpt[n->GetDimension()] = n->GetLeftMaxValue();
  ll.SetMaxPoint(maxpt[0], maxpt[1], maxpt[2]);
  l = ll;
  // create a box for right node
  vtkBoundingBox rr(b);
  rr.GetMinPoint(minpt[0], minpt[1], minpt[2]);
  minpt[n->GetDimension()] = n->GetRightMinValue();
  rr.SetMinPoint(minpt[0], minpt[1], minpt[2]);
  r = rr;
}
//---------------------------------------------------------------------------
void AddBox(vtkPolyData *pd, double *bounds, int level)
{
  vtkPoints      *pts = pd->GetPoints();
  vtkCellArray *lines = pd->GetLines();
  vtkIntArray *levels = vtkIntArray::SafeDownCast(pd->GetPointData()->GetArray(0));
  double x[3];
  vtkIdType cells[8], ids[2];
  //
  x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[4];
  cells[0] = pts->InsertNextPoint(x);
  x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[4];
  cells[1] = pts->InsertNextPoint(x);
  x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[4];
  cells[2] = pts->InsertNextPoint(x);
  x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[4];
  cells[3] = pts->InsertNextPoint(x);
  x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[5];
  cells[4] = pts->InsertNextPoint(x);
  x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[5];
  cells[5] = pts->InsertNextPoint(x);
  x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[5];
  cells[6] = pts->InsertNextPoint(x);
  x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[5];
  cells[7] = pts->InsertNextPoint(x);
  //
  ids[0] = cells[0]; ids[1] = cells[1];
  lines->InsertNextCell(2,ids);
  ids[0] = cells[2]; ids[1] = cells[3];
  lines->InsertNextCell(2,ids);
  ids[0] = cells[4]; ids[1] = cells[5];
  lines->InsertNextCell(2,ids);
  ids[0] = cells[6]; ids[1] = cells[7];
  lines->InsertNextCell(2,ids);
  ids[0] = cells[0]; ids[1] = cells[2];
  lines->InsertNextCell(2,ids);
  ids[0] = cells[1]; ids[1] = cells[3];
  lines->InsertNextCell(2,ids);
  ids[0] = cells[4]; ids[1] = cells[6];
  lines->InsertNextCell(2,ids);
  ids[0] = cells[5]; ids[1] = cells[7];
  lines->InsertNextCell(2,ids);
  ids[0] = cells[0]; ids[1] = cells[4];
  lines->InsertNextCell(2,ids);
  ids[0] = cells[1]; ids[1] = cells[5];
  lines->InsertNextCell(2,ids);
  ids[0] = cells[2]; ids[1] = cells[6];
  lines->InsertNextCell(2,ids);
  ids[0] = cells[3]; ids[1] = cells[7];
  lines->InsertNextCell(2,ids);
  //
  // Colour boxes by scalar if array present
  //
  for (int i=0; levels && i<8; i++) 
    {
    levels->InsertNextTuple1(level);
    }
}
//---------------------------------------------------------------------------
void vtkCellTreeLocator::GenerateRepresentation(int level, vtkPolyData *pd)
{
  this->BuildLocatorIfNeeded();
  //
  nodeinfostack ns;
  boxlist   bl;
  //
  vtkCellTreeNode *n0 = &this->Tree->Nodes.front();
  // create a box for the root
  float *DataBBox = this->Tree->DataBBox;
  vtkBoundingBox lbox, rbox, rootbox(DataBBox[0], DataBBox[1], DataBBox[2], DataBBox[3], DataBBox[4], DataBBox[5]);
  ns.push(nodeBoxLevel(n0,boxLevel(rootbox,0)));
  while (!ns.empty())
    {
    n0 = ns.top().first;
    int lev = ns.top().second.second;
    if (n0->IsLeaf() && ((lev==level) || (level==-1))) 
      {
      bl.push_back(boxLevel(ns.top().second.first,lev));
      ns.pop();
      }
    else if (n0->IsLeaf())
      {
      ns.pop();
      }
    else if (n0->IsNode())
      {
      SplitNodeBox(n0, ns.top().second.first, lbox, rbox);
      vtkCellTreeNode *n1 = &this->Tree->Nodes.at(n0->GetLeftChildIndex());
      vtkCellTreeNode *n2 = &this->Tree->Nodes.at(n0->GetLeftChildIndex()+1);
      ns.pop();
      ns.push(nodeBoxLevel(n1,boxLevel(lbox,lev+1)));
      ns.push(nodeBoxLevel(n2,boxLevel(rbox,lev+1)));
      }
    }
  //
  //
  //
  // For each node, add the bbox to our polydata
  int s = (int) bl.size();
  for (int i=0; i<s; i++) 
    {
    double bounds[6];
    bl[i].first.GetBounds(bounds);
    AddBox(pd, bounds, bl[i].second);
    }
}
//---------------------------------------------------------------------------
void vtkCellTreeLocator::FindCellsWithinBounds(double *bbox, vtkIdList *cells)
{
  this->BuildLocatorIfNeeded();
  //
  nodeinfostack  ns;
  double         cellBounds[6];
  vtkBoundingBox TestBox(bbox);
  //
  vtkCellTreeNode *n0 = &this->Tree->Nodes.front();
  // create a box for the root
  float *DataBBox = this->Tree->DataBBox;
  vtkBoundingBox lbox, rbox, rootbox(DataBBox[0], DataBBox[1], DataBBox[2], DataBBox[3], DataBBox[4], DataBBox[5]);
  ns.push(nodeBoxLevel(n0,boxLevel(rootbox,0)));
  while (!ns.empty())
    {
    n0 = ns.top().first;
    vtkBoundingBox &nodebox = ns.top().second.first;
    if (TestBox.Intersects(nodebox)) 
      {
      if (n0->IsLeaf())
        {
        for (int i=0; i<(int)n0->Size(); i++)
          {
          vtkIdType cell_ID = this->Tree->Leaves[n0->Start()+i];
          double *boundsPtr = cellBounds;
          if (this->CellBounds) 
            {
            boundsPtr = this->CellBounds[cell_ID];
            }
          else 
            {
            this->DataSet->GetCellBounds(cell_ID, boundsPtr);
            }
          vtkBoundingBox box(boundsPtr);
          if (TestBox.Intersects(box)) 
            {
            cells->InsertNextId(cell_ID);
            }
          }
        ns.pop();
        }
      else 
        {
        int lev = ns.top().second.second;
        SplitNodeBox(n0, nodebox, lbox, rbox);
        vtkCellTreeNode *n1 = &this->Tree->Nodes.at(n0->GetLeftChildIndex());
        vtkCellTreeNode *n2 = &this->Tree->Nodes.at(n0->GetLeftChildIndex()+1);
        ns.pop();
        ns.push(nodeBoxLevel(n1,boxLevel(lbox,lev+1)));
        ns.push(nodeBoxLevel(n2,boxLevel(rbox,lev+1)));
        }
      }
    else 
      {
      ns.pop();
      }
    }
}
//---------------------------------------------------------------------------

void vtkCellTreeLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
