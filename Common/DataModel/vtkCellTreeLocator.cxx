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
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

#include <algorithm>
#include <array>
#include <stack>
#include <vector>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkCellTreeLocator);

//------------------------------------------------------------------------------
namespace detail
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
#define CELLTREE_MAX_DEPTH 64

//------------------------------------------------------------------------------
// Perform locator operations like FindCell. Uses templated subclasses
// to reduce memory and enhance speed.
struct vtkCellTree
{
  double DataBBox[6]; // This store the bounding values of the dataset
  vtkCellTreeLocator* Locator;
  vtkDataSet* DataSet;

  vtkCellTree(vtkCellTreeLocator* locator)
    : Locator(locator)
    , DataSet(locator->GetDataSet())
  {
  }

  virtual ~vtkCellTree() = default;

  // Satisfy cell locator API
  virtual vtkIdType FindCell(
    double pos[3], vtkGenericCell* cell, int& subId, double pcoords[3], double* weights) = 0;
  virtual void FindCellsWithinBounds(double* bbox, vtkIdList* cells) = 0;
  virtual int IntersectWithLine(const double a0[3], const double a1[3], double tol, double& t,
    double x[3], double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell) = 0;
  virtual int IntersectWithLine(const double p1[3], const double p2[3], double tol,
    vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell) = 0;
  virtual void GenerateRepresentation(int level, vtkPolyData* pd) = 0;

  // Utility methods
  static int getDominantAxis(const double dir[3])
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
  inline static double _getMinDistPOS_X(
    const double origin[3], const double dir[3], const double B[6])
  {
    return ((B[0] - origin[0]) / dir[0]);
  }
  inline static double _getMinDistNEG_X(
    const double origin[3], const double dir[3], const double B[6])
  {
    return ((B[1] - origin[0]) / dir[0]);
  }
  inline static double _getMinDistPOS_Y(
    const double origin[3], const double dir[3], const double B[6])
  {
    return ((B[2] - origin[1]) / dir[1]);
  }
  inline static double _getMinDistNEG_Y(
    const double origin[3], const double dir[3], const double B[6])
  {
    return ((B[3] - origin[1]) / dir[1]);
  }
  inline static double _getMinDistPOS_Z(
    const double origin[3], const double dir[3], const double B[6])
  {
    return ((B[4] - origin[2]) / dir[2]);
  }
  inline static double _getMinDistNEG_Z(
    const double origin[3], const double dir[3], const double B[6])
  {
    return ((B[5] - origin[2]) / dir[2]);
  }
};

/**
 * This struct is the basic building block of the cell tree.
 * Nodes consist of two split planes, LeftMax and RightMin,
 * one which holds all cells assigned to the left, one for the right.
 * The planes may overlap in the box, but cells are only assigned
 * to one side, so some searches must traverse both leaves until they have eliminated
 * candidates. start is the location in the cell tree. e.g. for root node start is zero.
 * size is the number of the nodes under the (sub-)tree
 */
template <typename T>
struct CellTreeNode
{
  double LeftMax;  // left max value
  double RightMin; // right min value
  T Index;         // index of the cell
  T Sz;            // size
  T St;            // start

  /**
   * b is an array containing left max and right min values
   */
  inline void MakeNode(const T& left, const T& d, const double b[2])
  {
    this->Index = (d & 3) | (left << 2);
    this->LeftMax = b[0];
    this->RightMin = b[1];
  }
  inline void SetChildren(const T& left)
  {
    // In index 2 LSBs (Least Significant Bits) store the dimension. MSBs store the position
    this->Index = this->GetDimension() | (left << 2);
  }
  inline bool IsNode() const
  {
    return (this->Index & 3) != 3; // For a leaf 2 LSBs in index is 3
  }
  inline T GetLeftChildIndex() const { return (this->Index >> 2); }
  inline T GetRightChildIndex() const
  {
    // Right child node is adjacent to the Left child node in the data structure
    return (this->Index >> 2) + 1;
  }
  inline T GetDimension() const { return this->Index & 3; }
  inline const double& GetLeftMaxValue() const { return this->LeftMax; }
  inline const double& GetRightMinValue() const { return this->RightMin; }
  inline void MakeLeaf(const T& start, const T& size)
  {
    this->Index = 3;
    this->Sz = size;
    this->St = start;
  }
  bool IsLeaf() const { return this->Index == 3; }
  const T& Start() const { return this->St; }
  const T& Size() const { return this->Sz; }
};

