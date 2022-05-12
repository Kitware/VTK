/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellTreeLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// VTK_DEPRECATED_IN_9_2_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkCellTreeLocator.h"

#include "vtkBoundingBox.h"
#include "vtkBox.h"
#include "vtkCellArray.h"
#include "vtkGenericCell.h"
#include "vtkIdListCollection.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stack>
#include <vector>

vtkStandardNewMacro(vtkCellTreeLocator);

namespace
{
enum
{
  POS_X,
  NEG_X,
  POS_Y,
  NEG_Y,
  POS_Z,
  NEG_Z
};
#define CELLTREE_MAX_DEPTH 32
}

//------------------------------------------------------------------------------
// This class is the basic building block of the cell tree.  There is a node per dimension. i.e.
// There are 3 vtkCellTreeNode in x,y,z directions.  In contrast, vtkModifiedBSPTree class stores
// the bounding planes for all 3 dimensions in a single node. LeftMax and rm defines the bounding
// planes. start is the location in the cell tree. e.g. for root node start is zero. size is the
// number of the nodes under the tree
inline void vtkCellTreeLocator::vtkCellTreeNode::MakeNode(vtkIdType left, vtkIdType d,
  double b[2]) // b is an array containing left max and right min values
{
  this->Index = (d & 3) | (left << 2);
  this->LeftMax = b[0];
  this->RightMin = b[1];
}

//------------------------------------------------------------------------------
inline void vtkCellTreeLocator::vtkCellTreeNode::SetChildren(vtkIdType left)
{
  this->Index = GetDimension() | (left << 2); // In index 2 LSBs (Least Significant Bits) store the
                                              // dimension. MSBs store the position
}

//------------------------------------------------------------------------------
inline bool vtkCellTreeLocator::vtkCellTreeNode::IsNode() const
{
  return (this->Index & 3) != 3; // For a leaf 2 LSBs in index is 3
}

//------------------------------------------------------------------------------
inline vtkIdType vtkCellTreeLocator::vtkCellTreeNode::GetLeftChildIndex() const
{
  return (this->Index >> 2);
}

//------------------------------------------------------------------------------
inline vtkIdType vtkCellTreeLocator::vtkCellTreeNode::GetRightChildIndex() const
{
  return (this->Index >> 2) +
    1; // Right child node is adjacent to the Left child node in the data structure
}

//------------------------------------------------------------------------------
inline vtkIdType vtkCellTreeLocator::vtkCellTreeNode::GetDimension() const
{
  return this->Index & 3;
}

//------------------------------------------------------------------------------
inline const double& vtkCellTreeLocator::vtkCellTreeNode::GetLeftMaxValue() const
{
  return this->LeftMax;
}

//------------------------------------------------------------------------------
inline const double& vtkCellTreeLocator::vtkCellTreeNode::GetRightMinValue() const
{
  return this->RightMin;
}

//------------------------------------------------------------------------------
inline void vtkCellTreeLocator::vtkCellTreeNode::MakeLeaf(vtkIdType start, vtkIdType size)
{
  this->Index = 3;
  this->Sz = size;
  this->St = start;
}

//------------------------------------------------------------------------------
bool vtkCellTreeLocator::vtkCellTreeNode::IsLeaf() const
{
  return this->Index == 3;
}

//------------------------------------------------------------------------------
vtkIdType vtkCellTreeLocator::vtkCellTreeNode::Start() const
{
  return this->St;
}

//------------------------------------------------------------------------------
vtkIdType vtkCellTreeLocator::vtkCellTreeNode::Size() const
{
  return this->Sz;
}

//------------------------------------------------------------------------------
// This is a helper class to traverse the cell tree. This is derived from avtCellLocatorBIH class in
// VisIT Member variables of this class starts with m_*
class vtkCellPointTraversal
{
private:
  const vtkCellTreeLocator::vtkCellTree& m_ct;
  vtkIdType m_stack[CELLTREE_MAX_DEPTH];
  vtkIdType* m_sp;     // stack pointer
  const double* m_pos; // 3-D coordinates of the points
  vtkCellPointTraversal(const vtkCellPointTraversal&) = delete;
  void operator=(vtkCellPointTraversal&) = delete;

protected:
  friend class vtkCellTreeLocator::vtkCellTree;
  friend class vtkCellTreeLocator::vtkCellTreeNode;
  friend class vtkCellTreeBuilder;

public:
  vtkCellPointTraversal(const vtkCellTreeLocator::vtkCellTree& ct, const double* pos)
    : m_ct(ct)
    , m_pos(pos)
  {
    this->m_stack[0] = 0;           // first element is set to zero
    this->m_sp = this->m_stack + 1; // this points to the second element of the stack
  }

