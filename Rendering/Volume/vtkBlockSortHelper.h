// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @brief Collection of comparison functions for std::sort.
 *
 */

#ifndef vtkBlockSortHelper_h
#define vtkBlockSortHelper_h

#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkDataSet.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkRenderer.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <set>
#include <stack>
#include <vector>

namespace vtkBlockSortHelper
{
VTK_ABI_NAMESPACE_BEGIN

template <typename T>
inline void GetBounds(T a, double bds[6])
{
  a.GetBounds(bds);
}

template <>
inline void GetBounds(vtkDataSet* first, double bds[6])
{
  first->GetBounds(bds);
}

template <typename ValueT>
struct BlockGroup : public std::vector<ValueT>
{
  void GetBounds(double bds[6])
  {
    vtkBoundingBox bbox;
    for (auto it = this->begin(); it != this->end(); ++it)
    {
      double localBds[6];
      vtkBlockSortHelper::GetBounds(*it, localBds);
      bbox.AddBounds(localBds);
    }
    bbox.GetBounds(bds);
  }
};

/**
 *  operator() for back-to-front sorting.
 *
 *  \note Use as the 'comp' parameter of std::sort.
 *
 */
template <typename T>
struct BackToFront
{
  vtkVector3d CameraPosition;
  vtkVector3d CameraViewDirection;
  bool CameraIsParallel;

  //----------------------------------------------------------------------------
  BackToFront(vtkRenderer* ren, vtkMatrix4x4* volMatrix)
  {
    vtkCamera* cam = ren->GetActiveCamera();
    this->CameraIsParallel = (cam->GetParallelProjection() != 0);

    double camWorldPos[4];
    cam->GetPosition(camWorldPos);
    camWorldPos[3] = 1.0;

    double camWorldFocalPoint[4];
    cam->GetFocalPoint(camWorldFocalPoint);
    camWorldFocalPoint[3] = 1.0;

    // Transform the camera position to the volume (dataset) coordinate system.
    vtkNew<vtkMatrix4x4> InverseVolumeMatrix;
    InverseVolumeMatrix->DeepCopy(volMatrix);
    InverseVolumeMatrix->Invert();
    InverseVolumeMatrix->MultiplyPoint(camWorldPos, camWorldPos);
    InverseVolumeMatrix->MultiplyPoint(camWorldFocalPoint, camWorldFocalPoint);

    this->CameraPosition = vtkVector3d(camWorldPos[0], camWorldPos[1], camWorldPos[2]);
    this->CameraPosition = this->CameraPosition / vtkVector3d(camWorldPos[3]);

    vtkVector3d camFP(camWorldFocalPoint[0], camWorldFocalPoint[1], camWorldFocalPoint[2]);
    camFP = camFP / vtkVector3d(camWorldFocalPoint[3]);

    this->CameraViewDirection = camFP - this->CameraPosition;
  }

  //----------------------------------------------------------------------------
  // camPos is used when is_parallel is false, else viewdirection is used.
  // thus a valid camPos is only needed if is_parallel is false, and a valid viewdirection
  // is only needed if is_parallel is true.
  BackToFront(const vtkVector3d& camPos, const vtkVector3d& viewdirection, bool is_parallel)
    : CameraPosition(camPos)
    , CameraViewDirection(viewdirection)
    , CameraIsParallel(is_parallel)
  {
  }

  // -1 if first is closer than second
  //  0 if unknown
  //  1 if second is closer than first
  //  allowDisconnected is used to permit the comparisons of bounding boxes whose faces/edges do not
  //  touch at all.
  template <typename TT>
  inline int CompareOrderWithUncertainty(TT& first, TT& second, bool allowDisconnected = false)
  {
    double abounds[6], bbounds[6];
    vtkBlockSortHelper::GetBounds<TT>(first, abounds);
    vtkBlockSortHelper::GetBounds<TT>(second, bbounds);
    return CompareBoundsOrderWithUncertainty(abounds, bbounds, allowDisconnected);
  }