//------------------------------------------------------------------------------
template <typename T>
struct CellTree : public vtkCellTree
{
  using TCellTreeNode = CellTreeNode<T>;
  using TreeNodeStack = std::stack<TCellTreeNode*, std::vector<TCellTreeNode*>>;

  // For drawing coloured boxes, we want the level/depth
  using BoxLevel = std::pair<vtkBoundingBox, int>;
  using BoxList = std::vector<BoxLevel>;
  // For testing bounds of the tree we need node/box
  typedef std::pair<TCellTreeNode*, BoxLevel> NodeBoxLevel;
  typedef std::stack<NodeBoxLevel, std::vector<NodeBoxLevel>> NodeInfoStack;

  std::vector<TCellTreeNode> Nodes;
  std::vector<T> Leaves;

  CellTree(vtkCellTreeLocator* locator)
    : vtkCellTree(locator)
  {
  }

  ~CellTree() override
  {
    this->Nodes.clear();
    this->Leaves.clear();
  }

  void Classify(const double origin[3], const double dir[3], double& rDist,
    CellTreeNode<T>*& nearNode, TCellTreeNode*& parent, TCellTreeNode*& farNode, int& mustCheck);

  // Methods to satisfy vtkCellProcessor virtual API
  vtkIdType FindCell(
    double pos[3], vtkGenericCell* cell, int& subId, double pcoords[3], double* weights) override;
  void FindCellsWithinBounds(double* bbox, vtkIdList* cells) override;
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell) override;
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, vtkPoints* points,
    vtkIdList* cellIds, vtkGenericCell* cell) override;
  void GenerateRepresentation(int level, vtkPolyData* pd) override;
};

//------------------------------------------------------------------------------
// This is a helper struct to traverse the cell tree.
// This is derived from avtCellLocatorBIH struct in VisIT.
template <typename T>
struct CellPointTraversal
{
  using TCellTree = CellTree<T>;
  using TCellTreeNode = typename TCellTree::TCellTreeNode;
  const TCellTree& Tree;
  T Stack[CELLTREE_MAX_DEPTH];
  T* StackPtr;       // stack pointer
  const double* Pos; // 3-D coordinates of the points

  CellPointTraversal(const CellPointTraversal&) = delete;
  void operator=(CellPointTraversal&) = delete;

  CellPointTraversal(const CellTree<T>& tree, const double* pos)
    : Tree(tree)
    , Pos(pos)
  {
    this->Stack[0] = 0;               // first element is set to zero
    this->StackPtr = this->Stack + 1; // this points to the second element of the stack
  }

  // this returns n (the location in the CellTree) if it is a leaf or 0 if the point doesn't
  // contain in the data domain
  const CellTreeNode<T>* Next()
  {
    while (true)
    {
      if (this->StackPtr == this->Stack) // This means the point is not within the domain
      {
        return nullptr;
      }

      const TCellTreeNode* n = &this->Tree.Nodes.front() + *(--this->StackPtr);

      if (n->IsLeaf())
      {
        return n;
      }

      const double p = this->Pos[n->GetDimension()];
      const T left = n->GetLeftChildIndex();

      bool l = p <= n->GetLeftMaxValue();  // Check if the points is within the left subtree
      bool r = p >= n->GetRightMinValue(); // Check if the point is within the right subtree

      //  This means if there is an overlap region both left and right subtrees should
      //  be traversed
      if (l && r)
      {
        if (n->GetLeftMaxValue() - p < p - n->GetRightMinValue())
        {
          *(this->StackPtr++) = left;
          *(this->StackPtr++) = left + 1;
        }
        else
        {
          *(this->StackPtr++) = left + 1;
          *(this->StackPtr++) = left;
        }
      }
      else if (l)
      {
        *(this->StackPtr++) = left;
      }
      else if (r)
      {
        *(this->StackPtr++) = left + 1;
      }
    }
  }
};

//------------------------------------------------------------------------------
// This class builds the CellTree according to the algorithm given in the paper.
// This class is derived from the avtCellLocatorBIH class in VisIT.
template <typename T>
struct CellTreeBuilder
{
private:
  struct Bucket
  {
    double Min;
    double Max;
    T Cnt;

    Bucket()
      : Min(VTK_DOUBLE_MAX)
      , Max(-VTK_DOUBLE_MAX)
      , Cnt(0)
    {
    }

    inline void Add(const double& min, const double& max)
    {
      ++this->Cnt;
      if (min < this->Min)
      {
        this->Min = min;
      }
      if (max > this->Max)
      {
        this->Max = max;
      }
    }
  };