  const vtkCellTreeLocator::vtkCellTreeNode*
  Next() // this returns n (the location in the CellTree) if it is a leaf or 0 if the point doesn't
         // contain in the data domain
  {
    while (true)
    {
      if (this->m_sp == this->m_stack) // This means the point is not within the domain
      {
        return nullptr;
      }

      const vtkCellTreeLocator::vtkCellTreeNode* n = &this->m_ct.Nodes.front() + *(--this->m_sp);

      if (n->IsLeaf())
      {
        return n;
      }

      const double p = m_pos[n->GetDimension()];
      const vtkIdType left = n->GetLeftChildIndex();

      bool l = p <= n->GetLeftMaxValue();  // Check if the points is within the left sub tree
      bool r = p >= n->GetRightMinValue(); // Check if the point is within the right sub tree

      if (l && r) //  This means if there is a overlap region both left and right sub trees should
                  //  be traversed
      {
        if (n->GetLeftMaxValue() - p < p - n->GetRightMinValue())
        {
          *(this->m_sp++) = left;
          *(this->m_sp++) = left + 1;
        }
        else
        {
          *(this->m_sp++) = left + 1;
          *(this->m_sp++) = left;
        }
      }
      else if (l)
      {
        *(this->m_sp++) = left;
      }
      else if (r)
      {
        *(this->m_sp++) = left + 1;
      }
    }
  }
};

//------------------------------------------------------------------------------
// This class builds the CellTree according to the algorithm given in the paper.
// This class is derived from the avtCellLocatorBIH class in VisIT.  Member variables of this class
// starts with m_*
class vtkCellTreeBuilder
{
private:
  struct Bucket
  {
    double Min;
    double Max;
    vtkIdType Cnt;

    Bucket()
    {
      Cnt = 0;
      Min = std::numeric_limits<double>::max();
      Max = -std::numeric_limits<double>::max();
    }

    void Add(const double _min, const double _max)
    {
      ++Cnt;

      if (_min < Min)
      {
        Min = _min;
      }

      if (_max > Max)
      {
        Max = _max;
      }
    }
  };

  struct PerCell
  {
    double Min[3];
    double Max[3];
    vtkIdType Ind;
  };

  struct CenterOrder
  {
    vtkIdType d;
    CenterOrder(vtkIdType _d)
      : d(_d)
    {
    }

    bool operator()(const PerCell& pc0, const PerCell& pc1)
    {
      return (pc0.Min[d] + pc0.Max[d]) < (pc1.Min[d] + pc1.Max[d]);
    }
  };

  struct LeftPredicate
  {
    vtkIdType d;
    double p;
    LeftPredicate(vtkIdType _d, double _p)
      : d(_d)
      , p(2.0f * _p)
    {
    }

    bool operator()(const PerCell& pc) { return (pc.Min[d] + pc.Max[d]) < p; }
  };

  // -------------------------------------------------------------------------

  void FindMinMax(const PerCell* begin, const PerCell* end, double* min, double* max)
  {
    if (begin == end)
    {
      return;
    }

    for (vtkIdType d = 0; d < 3; ++d)
    {
      min[d] = begin->Min[d];
      max[d] = begin->Max[d];
    }

    while (++begin != end)
    {
      for (vtkIdType d = 0; d < 3; ++d)
      {
        if (begin->Min[d] < min[d])
          min[d] = begin->Min[d];
        if (begin->Max[d] > max[d])
          max[d] = begin->Max[d];
      }
    }
  }

  // -------------------------------------------------------------------------