  // -1 if first is closer than second
  //  0 if unknown
  //  1 if second is closer than first
  //  allowDisconnected is used to permit the comparisons of bounding boxes whose faces/edges do not
  //  touch at all.
  inline int CompareBoundsOrderWithUncertainty(
    const double abounds[6], const double bbounds[6], bool allowDisconnected = false)
  {
    // bounds of the projection of block A onto block B
    double bboundsP[6];
    // bounds of the projection of block B onto block A
    double aboundsP[6];
    for (int i = 0; i < 6; ++i)
    {
      int low = 2 * (i / 2);
      bboundsP[i] = bbounds[i];
      if (bboundsP[i] < abounds[low])
      {
        bboundsP[i] = abounds[low];
      }
      if (bboundsP[i] > abounds[low + 1])
      {
        bboundsP[i] = abounds[low + 1];
      }
      aboundsP[i] = abounds[i];
      if (aboundsP[i] < bbounds[low])
      {
        aboundsP[i] = bbounds[low];
      }
      if (aboundsP[i] > bbounds[low + 1])
      {
        aboundsP[i] = bbounds[low + 1];
      }
    }

    // Determine the dimensionality of the projection
    /// dims == 3 | Overlap? Yes | Type: Volume
    /// dims == 2 | Overlap? Yes | Type: Plane
    /// dims == 1 | Overlap? Yes | Type: Line
    /// dims == 0 | Overlap? No | Type: None
    int dims = 0;
    int degenDims = 0;
    int degenAxes[3];
    int dimSize[3];
    for (int i = 0; i < 6; i += 2)
    {
      if (aboundsP[i] != aboundsP[i + 1])
      {
        dimSize[dims] = aboundsP[i + 1] - aboundsP[i];
        dims++;
      }
      else
      {
        degenAxes[degenDims] = i / 2;
        degenDims++;
      }
    }

    // overlapping volumes?
    // in that case find the two largest dimensions
    // the 3d overlap is collapsed down to a 2d overlap.
    // geneally this should not happen
    if (dims == 3)
    {
      if (dimSize[0] < dimSize[1])
      {
        if (dimSize[0] < dimSize[2])
        {
          degenAxes[0] = 0;
        }
        else
        {
          degenAxes[0] = 2;
        }
      }
      else
      {
        if (dimSize[1] < dimSize[2])
        {
          degenAxes[0] = 1;
        }
        else
        {
          degenAxes[0] = 2;
        }
      }
      dims = 2;
      degenDims = 1;
    }

    // compute the direction from a to b
    double atobdir[3] = { bbounds[0] + bbounds[1] - abounds[0] - abounds[1],
      bbounds[2] + bbounds[3] - abounds[2] - abounds[3],
      bbounds[4] + bbounds[5] - abounds[4] - abounds[5] };
    double atoblength = vtkMath::Normalize(atobdir);

    // no comment on blocks that do not touch
    if (!allowDisconnected &&
      (fabs(aboundsP[degenAxes[0] * 2] - bboundsP[degenAxes[0] * 2]) > 0.01 * atoblength))
    {
      return 0;
    }

    // compute the camera direction
    vtkVector3d dir;
    if (this->CameraIsParallel)
    {
      dir = this->CameraViewDirection;
    }
    else
    {
      // compute a point for the half space plane
      vtkVector3d planePoint(0.25 * (aboundsP[0] + aboundsP[1] + bboundsP[0] + bboundsP[1]),
        0.25 * (aboundsP[2] + aboundsP[3] + bboundsP[2] + bboundsP[3]),
        0.25 * (aboundsP[4] + aboundsP[5] + bboundsP[4] + bboundsP[5]));
      dir = planePoint - this->CameraPosition;
    }
    dir.Normalize();

    // planar interface
    if (dims == 2)
    {
      double plane[3] = { 0, 0, 0 };
      plane[degenAxes[0]] = 1.0;
      // dot product with camera direction
      double dot = dir[0] * plane[0] + dir[1] * plane[1] + dir[2] * plane[2];
      if (dot == 0)
      {
        return 0;
      }
      // figure out what side we are on
      double side = plane[0] * atobdir[0] + plane[1] * atobdir[1] + plane[2] * atobdir[2];
      return (dot * side < 0 ? 1 : -1);
    }

    return 0;
  }
};

template <typename RandomIt>
class GraphNode
{
public:
  RandomIt Value;
  bool Visited = false;
  std::set<GraphNode<RandomIt>*> Neighbors;
};

template <typename RandomIt>
bool operator==(GraphNode<RandomIt> const& lhs, GraphNode<RandomIt> const& rhs)
{
  return lhs.Value == rhs.Value && lhs.Neighbors == rhs.Neighbors;
}

template <typename RandomIt>
bool findCycle(GraphNode<RandomIt>& start, std::vector<GraphNode<RandomIt>>& graph,
  std::vector<GraphNode<RandomIt>>& active, std::vector<GraphNode<RandomIt>>& loop)
{
  if (start.Visited)
  {
    return false;
  }

  // add the current node to active list
  active.push_back(start);

  // traverse the closer nodes one by one depth first
  for (auto& close : start.Neighbors)
  {
    if (close->Visited)
    {
      continue;
    }

    // is the node already in the active list? if so we have a loop
    for (auto ait = active.begin(); ait != active.end(); ++ait)
    {
      if (ait->Value == close->Value)
      {
        loop.push_back(*ait);
        return true;
      }
    }
    // otherwise recurse
    if (findCycle(*close, graph, active, loop))
    {
      // a loop was detected, build the loop output
      loop.push_back(*close);
      return true;
    }
  }

  active.erase(std::find(active.begin(), active.end(), start));
  start.Visited = true;
  return false;
}

template <typename RandomIt>
void VisitNeighborsDFS(GraphNode<RandomIt>& start, std::set<RandomIt>& connected)
{
  // use a stack instead of callstack for dfs
  std::stack<GraphNode<RandomIt>*> nodeStack;
  nodeStack.push(&start);
  while (!nodeStack.empty())
  {
    auto node = nodeStack.top();
    nodeStack.pop();
    // insert into `connected` only if node was not visited.
    if (!node->Visited)
    {
      node->Visited = true;
      connected.insert(node->Value);
    }
    // push all unvisited neighbors onto the stack.
    for (auto& neighbor : start.Neighbors)
    {
      if (!neighbor->Visited)
      {
        nodeStack.push(neighbor);
      }
    }
  }
}

template <typename RandomIt,
  typename BlockGroupType = typename vtkBlockSortHelper::BlockGroup<typename RandomIt::value_type>>
std::vector<BlockGroupType> FindConnectedBlocks(std::vector<GraphNode<RandomIt>>& graph)
{
  // unvisit all nodes.
  for (auto& node : graph)
  {
    node.Visited = false;
  }
  std::vector<BlockGroupType> result;
  for (auto& node : graph)
  {
    // skip if the node was visited.
    if (node.Visited)
    {
      continue;
    }
    std::set<RandomIt> connected;
    VisitNeighborsDFS(node, connected);
    if (!connected.empty())
    {
      BlockGroupType blocks;
      blocks.reserve(connected.size());
      for (auto& elem : connected)
      {
        blocks.emplace_back(*elem);
      }
      result.emplace_back(blocks);
    }
  }
  return result;
}

template <typename ValueType, typename B2FType>
/// Sorts `input` vector in-place from front-to-back.
inline void SortFrontToBackImplementation(
  std::vector<ValueType>& input, BackToFront<B2FType>& b2f, bool allowDisconnected = false)
{
  std::vector<ValueType> result;
  const std::size_t numNodes = input.size();
  result.reserve(numNodes);

  // loop over the `input` vector in search of a block that is the front most.
  // as we discover such blocks, they are moved from `input` to `result` and
  // the `input` vector is shortened in length. Repeat this process until the
  // last block in the `input` vector is reached.
  for (auto it = input.begin(); it != input.end();)
  {
    auto it2 = input.begin();
    for (; it2 != input.end(); ++it2)
    {
      // if `it2` is closer than `it`, then `it` is not the closest block,
      // so break and try the next block from `input` vector.
      if (it != it2 && b2f.CompareOrderWithUncertainty(*it, *it2, allowDisconnected) > 0)
      {
        // not a winner
        break;
      }
    }
    if (it2 == input.end())
    {
      // found a winner, add it to the `result`, remove from the input set and then restart
      result.push_back(*it);
      input.erase(it);
      it = input.begin();
    }
    else
    {
      ++it;
    }
  }
  if (result.size() != numNodes)
  {
    vtkGenericWarningMacro(<< "SortFrontToBackImplementation failed with allowDisconnected="
                           << allowDisconnected);
    // do not modify input vector.
    return;
  }
  input.assign(result.begin(), result.end());
}

template <typename RandomIt, typename T>
inline void Sort(RandomIt bitr, RandomIt eitr, BackToFront<T>& me)
{
  auto start = bitr;

  // brute force for testing

  using ElementType = typename RandomIt::value_type;
  std::vector<ElementType> working;
  std::vector<ElementType> result;
  working.assign(bitr, eitr);
  const size_t numNodes = working.size();

  // build a graph that describes neighbors of each block.
  std::vector<GraphNode<RandomIt>> graph;
  graph.reserve(numNodes);
  for (auto it = working.begin(); it != working.end(); ++it)
  {
    GraphNode<RandomIt> anode;
    anode.Value = it;
    graph.emplace_back(anode);
  }
  for (auto& node1 : graph)
  {
    for (auto& node2 : graph)
    {
      if (node1.Value != node2.Value &&
        me.CompareOrderWithUncertainty(*node1.Value, *node2.Value) != 0)
      {
        // node2 is a face/line neighbor of node1
        node1.Neighbors.insert(&node2);
      }
    }
  }

#ifdef MB_DEBUG
  // graph constructed, now look for a loop
  std::vector<GraphNode<RandomIt>> active;
  std::vector<GraphNode<RandomIt>> loop;
  for (auto& node : graph)
  {
    loop.clear();
    if (findCycle(node, graph, active, loop))
    {
      vtkVector3d dir = me.CameraViewDirection;
      dir.Normalize();
      vtkGenericWarningMacro("found a loop cam dir: " << dir[0] << " " << dir[1] << " " << dir[2]);
      for (auto& lval : loop)
      {
        double bnds[6];
        vtkBlockSortHelper::GetBounds((*lval.Value), bnds);
        vtkGenericWarningMacro(<< bnds[0] << " " << bnds[1] << " " << bnds[2] << " " << bnds[3]
                               << " " << bnds[4] << " " << bnds[5]);
      }
    }
  }
#endif

  // break graph into groups of blocks which are connected by face/line
  // The blocks inside the group are all connected.
  using BlockGroupType = BlockGroup<ElementType>;
  std::vector<BlockGroupType> blockGroups = vtkBlockSortHelper::FindConnectedBlocks(graph);
  // sort elements inside each of the block group.
  for (auto& blockGroup : blockGroups)
  {
    vtkBlockSortHelper::SortFrontToBackImplementation(blockGroup, me, /*allowDisconnected=*/false);
  }
  // now sort the overall blockGroup(s).
  vtkBlockSortHelper::SortFrontToBackImplementation(blockGroups, me, /*allowDisconnected=*/true);
  // collect all blocks in the back-to-front sorted order.
  result.reserve(numNodes);
  for (const BlockGroupType& blocks : blockGroups)
  {
    for (const ElementType& block : blocks)
    {
      result.push_back(block);
    }
  }
  if (result.size() != numNodes)
  {
    vtkGenericWarningMacro("sorting failed");
    // for whatever reason, sorting failed, return without modifying `start`
    return;
  }
  // copy results to original container
  std::reverse_copy(result.begin(), result.end(), start);
}
VTK_ABI_NAMESPACE_END
}

#endif // vtkBlockSortHelper_h
// VTK-HeaderTest-Exclude: vtkBlockSortHelper.h