  struct CellInfo
  {
    double Min[3];
    double Max[3];
    T Ind;
  };

  struct CenterOrder
  {
    T D;
    CenterOrder(const T& d)
      : D(d)
    {
    }

    inline bool operator()(const CellInfo& pc0, const CellInfo& pc1)
    {
      return (pc0.Min[this->D] + pc0.Max[this->D]) < (pc1.Min[this->D] + pc1.Max[this->D]);
    }
  };

  struct LeftPredicate
  {
    T D;
    double P;
    LeftPredicate(const T& d, const double& p)
      : D(d)
      , P(2.0 * p)
    {
    }

    inline bool operator()(const CellInfo& pc)
    {
      return pc.Min[this->D] + pc.Max[this->D] < this->P;
    }
  };

  struct SplitInfo
  {
    T Index;
    double Min[3];
    double Max[3];

    SplitInfo(const T& index, const double* min, const double* max)
      : Index(index)
    {
      memcpy(this->Min, min, sizeof(double) * 3);
      memcpy(this->Max, max, sizeof(double) * 3);
    }
  };

  using TCellTree = CellTree<T>;
  using TCellTreeNode = typename TCellTree::TCellTreeNode;

  vtkCellTreeLocator* Locator;
  TCellTree& Tree;
  vtkDataSet* DataSet;
  int NumberOfBuckets;
  int NumberOfNodesPerLeaf;

  std::vector<CellInfo> CellsInfo;
  std::vector<CellTreeNode<T>> Nodes;
  std::stack<SplitInfo> SplitStack;

  struct BucketsType : public std::array<std::vector<Bucket>, 3>
  {
    BucketsType() = default;

    BucketsType(int numBuckets)
    {
      size_t numberOfBuckets = static_cast<size_t>(numBuckets);
      (*this)[0].resize(numberOfBuckets);
      (*this)[1].resize(numberOfBuckets);
      (*this)[2].resize(numberOfBuckets);
    }

    void Reset()
    {
      std::fill((*this)[0].begin(), (*this)[0].end(), Bucket());
      std::fill((*this)[1].begin(), (*this)[1].end(), Bucket());
      std::fill((*this)[2].begin(), (*this)[2].end(), Bucket());
    }
  };
  BucketsType Buckets;

  // -------------------------------------------------------------------------
  void FindMinMax(const CellInfo* begin, const CellInfo* end, double* min, double* max)
  {
    if (begin == end)
    {
      return;
    }

    for (uint8_t d = 0; d < 3; ++d)
    {
      min[d] = begin->Min[d];
      max[d] = begin->Max[d];
    }

    while (++begin != end)
    {
      for (uint8_t d = 0; d < 3; ++d)
      {
        if (begin->Min[d] < min[d])
        {
          min[d] = begin->Min[d];
        }
        if (begin->Max[d] > max[d])
        {
          max[d] = begin->Max[d];
        }
      }
    }
  }