  void Split(vtkIdType index, double min[3], double max[3])
  {
    vtkIdType start = this->m_nodes[index].Start();
    vtkIdType size = this->m_nodes[index].Size();

    if (size < this->m_leafsize)
    {
      return;
    }

    PerCell* begin = &(this->m_pc[start]);
    PerCell* end = this->m_pc.data() + start + size;
    PerCell* mid = begin;

    const int nbuckets = 6;

    const double ext[3] = { max[0] - min[0], max[1] - min[1], max[2] - min[2] };
    const double iext[3] = { nbuckets / ext[0], nbuckets / ext[1], nbuckets / ext[2] };

    Bucket b[3][nbuckets];

    for (const PerCell* pc = begin; pc != end; ++pc)
    {
      for (vtkIdType d = 0; d < 3; ++d)
      {
        double cen = (pc->Min[d] + pc->Max[d]) / 2.0f;
        int ind = (int)((cen - min[d]) * iext[d]);

        if (ind < 0)
        {
          ind = 0;
        }

        if (ind >= nbuckets)
        {
          ind = nbuckets - 1;
        }

        b[d][ind].Add(pc->Min[d], pc->Max[d]);
      }
    }

    double cost = std::numeric_limits<double>::max();
    double plane = VTK_DOUBLE_MIN; // bad value in case it doesn't get setx
    vtkIdType dim = VTK_INT_MAX;   // bad value in case it doesn't get set

    for (vtkIdType d = 0; d < 3; ++d)
    {
      vtkIdType sum = 0;

      for (vtkIdType n = 0; n < (vtkIdType)nbuckets - 1; ++n)
      {
        double lmax = -std::numeric_limits<double>::max();
        double rmin = std::numeric_limits<double>::max();

        for (vtkIdType m = 0; m <= n; ++m)
        {
          if (b[d][m].Max > lmax)
          {
            lmax = b[d][m].Max;
          }
        }

        for (vtkIdType m = n + 1; m < (vtkIdType)nbuckets; ++m)
        {
          if (b[d][m].Min < rmin)
          {
            rmin = b[d][m].Min;
          }
        }

        //
        // JB : added if (...) to stop floating point error if rmin is unset
        // this happens when some buckets are empty (bad volume calc)
        //
        if (lmax != -std::numeric_limits<double>::max() &&
          rmin != std::numeric_limits<double>::max())
        {
          sum += b[d][n].Cnt;

          double lvol = (lmax - min[d]) / ext[d];
          double rvol = (max[d] - rmin) / ext[d];

          double c = lvol * sum + rvol * (size - sum);

          if (sum > 0 && sum < size && c < cost)
          {
            cost = c;
            dim = d;
            plane = min[d] + (n + 1) / iext[d];
          }
        }
      }
    }

    if (cost != std::numeric_limits<double>::max())
    {
      mid = std::partition(begin, end, LeftPredicate(dim, plane));
    }

    // fallback
    if (mid == begin || mid == end)
    {
      dim = std::max_element(ext, ext + 3) - ext;

      mid = begin + (end - begin) / 2;
      std::nth_element(begin, mid, end, CenterOrder(dim));
    }

    double lmin[3], lmax[3], rmin[3], rmax[3];

    FindMinMax(begin, mid, lmin, lmax);
    FindMinMax(mid, end, rmin, rmax);

    double clip[2] = { lmax[dim], rmin[dim] };

    vtkCellTreeLocator::vtkCellTreeNode child[2];
    child[0].MakeLeaf(begin - this->m_pc.data(), mid - begin);
    child[1].MakeLeaf(mid - this->m_pc.data(), end - mid);

    this->m_nodes[index].MakeNode((int)m_nodes.size(), dim, clip);
    this->m_nodes.insert(m_nodes.end(), child, child + 2);

    Split(this->m_nodes[index].GetLeftChildIndex(), lmin, lmax);
    Split(this->m_nodes[index].GetRightChildIndex(), rmin, rmax);
  }

public:
  vtkCellTreeBuilder()
  {
    this->m_buckets = 5;
    this->m_leafsize = 8;
  }

  void Build(vtkCellTreeLocator* ctl, vtkCellTreeLocator::vtkCellTree& ct, vtkDataSet* ds)
  {
    const vtkIdType numberOfCells = ds->GetNumberOfCells();
    this->m_pc.resize(static_cast<size_t>(numberOfCells));

    double min[3] = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
      std::numeric_limits<double>::max() };
    double max[3] = {
      -std::numeric_limits<double>::max(),
      -std::numeric_limits<double>::max(),
      -std::numeric_limits<double>::max(),
    };