  // -------------------------------------------------------------------------
  void Split(T index, double min[3], double max[3], BucketsType& buckets)
  {
    const T& start = this->Nodes[index].Start();
    const T& size = this->Nodes[index].Size();

    if (size < this->NumberOfNodesPerLeaf)
    {
      return;
    }

    CellInfo* begin = &(this->CellsInfo[start]);
    CellInfo* end = this->CellsInfo.data() + start + size;
    CellInfo* mid = begin;

    const double ext[3] = { max[0] - min[0], max[1] - min[1], max[2] - min[2] };
    const double iext[3] = { this->NumberOfBuckets / ext[0], this->NumberOfBuckets / ext[1],
      this->NumberOfBuckets / ext[2] };

    buckets.Reset();

    double cen;
    int ind;

    for (const CellInfo* pc = begin; pc != end; ++pc)
    {
      for (uint8_t d = 0; d < 3; ++d)
      {
        cen = (pc->Min[d] + pc->Max[d]) / 2.0;
        ind = (int)((cen - min[d]) * iext[d]);

        if (ind < 0)
        {
          ind = 0;
        }

        if (ind >= this->NumberOfBuckets)
        {
          ind = this->NumberOfBuckets - 1;
        }

        buckets[d][ind].Add(pc->Min[d], pc->Max[d]);
      }
    }

    double cost = VTK_DOUBLE_MAX;
    double plane = VTK_DOUBLE_MIN; // bad value in case it doesn't get setx
    T dim = VTK_INT_MAX;           // bad value in case it doesn't get set
    T sum;
    double lVol, rVol, c, lMaxValue, rMinValue;
    int n, m;

    for (uint8_t d = 0; d < 3; ++d)
    {
      sum = 0;

      for (n = 0; n < this->NumberOfBuckets - 1; ++n)
      {
        lMaxValue = -VTK_DOUBLE_MAX;
        rMinValue = VTK_DOUBLE_MAX;

        for (m = 0; m <= n; ++m)
        {
          if (buckets[d][m].Max > lMaxValue)
          {
            lMaxValue = buckets[d][m].Max;
          }
        }

        for (m = n + 1; m < this->NumberOfBuckets; ++m)
        {
          if (buckets[d][m].Min < rMinValue)
          {
            rMinValue = buckets[d][m].Min;
          }
        }

        // JB : added if (...) to stop floating point error if rmin is unset
        // this happens when some buckets are empty (bad volume calc)
        if (lMaxValue != -VTK_DOUBLE_MAX && rMinValue != VTK_DOUBLE_MAX)
        {
          sum += buckets[d][n].Cnt;

          lVol = (lMaxValue - min[d]) / ext[d];
          rVol = (max[d] - rMinValue) / ext[d];

          c = lVol * sum + rVol * (size - sum);

          if (sum > 0 && sum < size && c < cost)
          {
            cost = c;
            dim = d;
            plane = min[d] + (n + 1) / iext[d];
          }
        }
      }
    }

    if (cost != VTK_DOUBLE_MAX)
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

    double lMin[3], lMax[3], rMin[3], rMax[3];

    this->FindMinMax(begin, mid, lMin, lMax);
    this->FindMinMax(mid, end, rMin, rMax);

    double clip[2] = { lMax[dim], rMin[dim] };

    TCellTreeNode child[2];
    child[0].MakeLeaf(begin - this->CellsInfo.data(), mid - begin);
    child[1].MakeLeaf(mid - this->CellsInfo.data(), end - mid);

    this->Nodes[index].MakeNode(static_cast<T>(this->Nodes.size()), dim, clip);
    this->Nodes.insert(this->Nodes.end(), child, child + 2);

    this->SplitStack.emplace(this->Nodes[index].GetRightChildIndex(), rMin, rMax);
    this->SplitStack.emplace(this->Nodes[index].GetLeftChildIndex(), lMin, lMax);
  }

public:
  CellTreeBuilder(vtkCellTreeLocator* locator, TCellTree& tree, vtkDataSet* dataSet,
    int numberOfBuckets, int numberOfNodesPerLeaf)
    : Locator(locator)
    , Tree(tree)
    , DataSet(dataSet)
    , NumberOfBuckets(numberOfBuckets)
    , NumberOfNodesPerLeaf(numberOfNodesPerLeaf)
  {
    const auto numberOfCells = static_cast<T>(this->DataSet->GetNumberOfCells());
    this->CellsInfo.resize(static_cast<size_t>(numberOfCells));

    double min[3] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };
    double max[3] = {
      -VTK_DOUBLE_MAX,
      -VTK_DOUBLE_MAX,
      -VTK_DOUBLE_MAX,
    };

    double cellBounds[6], *cellBoundsPtr;
    cellBoundsPtr = cellBounds;
    for (T i = 0; i < numberOfCells; ++i)
    {
      this->CellsInfo[i].Ind = i;
      this->Locator->GetCellBounds(i, cellBoundsPtr);

      for (uint8_t d = 0; d < 3; ++d)
      {
        this->CellsInfo[i].Min[d] = cellBoundsPtr[2 * d + 0];
        this->CellsInfo[i].Max[d] = cellBoundsPtr[2 * d + 1];

        if (this->CellsInfo[i].Min[d] < min[d])
        {
          min[d] = this->CellsInfo[i].Min[d];
        }
        if (this->CellsInfo[i].Max[d] > max[d])
        {
          max[d] = this->CellsInfo[i].Max[d];
        }
      }
    }

    this->Tree.DataBBox[0] = min[0];
    this->Tree.DataBBox[1] = max[0];
    this->Tree.DataBBox[2] = min[1];
    this->Tree.DataBBox[3] = max[1];
    this->Tree.DataBBox[4] = min[2];
    this->Tree.DataBBox[5] = max[2];

    TCellTreeNode root;
    root.MakeLeaf(0, numberOfCells);
    this->Nodes.push_back(root);