    double cellBounds[6], *cellBoundsPtr;
    cellBoundsPtr = cellBounds;
    for (vtkIdType i = 0; i < numberOfCells; ++i)
    {
      this->m_pc[i].Ind = i;
      ctl->GetCellBounds(i, cellBoundsPtr);

      for (uint8_t d = 0; d < 3; ++d)
      {
        this->m_pc[i].Min[d] = cellBoundsPtr[2 * d + 0];
        this->m_pc[i].Max[d] = cellBoundsPtr[2 * d + 1];

        if (this->m_pc[i].Min[d] < min[d])
        {
          min[d] = this->m_pc[i].Min[d];
        }
        if (this->m_pc[i].Max[d] > max[d])
        {
          max[d] = this->m_pc[i].Max[d];
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
    root.MakeLeaf(0, numberOfCells);
    this->m_nodes.push_back(root);

    Split(0, min, max);

    ct.Nodes.resize(this->m_nodes.size());
    ct.Nodes[0] = this->m_nodes[0];

    for (auto ni = ct.Nodes.begin(), nn = ct.Nodes.begin() + 1; ni != ct.Nodes.end(); ++ni)
    {
      if (ni->IsLeaf())
      {
        continue;
      }

      *(nn++) = this->m_nodes[ni->GetLeftChildIndex()];
      *(nn++) = this->m_nodes[ni->GetRightChildIndex()];
      ni->SetChildren(nn - ct.Nodes.begin() - 2);
    }

    // --- final
    ct.Leaves.resize(static_cast<size_t>(numberOfCells));
    for (vtkIdType i = 0; i < numberOfCells; ++i)
    {
      ct.Leaves[i] = this->m_pc[i].Ind;
    }
    this->m_pc.clear();
  }

  vtkIdType m_buckets;
  vtkIdType m_leafsize;
  std::vector<PerCell> m_pc;
  std::vector<vtkCellTreeLocator::vtkCellTreeNode> m_nodes;
};

//------------------------------------------------------------------------------
typedef std::stack<vtkCellTreeLocator::vtkCellTreeNode*,
  std::vector<vtkCellTreeLocator::vtkCellTreeNode*>>
  nodeStack;

//------------------------------------------------------------------------------
vtkCellTreeLocator::vtkCellTreeLocator()
{
  this->NumberOfCellsPerNode = 8;
  this->NumberOfBuckets = 5;
  this->Tree = nullptr;
}

//------------------------------------------------------------------------------
vtkCellTreeLocator::~vtkCellTreeLocator()
{
  this->FreeSearchStructure();
  this->FreeCellBounds();
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::BuildLocator()
{
  // don't rebuild if build time is newer than modified and dataset modified time
  if (this->Tree.get() && this->BuildTime > this->MTime &&
    this->BuildTime > this->DataSet->GetMTime())
  {
    return;
  }
  // don't rebuild if UseExistingSearchStructure is ON and a search structure already exists
  if (this->Tree.get() && this->UseExistingSearchStructure)
  {
    this->BuildTime.Modified();
    vtkDebugMacro(<< "BuildLocator exited - UseExistingSearchStructure");
    return;
  }
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::ForceBuildLocator()
{
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::BuildLocatorInternal()
{
  if (!this->DataSet || (this->DataSet->GetNumberOfCells() < 1))
  {
    vtkErrorMacro(<< " No Cells in the data set\n");
    return;
  }
  this->FreeSearchStructure();
  if (this->CacheCellBounds)
  {
    this->FreeCellBounds();
    this->StoreCellBounds();
  }
  this->Tree = std::make_shared<vtkCellTree>();
  vtkCellTreeBuilder builder;
  builder.m_leafsize = this->NumberOfCellsPerNode;
  builder.m_buckets = this->NumberOfBuckets;
  builder.Build(this, *(this->Tree), this->DataSet);
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
vtkIdType vtkCellTreeLocator::FindCell(
  double pos[3], double, vtkGenericCell* cell, int& subId, double pcoords[3], double* weights)
{
  this->BuildLocator();
  if (this->Tree == nullptr)
  {
    return -1;
  }
  // check if pos outside of bounds
  if (!vtkAbstractCellLocator::IsInBounds(this->Tree->DataBBox, pos))
  {
    return -1;
  }

  double dist2;

  vtkCellPointTraversal pt(*(this->Tree), pos);
  while (const vtkCellTreeNode* n = pt.Next())
  {
    const vtkIdType* begin = &(this->Tree->Leaves[n->Start()]);
    const vtkIdType* end = begin + n->Size();

    for (; begin != end; ++begin)
    {
      if (this->InsideCellBounds(pos, *begin))
      {
        this->DataSet->GetCell(*begin, cell);
        if (cell->EvaluatePosition(pos, nullptr, subId, pcoords, dist2, weights) == 1)
        {
          return *begin;
        }
      }
    }
  }

  return -1;
}

//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
int vtkCellTreeLocator::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& t, double x[3], double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell)
{
  this->BuildLocator();
  if (this->Tree == nullptr)
  {
    return 0;
  }
  vtkCellTreeNode *node, *nearNode, *farNode;
  double tmin, tmax, tDist, tHitCell, tBest = VTK_DOUBLE_MAX, xBest[3], pCoordsBest[3];
  double rayDir[3], x0[3], x1[3], hitCellBoundsPosition[3];
  int plane0, plane1, subIdBest = -1;
  vtkMath::Subtract(p2, p1, rayDir);
  double* bounds = this->Tree->DataBBox;
  double cellBounds[6], *cellBoundsPtr;
  cellBoundsPtr = cellBounds;
  vtkIdType cId, cellIdBest = -1;
  cellId = -1;

  // Does ray pass through root BBox
  if (vtkBox::IntersectWithLine(bounds, p1, p2, tmin, tmax, x0, x1, plane0, plane1) == 0)
  {
    return 0; // No intersections possible, line is outside the locator
  }

  // Initialize intersection query array if necessary. This is done
  // locally to ensure thread safety.
  std::vector<bool> cellHasBeenVisited(this->DataSet->GetNumberOfCells(), false);

  // Ok, setup a stack and various params
  nodeStack ns;
  // setup our axis optimized ray box edge stuff
  int axis = getDominantAxis(rayDir);
  double (*_getMinDist)(const double origin[3], const double dir[3], const double B[6]);
  switch (axis)
  {
    case POS_X:
      _getMinDist = _getMinDistPOS_X;
      break;
    case NEG_X:
      _getMinDist = _getMinDistNEG_X;
      break;
    case POS_Y:
      _getMinDist = _getMinDistPOS_Y;
      break;
    case NEG_Y:
      _getMinDist = _getMinDistNEG_Y;
      break;
    case POS_Z:
      _getMinDist = _getMinDistPOS_Z;
      break;
    default:
      _getMinDist = _getMinDistNEG_Z;
      break;
  }

  // OK, lets walk the tree and find intersections
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

    int mustCheck = 0; // to check if both left and right sub trees need to be checked

    while (!node->IsLeaf())
    { // this must be a parent node
      // Which child node is closest to ray origin - given direction
      Classify(p1, rayDir, tDist, nearNode, node, farNode, mustCheck);
      // if the distance to the farNode edge of the nearNode box is > tmax, no need to test farNode
      // box (we still need to test Mid because it may overlap slightly)
      if (mustCheck)
      {
        ns.push(farNode);
        node = nearNode;
      }
      else if ((tDist > tmax) || (tDist <= 0))
      { // <=0 for ray on edge
        node = nearNode;
      }
      // if the distance to the farNode edge of the nearNode box is < tmin, no need to test nearNode
      // box
      else if (tDist < tmin)
      {
        ns.push(nearNode);
        node = farNode;
      }
      // All the child nodes may be candidates, keep nearNode, push farNode then mid
      else
      {
        ns.push(farNode);
        node = nearNode;
      }
    }
    // Ok, so we're a leaf node, first check the BBox against the ray
    // then test the candidates in our sorted ray direction order
    for (vtkIdType i = 0; i < node->Size(); i++)
    {
      cId = this->Tree->Leaves[node->Start() + i];
      if (!cellHasBeenVisited[cId])
      {
        cellHasBeenVisited[cId] = true;

        this->GetCellBounds(cId, cellBoundsPtr);
        if (_getMinDist(p1, rayDir, cellBoundsPtr) > tBest)
        {
          break;
        }
        // check whether we intersect the cell bounds
        int hitCellBounds =
          vtkBox::IntersectBox(cellBoundsPtr, p1, rayDir, hitCellBoundsPosition, tHitCell, tol);

        if (hitCellBounds)
        {
          this->DataSet->GetCell(cId, cell);
          if (cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId))
          {
            if (t < tBest)
            {
              tBest = t;
              xBest[0] = x[0];
              xBest[1] = x[1];
              xBest[2] = x[2];
              pCoordsBest[0] = pcoords[0];
              pCoordsBest[1] = pcoords[1];
              pCoordsBest[2] = pcoords[2];
              subIdBest = subId;
              cellIdBest = cId;
            }
          }
        }
      }
    }
  }
  // If a cell has been intersected, recover the information and return.
  if (cellIdBest >= 0)
  {
    this->DataSet->GetCell(cellIdBest, cell);
    t = tBest;
    x[0] = xBest[0];
    x[1] = xBest[1];
    x[2] = xBest[2];
    pcoords[0] = pCoordsBest[0];
    pcoords[1] = pCoordsBest[1];
    pcoords[2] = pCoordsBest[2];
    subId = subIdBest;
    cellId = cellIdBest;
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
struct IntersectionInfo
{
  vtkIdType CellId;
  std::array<double, 3> IntersectionPoint;
  double T;

  IntersectionInfo(vtkIdType cellId, double x[3], double t)
    : CellId(cellId)
    , IntersectionPoint({ x[0], x[1], x[2] })
    , T(t)
  {
  }
};

//------------------------------------------------------------------------------
int vtkCellTreeLocator::IntersectWithLine(const double p1[3], const double p2[3], const double tol,
  vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell)
{
  this->BuildLocator();
  if (this->Tree == nullptr)
  {
    return 0;
  }
  // Initialize the list of points/cells
  if (points)
  {
    points->Reset();
  }
  if (cellIds)
  {
    cellIds->Reset();
  }
  vtkCellTreeNode *node, *nearNode, *farNode;
  double tmin, tmax, tDist, tHitCell;
  double rayDir[3], x0[3], x1[3], hitCellBoundsPosition[3];
  int plane0, plane1, subId, hitCellBounds;
  vtkMath::Subtract(p2, p1, rayDir);
  double* bounds = this->Tree->DataBBox;
  double cellBounds[6], *cellBoundsPtr;
  cellBoundsPtr = cellBounds;
  vtkIdType cId;
  double t, x[3], pcoords[3];

  // Does ray pass through root BBox
  if (vtkBox::IntersectWithLine(bounds, p1, p2, tmin, tmax, x0, x1, plane0, plane1) == 0)
  {
    return 0; // No intersections possible, line is outside the locator
  }

  // Initialize intersection query array if necessary. This is done
  // locally to ensure thread safety.
  std::vector<bool> cellHasBeenVisited(this->DataSet->GetNumberOfCells(), false);

  // Ok, setup a stack and various params
  nodeStack ns;

  // we will sort intersections by t, so keep track using these lists
  std::vector<IntersectionInfo> cellIntersections;
  // OK, lets walk the tree and find intersections
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

    int mustCheck = 0; // to check if both left and right sub trees need to be checked

    //
    while (!node->IsLeaf())
    { // this must be a parent node
      // Which child node is closest to ray origin - given direction
      Classify(p1, rayDir, tDist, nearNode, node, farNode, mustCheck);
      // if the distance to the farNode edge of the nearNode box is > tmax, no need to test farNode
      // box (we still need to test Mid because it may overlap slightly)
      if (mustCheck)
      {
        ns.push(farNode);
        node = nearNode;
      }
      else if ((tDist > tmax) || (tDist <= 0))
      { // <=0 for ray on edge
        node = nearNode;
      }
      // if the distance to the farNode edge of the nearNode box is < tmin, no need to test nearNode
      // box
      else if (tDist < tmin)
      {
        ns.push(nearNode);
        node = farNode;
      }
      // All the child nodes may be candidates, keep nearNode, push farNode then mid
      else
      {
        ns.push(farNode);
        node = nearNode;
      }
    }
    // Ok, so we're a leaf node, first check the BBox against the ray
    // then test the candidates in our sorted ray direction order
    for (vtkIdType i = 0; i < node->Size(); i++)
    {
      cId = this->Tree->Leaves[node->Start() + i];
      if (!cellHasBeenVisited[cId])
      {
        cellHasBeenVisited[cId] = true;

        // check whether we intersect the cell bounds
        this->GetCellBounds(cId, cellBoundsPtr);
        hitCellBounds =
          vtkBox::IntersectBox(cellBoundsPtr, p1, rayDir, hitCellBoundsPosition, tHitCell, tol);

        if (hitCellBounds)
        {
          // Note because of cellHasBeenVisited[], we know this cId is unique
          if (cell)
          {
            this->DataSet->GetCell(cId, cell);
            if (cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId))
            {
              cellIntersections.emplace_back(cId, x, t);
            }
          }
          else
          {
            cellIntersections.emplace_back(cId, hitCellBoundsPosition, tHitCell);
          }
        }
      }
    }
  }
  // if we had intersections, sort them by increasing t
  if (!cellIntersections.empty())
  {
    vtkIdType numIntersections = static_cast<vtkIdType>(cellIntersections.size());
    std::sort(cellIntersections.begin(), cellIntersections.end(),
      [&](const IntersectionInfo& a, const IntersectionInfo b) { return a.T < b.T; });
    if (points)
    {
      points->SetNumberOfPoints(numIntersections);
      for (vtkIdType i = 0; i < numIntersections; ++i)
      {
        points->SetPoint(i, cellIntersections[i].IntersectionPoint.data());
      }
    }
    if (cellIds)
    {
      cellIds->SetNumberOfIds(numIntersections);
      for (vtkIdType i = 0; i < numIntersections; ++i)
      {
        cellIds->SetId(i, cellIntersections[i].CellId);
      }
    }
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkCellTreeLocator::getDominantAxis(const double dir[3])
{
  double tX = (dir[0] > 0) ? dir[0] : -dir[0];
  double tY = (dir[1] > 0) ? dir[1] : -dir[1];
  double tZ = (dir[2] > 0) ? dir[2] : -dir[2];
  if (tX > tY && tX > tZ)
  {
    return ((dir[0] > 0) ? POS_X : NEG_X);
  }
  else if (tY > tZ)
  {
    return ((dir[1] > 0) ? POS_Y : NEG_Y);
  }
  else
  {
    return ((dir[2] > 0) ? POS_Z : NEG_Z);
  }
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::Classify(const double origin[3], const double dir[3], double& rDist,
  vtkCellTreeNode*& nearNode, vtkCellTreeNode*& parent, vtkCellTreeNode*& farNode, int& mustCheck)
{
  double tOriginToDivPlane = parent->GetLeftMaxValue() - origin[parent->GetDimension()];
  double tOriginToDivPlane2 = parent->GetRightMinValue() - origin[parent->GetDimension()];
  double tDivDirection = dir[parent->GetDimension()];
  if (tOriginToDivPlane2 > 0) // origin is right of the rmin
  {
    nearNode = &this->Tree->Nodes.at(parent->GetLeftChildIndex());
    farNode = &this->Tree->Nodes.at(parent->GetLeftChildIndex() + 1);
    rDist = (tDivDirection) ? tOriginToDivPlane2 / tDivDirection : VTK_DOUBLE_MAX;
  }
  else if (tOriginToDivPlane < 0) // origin is left of the lm
  {
    farNode = &this->Tree->Nodes.at(parent->GetLeftChildIndex());
    nearNode = &this->Tree->Nodes.at(parent->GetLeftChildIndex() + 1);
    rDist = (tDivDirection) ? tOriginToDivPlane / tDivDirection : VTK_DOUBLE_MAX;
  }

  else
  {
    if (tOriginToDivPlane > 0 && tOriginToDivPlane2 < 0)
    {
      mustCheck = 1; // The point is within right min and left max. both left and right subtrees
                     // must be checked
    }

    if (tDivDirection < 0)
    {
      nearNode = &this->Tree->Nodes.at(parent->GetLeftChildIndex());
      farNode = &this->Tree->Nodes.at(parent->GetLeftChildIndex() + 1);
      if (!(tOriginToDivPlane > 0 || tOriginToDivPlane < 0))
      {
        mustCheck = 1; // Ray was exactly on edge left max box.
      }
      rDist = (tDivDirection) ? 0 / tDivDirection : VTK_DOUBLE_MAX;
    }
    else
    {
      farNode = &this->Tree->Nodes.at(parent->GetLeftChildIndex());
      nearNode = &this->Tree->Nodes.at(parent->GetLeftChildIndex() + 1);
      if (!(tOriginToDivPlane2 > 0 || tOriginToDivPlane2 < 0))
      {
        mustCheck = 1; // Ray was exactly on edge right min box.
      }
      rDist = (tDivDirection) ? 0 / tDivDirection : VTK_DOUBLE_MAX;
    }
  }
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::FreeSearchStructure()
{
  this->Tree.reset();
}

//------------------------------------------------------------------------------
// For drawing coloured boxes, we want the level/depth
typedef std::pair<vtkBoundingBox, int> boxLevel;
typedef std::vector<boxLevel> boxlist;
// For testing bounds of the tree we need node/box
typedef std::pair<vtkCellTreeLocator::vtkCellTreeNode*, boxLevel> nodeBoxLevel;
typedef std::stack<nodeBoxLevel, std::vector<nodeBoxLevel>> nodeinfostack;

//------------------------------------------------------------------------------
static void SplitNodeBox(
  vtkCellTreeLocator::vtkCellTreeNode* n, vtkBoundingBox& b, vtkBoundingBox& l, vtkBoundingBox& r)
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

//------------------------------------------------------------------------------
static void AddBox(vtkPolyData* pd, double* bounds, int level)
{
  vtkPoints* pts = pd->GetPoints();
  vtkCellArray* lines = pd->GetLines();
  vtkIntArray* levels = vtkArrayDownCast<vtkIntArray>(pd->GetPointData()->GetArray(0));
  double x[3];
  vtkIdType cells[8], ids[2];
  //
  x[0] = bounds[0];
  x[1] = bounds[2];
  x[2] = bounds[4];
  cells[0] = pts->InsertNextPoint(x);
  x[0] = bounds[1];
  x[1] = bounds[2];
  x[2] = bounds[4];
  cells[1] = pts->InsertNextPoint(x);
  x[0] = bounds[0];
  x[1] = bounds[3];
  x[2] = bounds[4];
  cells[2] = pts->InsertNextPoint(x);
  x[0] = bounds[1];
  x[1] = bounds[3];
  x[2] = bounds[4];
  cells[3] = pts->InsertNextPoint(x);
  x[0] = bounds[0];
  x[1] = bounds[2];
  x[2] = bounds[5];
  cells[4] = pts->InsertNextPoint(x);
  x[0] = bounds[1];
  x[1] = bounds[2];
  x[2] = bounds[5];
  cells[5] = pts->InsertNextPoint(x);
  x[0] = bounds[0];
  x[1] = bounds[3];
  x[2] = bounds[5];
  cells[6] = pts->InsertNextPoint(x);
  x[0] = bounds[1];
  x[1] = bounds[3];
  x[2] = bounds[5];
  cells[7] = pts->InsertNextPoint(x);
  //
  ids[0] = cells[0];
  ids[1] = cells[1];
  lines->InsertNextCell(2, ids);
  ids[0] = cells[2];
  ids[1] = cells[3];
  lines->InsertNextCell(2, ids);
  ids[0] = cells[4];
  ids[1] = cells[5];
  lines->InsertNextCell(2, ids);
  ids[0] = cells[6];
  ids[1] = cells[7];
  lines->InsertNextCell(2, ids);
  ids[0] = cells[0];
  ids[1] = cells[2];
  lines->InsertNextCell(2, ids);
  ids[0] = cells[1];
  ids[1] = cells[3];
  lines->InsertNextCell(2, ids);
  ids[0] = cells[4];
  ids[1] = cells[6];
  lines->InsertNextCell(2, ids);
  ids[0] = cells[5];
  ids[1] = cells[7];
  lines->InsertNextCell(2, ids);
  ids[0] = cells[0];
  ids[1] = cells[4];
  lines->InsertNextCell(2, ids);
  ids[0] = cells[1];
  ids[1] = cells[5];
  lines->InsertNextCell(2, ids);
  ids[0] = cells[2];
  ids[1] = cells[6];
  lines->InsertNextCell(2, ids);
  ids[0] = cells[3];
  ids[1] = cells[7];
  lines->InsertNextCell(2, ids);
  //
  // Colour boxes by scalar if array present
  //
  for (int i = 0; levels && i < 8; i++)
  {
    levels->InsertNextTuple1(level);
  }
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::GenerateRepresentation(int level, vtkPolyData* pd)
{
  this->BuildLocator();
  if (this->Tree == nullptr)
  {
    return;
  }
  nodeinfostack ns;
  boxlist bl;
  //
  vtkCellTreeNode* n0 = &this->Tree->Nodes.front();
  // create a box for the root
  double* DataBBox = this->Tree->DataBBox;
  vtkBoundingBox lbox, rbox,
    rootbox(DataBBox[0], DataBBox[1], DataBBox[2], DataBBox[3], DataBBox[4], DataBBox[5]);
  ns.push(nodeBoxLevel(n0, boxLevel(rootbox, 0)));
  while (!ns.empty())
  {
    n0 = ns.top().first;
    int lev = ns.top().second.second;
    if (n0->IsLeaf() && ((lev == level) || (level == -1)))
    {
      bl.push_back(boxLevel(ns.top().second.first, lev));
      ns.pop();
    }
    else if (n0->IsLeaf())
    {
      ns.pop();
    }
    else if (n0->IsNode())
    {
      SplitNodeBox(n0, ns.top().second.first, lbox, rbox);
      vtkCellTreeNode* n1 = &this->Tree->Nodes.at(n0->GetLeftChildIndex());
      vtkCellTreeNode* n2 = &this->Tree->Nodes.at(n0->GetLeftChildIndex() + 1);
      ns.pop();
      ns.push(nodeBoxLevel(n1, boxLevel(lbox, lev + 1)));
      ns.push(nodeBoxLevel(n2, boxLevel(rbox, lev + 1)));
    }
  }
  // For each node, add the bbox to our polydata
  for (auto i = 0; i < bl.size(); i++)
  {
    double bounds[6];
    bl[i].first.GetBounds(bounds);
    AddBox(pd, bounds, bl[i].second);
  }
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::FindCellsWithinBounds(double* bbox, vtkIdList* cells)
{
  this->BuildLocator();
  if (this->Tree == nullptr)
  {
    return;
  }
  nodeinfostack ns;
  double cellBounds[6], *cellBoundsPtr;
  cellBoundsPtr = cellBounds;
  vtkBoundingBox TestBox(bbox);

  vtkCellTreeNode* n0 = &this->Tree->Nodes.front();
  // create a box for the root
  double* DataBBox = this->Tree->DataBBox;
  vtkBoundingBox lbox, rbox,
    rootbox(DataBBox[0], DataBBox[1], DataBBox[2], DataBBox[3], DataBBox[4], DataBBox[5]);
  ns.push(nodeBoxLevel(n0, boxLevel(rootbox, 0)));
  while (!ns.empty())
  {
    n0 = ns.top().first;
    vtkBoundingBox& nodebox = ns.top().second.first;
    if (TestBox.Intersects(nodebox))
    {
      if (n0->IsLeaf())
      {
        for (int i = 0; i < (int)n0->Size(); i++)
        {
          vtkIdType cell_ID = this->Tree->Leaves[n0->Start() + i];
          this->GetCellBounds(cell_ID, cellBoundsPtr);
          vtkBoundingBox box(cellBoundsPtr);
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
        vtkCellTreeNode* n1 = &this->Tree->Nodes.at(n0->GetLeftChildIndex());
        vtkCellTreeNode* n2 = &this->Tree->Nodes.at(n0->GetLeftChildIndex() + 1);
        ns.pop();
        ns.push(nodeBoxLevel(n1, boxLevel(lbox, lev + 1)));
        ns.push(nodeBoxLevel(n2, boxLevel(rbox, lev + 1)));
      }
    }
    else
    {
      ns.pop();
    }
  }
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::ShallowCopy(vtkAbstractCellLocator* locator)
{
  vtkCellTreeLocator* cellLocator = vtkCellTreeLocator::SafeDownCast(locator);
  if (!cellLocator)
  {
    vtkErrorMacro("Cannot cast " << locator->GetClassName() << " to vtkCellTreeLocator.");
    return;
  }
  // we only copy what's actually used by vtkCellTreeLocator

  // vtkLocator parameters
  this->SetDataSet(cellLocator->GetDataSet());
  this->SetUseExistingSearchStructure(cellLocator->GetUseExistingSearchStructure());

  // vtkAbstractCellLocator parameters
  this->SetNumberOfCellsPerNode(cellLocator->GetNumberOfCellsPerNode());
  this->CacheCellBounds = cellLocator->CacheCellBounds;
  this->CellBoundsSharedPtr = cellLocator->CellBoundsSharedPtr; // This is important
  this->CellBounds = this->CellBoundsSharedPtr.get() ? this->CellBoundsSharedPtr->data() : nullptr;

  // vtkCellTreeLocator parameters
  this->NumberOfBuckets = cellLocator->NumberOfBuckets;
  this->Tree = cellLocator->Tree;
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