    this->SplitStack.emplace(0, min, max);
  }

  void Initialize()
  {
    auto& buckets = this->Buckets;
    buckets = BucketsType(this->NumberOfBuckets);
  }

  void operator()()
  {
    auto& buckets = this->Buckets;
    while (!this->SplitStack.empty())
    {
      auto splitInfo = std::move(this->SplitStack.top());
      this->SplitStack.pop();
      this->Split(splitInfo.Index, splitInfo.Min, splitInfo.Max, buckets);
    }
  }

  void Reduce()
  {
    this->Tree.Nodes.resize(this->Nodes.size());
    this->Tree.Nodes[0] = this->Nodes[0];

    for (auto ni = this->Tree.Nodes.begin(), nn = this->Tree.Nodes.begin() + 1;
         ni != this->Tree.Nodes.end(); ++ni)
    {
      if (ni->IsLeaf())
      {
        continue;
      }

      *(nn++) = this->Nodes[ni->GetLeftChildIndex()];
      *(nn++) = this->Nodes[ni->GetRightChildIndex()];
      ni->SetChildren(nn - this->Tree.Nodes.begin() - 2);
    }

    const auto numberOfCells = static_cast<size_t>(this->DataSet->GetNumberOfCells());
    this->Tree.Leaves.resize(numberOfCells);
    for (size_t i = 0; i < numberOfCells; ++i)
    {
      this->Tree.Leaves[i] = this->CellsInfo[i].Ind;
    }
    this->CellsInfo.clear();
  }
};

//------------------------------------------------------------------------------
template <typename T>
vtkIdType CellTree<T>::FindCell(
  double* pos, vtkGenericCell* cell, int& subId, double* pcoords, double* weights)
{
  // check if pos outside of bounds
  if (!this->Locator->IsInBounds(this->DataBBox, pos))
  {
    return -1;
  }

  double dist2;

  CellPointTraversal<T> pt(*this, pos);
  while (const TCellTreeNode* n = pt.Next())
  {
    const T* begin = &(this->Leaves[n->Start()]);
    const T* end = begin + n->Size();

    for (; begin != end; ++begin)
    {
      if (this->Locator->InsideCellBounds(pos, *begin))
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
template <typename T>
int CellTree<T>::IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
  double x[3], double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell)
{
  TCellTreeNode *node, *nearNode, *farNode;
  double tmin, tmax, tDist, tHitCell, tBest = VTK_DOUBLE_MAX, xBest[3], pCoordsBest[3];
  double rayDir[3], x0[3], x1[3], hitCellBoundsPosition[3];
  int plane0, plane1, subIdBest = -1;
  vtkMath::Subtract(p2, p1, rayDir);
  double* bounds = this->DataBBox;
  double cellBounds[6], *cellBoundsPtr;
  cellBoundsPtr = cellBounds;
  T cId, cellIdBest = -1;
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
  TreeNodeStack ns;
  // setup our axis optimized ray box edge stuff
  int axis = vtkCellTree::getDominantAxis(rayDir);
  double (*_getMinDist)(const double origin[3], const double dir[3], const double B[6]);
  switch (axis)
  {
    case POS_X:
      _getMinDist = vtkCellTree::_getMinDistPOS_X;
      break;
    case NEG_X:
      _getMinDist = vtkCellTree::_getMinDistNEG_X;
      break;
    case POS_Y:
      _getMinDist = vtkCellTree::_getMinDistPOS_Y;
      break;
    case NEG_Y:
      _getMinDist = vtkCellTree::_getMinDistNEG_Y;
      break;
    case POS_Z:
      _getMinDist = vtkCellTree::_getMinDistPOS_Z;
      break;
    default:
      _getMinDist = vtkCellTree::_getMinDistNEG_Z;
      break;
  }

  // OK, lets walk the tree and find intersections
  TCellTreeNode* n = &this->Nodes.front();
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
    for (T i = 0; i < node->Size(); i++)
    {
      cId = this->Leaves[node->Start() + i];
      if (!cellHasBeenVisited[cId])
      {
        cellHasBeenVisited[cId] = true;

        this->Locator->GetCellBounds(cId, cellBoundsPtr);
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
template <typename T>
struct IntersectionInfo
{
  T CellId;
  std::array<double, 3> IntersectionPoint;
  double TValue;

  IntersectionInfo(T cellId, double x[3], double tValue)
    : CellId(cellId)
    , IntersectionPoint({ x[0], x[1], x[2] })
    , TValue(tValue)
  {
  }
};

//------------------------------------------------------------------------------
template <typename T>
int CellTree<T>::IntersectWithLine(const double p1[3], const double p2[3], const double tol,
  vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell)
{
  using TIntersectionInfo = IntersectionInfo<T>;
  // Initialize the list of points/cells
  if (points)
  {
    points->Reset();
  }
  if (cellIds)
  {
    cellIds->Reset();
  }
  TCellTreeNode *node, *nearNode, *farNode;
  double tmin, tmax, tDist, tHitCell;
  double rayDir[3], x0[3], x1[3], hitCellBoundsPosition[3];
  int plane0, plane1, subId, hitCellBounds;
  vtkMath::Subtract(p2, p1, rayDir);
  double* bounds = this->DataBBox;
  double cellBounds[6], *cellBoundsPtr;
  cellBoundsPtr = cellBounds;
  T cId;
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
  TreeNodeStack ns;

  // we will sort intersections by t, so keep track using these lists
  std::vector<TIntersectionInfo> cellIntersections;
  // OK, lets walk the tree and find intersections
  TCellTreeNode* n = &this->Nodes.front();
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
    for (T i = 0; i < node->Size(); i++)
    {
      cId = this->Leaves[node->Start() + i];
      if (!cellHasBeenVisited[cId])
      {
        cellHasBeenVisited[cId] = true;

        // check whether we intersect the cell bounds
        this->Locator->GetCellBounds(cId, cellBoundsPtr);
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
    auto numIntersections = static_cast<T>(cellIntersections.size());
    std::sort(cellIntersections.begin(), cellIntersections.end(),
      [&](const TIntersectionInfo& a, const TIntersectionInfo& b) { return a.TValue < b.TValue; });
    if (points)
    {
      points->SetNumberOfPoints(numIntersections);
      for (T i = 0; i < numIntersections; ++i)
      {
        points->SetPoint(i, cellIntersections[i].IntersectionPoint.data());
      }
    }
    if (cellIds)
    {
      cellIds->SetNumberOfIds(numIntersections);
      for (T i = 0; i < numIntersections; ++i)
      {
        cellIds->SetId(i, cellIntersections[i].CellId);
      }
    }
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
template <typename T>
void CellTree<T>::Classify(const double origin[3], const double dir[3], double& rDist,
  TCellTreeNode*& nearNode, TCellTreeNode*& parent, TCellTreeNode*& farNode, int& mustCheck)
{
  double tOriginToDivPlane = parent->GetLeftMaxValue() - origin[parent->GetDimension()];
  double tOriginToDivPlane2 = parent->GetRightMinValue() - origin[parent->GetDimension()];
  double tDivDirection = dir[parent->GetDimension()];
  if (tOriginToDivPlane2 > 0) // origin is right of the rmin
  {
    nearNode = &this->Nodes.at(parent->GetLeftChildIndex());
    farNode = &this->Nodes.at(parent->GetLeftChildIndex() + 1);
    rDist = (tDivDirection) ? tOriginToDivPlane2 / tDivDirection : VTK_DOUBLE_MAX;
  }
  else if (tOriginToDivPlane < 0) // origin is left of the lm
  {
    farNode = &this->Nodes.at(parent->GetLeftChildIndex());
    nearNode = &this->Nodes.at(parent->GetLeftChildIndex() + 1);
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
      nearNode = &this->Nodes.at(parent->GetLeftChildIndex());
      farNode = &this->Nodes.at(parent->GetLeftChildIndex() + 1);
      if (!(tOriginToDivPlane > 0 || tOriginToDivPlane < 0))
      {
        mustCheck = 1; // Ray was exactly on edge left max box.
      }
      rDist = (tDivDirection) ? 0 / tDivDirection : VTK_DOUBLE_MAX;
    }
    else
    {
      farNode = &this->Nodes.at(parent->GetLeftChildIndex());
      nearNode = &this->Nodes.at(parent->GetLeftChildIndex() + 1);
      if (!(tOriginToDivPlane2 > 0 || tOriginToDivPlane2 < 0))
      {
        mustCheck = 1; // Ray was exactly on edge right min box.
      }
      rDist = (tDivDirection) ? 0 / tDivDirection : VTK_DOUBLE_MAX;
    }
  }
}

//------------------------------------------------------------------------------
template <typename T>
void SplitNodeBox(CellTreeNode<T>* n, vtkBoundingBox& b, vtkBoundingBox& l, vtkBoundingBox& r)
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
void AddBox(vtkPolyData* pd, double* bounds, int level)
{
  vtkPoints* pts = pd->GetPoints();
  vtkCellArray* lines = pd->GetLines();
  vtkIntArray* levels = vtkArrayDownCast<vtkIntArray>(pd->GetPointData()->GetArray(0));
  double x[3];
  vtkIdType cells[8], ids[2];

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
  // Colour boxes by scalar if array present
  for (int i = 0; levels && i < 8; i++)
  {
    levels->InsertNextTuple1(level);
  }
}

//------------------------------------------------------------------------------
template <typename T>
void CellTree<T>::GenerateRepresentation(int level, vtkPolyData* pd)
{
  NodeInfoStack ns;
  BoxList bl;

  TCellTreeNode* n0 = &this->Nodes.front();
  // create a box for the root
  double* dataBBox = this->DataBBox;
  vtkBoundingBox lbox, rbox,
    rootbox(dataBBox[0], dataBBox[1], dataBBox[2], dataBBox[3], dataBBox[4], dataBBox[5]);
  ns.push(NodeBoxLevel(n0, BoxLevel(rootbox, 0)));
  while (!ns.empty())
  {
    n0 = ns.top().first;
    int lev = ns.top().second.second;
    if (n0->IsLeaf() && ((lev == level) || (level == -1)))
    {
      bl.push_back(BoxLevel(ns.top().second.first, lev));
      ns.pop();
    }
    else if (n0->IsLeaf())
    {
      ns.pop();
    }
    else if (n0->IsNode())
    {
      SplitNodeBox(n0, ns.top().second.first, lbox, rbox);
      TCellTreeNode* n1 = &this->Nodes.at(n0->GetLeftChildIndex());
      TCellTreeNode* n2 = &this->Nodes.at(n0->GetLeftChildIndex() + 1);
      ns.pop();
      ns.push(NodeBoxLevel(n1, BoxLevel(lbox, lev + 1)));
      ns.push(NodeBoxLevel(n2, BoxLevel(rbox, lev + 1)));
    }
  }
  // For each node, add the bbox to our polydata
  for (auto const& b : bl)
  {
    double bounds[6];
    b.first.GetBounds(bounds);
    AddBox(pd, bounds, b.second);
  }
}

//------------------------------------------------------------------------------
template <typename T>
void CellTree<T>::FindCellsWithinBounds(double* bbox, vtkIdList* cells)
{
  NodeInfoStack ns;
  double cellBounds[6], *cellBoundsPtr;
  cellBoundsPtr = cellBounds;
  vtkBoundingBox TestBox(bbox);

  TCellTreeNode* n0 = &this->Nodes.front();
  // create a box for the root
  double* dataBBox = this->DataBBox;
  vtkBoundingBox lbox, rbox,
    rootbox(dataBBox[0], dataBBox[1], dataBBox[2], dataBBox[3], dataBBox[4], dataBBox[5]);
  ns.push(NodeBoxLevel(n0, BoxLevel(rootbox, 0)));
  while (!ns.empty())
  {
    n0 = ns.top().first;
    vtkBoundingBox& nodebox = ns.top().second.first;
    if (TestBox.Intersects(nodebox))
    {
      if (n0->IsLeaf())
      {
        for (T i = 0; i < n0->Size(); i++)
        {
          T cellId = this->Leaves[n0->Start() + i];
          this->Locator->GetCellBounds(cellId, cellBoundsPtr);
          vtkBoundingBox box(cellBoundsPtr);
          if (TestBox.Intersects(box))
          {
            cells->InsertNextId(cellId);
          }
        }
        ns.pop();
      }
      else
      {
        int lev = ns.top().second.second;
        SplitNodeBox(n0, nodebox, lbox, rbox);
        TCellTreeNode* n1 = &this->Nodes.at(n0->GetLeftChildIndex());
        TCellTreeNode* n2 = &this->Nodes.at(n0->GetLeftChildIndex() + 1);
        ns.pop();
        ns.push(NodeBoxLevel(n1, BoxLevel(lbox, lev + 1)));
        ns.push(NodeBoxLevel(n2, BoxLevel(rbox, lev + 1)));
      }
    }
    else
    {
      ns.pop();
    }
  }
}
} // namespace

//------------------------------------------------------------------------------
vtkCellTreeLocator::vtkCellTreeLocator()
{
  this->NumberOfCellsPerNode = 8;
  this->NumberOfBuckets = 6;
  this->Tree = nullptr;
}

//------------------------------------------------------------------------------
vtkCellTreeLocator::~vtkCellTreeLocator()
{
  this->FreeSearchStructure();
  this->FreeCellBounds();
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::FreeSearchStructure()
{
  if (this->Tree)
  {
    delete this->Tree;
    this->Tree = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::BuildLocator()
{
  // don't rebuild if build time is newer than modified and dataset modified time
  if (this->Tree && this->BuildTime > this->MTime && this->BuildTime > this->DataSet->GetMTime())
  {
    return;
  }
  // don't rebuild if UseExistingSearchStructure is ON and a search structure already exists
  if (this->Tree && this->UseExistingSearchStructure)
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
  using namespace detail;
  vtkIdType numCells;
  if (!this->DataSet || (numCells = this->DataSet->GetNumberOfCells() < 1))
  {
    vtkErrorMacro(<< " No Cells in the data set\n");
    return;
  }
  this->FreeSearchStructure();
  this->ComputeCellBounds();
  // Create sorted cell fragments tuples of (cellId,binId). Depending
  // on problem size, different types are used.
  if (numCells >= VTK_INT_MAX)
  {
    this->LargeIds = true;
    auto tree = new CellTree<vtkIdType>(this);
    CellTreeBuilder<vtkIdType> treeBuilder(
      this, *tree, this->DataSet, this->NumberOfBuckets, this->NumberOfCellsPerNode);
    treeBuilder.Initialize();
    treeBuilder();
    treeBuilder.Reduce();
    this->Tree = tree;
  }
  else
  {
    this->LargeIds = false;
    auto tree = new CellTree<int>(this);
    CellTreeBuilder<int> treeBuilder(
      this, *tree, this->DataSet, this->NumberOfBuckets, this->NumberOfCellsPerNode);
    treeBuilder.Initialize();
    treeBuilder();
    treeBuilder.Reduce();
    this->Tree = tree;
  }
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
vtkIdType vtkCellTreeLocator::FindCell(
  double pos[3], double, vtkGenericCell* cell, int& subId, double pcoords[3], double* weights)
{
  this->BuildLocator();
  if (!this->Tree)
  {
    return -1;
  }
  return this->Tree->FindCell(pos, cell, subId, pcoords, weights);
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::FindCellsWithinBounds(double* bbox, vtkIdList* cells)
{
  this->BuildLocator();
  if (!this->Tree)
  {
    return;
  }
  return this->Tree->FindCellsWithinBounds(bbox, cells);
}

//------------------------------------------------------------------------------
int vtkCellTreeLocator::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& t, double x[3], double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell)
{
  this->BuildLocator();
  if (!this->Tree)
  {
    return 0;
  }
  return this->Tree->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId, cellId, cell);
}

//------------------------------------------------------------------------------
int vtkCellTreeLocator::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell)
{
  this->BuildLocator();
  if (!this->Tree)
  {
    return 0;
  }
  return this->Tree->IntersectWithLine(p1, p2, tol, points, cellIds, cell);
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::GenerateRepresentation(int level, vtkPolyData* pd)
{
  this->BuildLocator();
  if (!this->Tree)
  {
    return;
  }
  this->Tree->GenerateRepresentation(level, pd);
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::ShallowCopy(vtkAbstractCellLocator* locator)
{
  using namespace detail;
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
  this->LargeIds = cellLocator->LargeIds;
  if (this->LargeIds)
  {
    auto cellLocatorTree = static_cast<CellTree<vtkIdType>*>(cellLocator->Tree);
    auto tree = new CellTree<vtkIdType>(this);
    tree->Locator = this;
    tree->DataSet = cellLocatorTree->DataSet;
    tree->Leaves = cellLocatorTree->Leaves;
    tree->Nodes = cellLocatorTree->Nodes;
    std::copy_n(cellLocatorTree->DataBBox, 6, tree->DataBBox);
    this->Tree = tree;
  }
  else
  {
    auto cellLocatorTree = static_cast<CellTree<int>*>(cellLocator->Tree);
    auto tree = new CellTree<int>(this);
    tree->Locator = this;
    tree->DataSet = cellLocatorTree->DataSet;
    tree->Leaves = cellLocatorTree->Leaves;
    tree->Nodes = cellLocatorTree->Nodes;
    std::copy_n(cellLocatorTree->DataBBox, 6, tree->DataBBox);
    this->Tree = tree;
  }
}

//------------------------------------------------------------------------------
void vtkCellTreeLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfBuckets: " << this->NumberOfBuckets << "\n";
  os << indent << "LargeIds: " << this->LargeIds << "\n";
}
