// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @brief Collection of comparison functions for std::sort.
 *
 */

#ifndef vtkBlockSortHelper_h
#define vtkBlockSortHelper_h

#include "vtkCamera.h"
#include "vtkDataSet.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkRenderer.h"
#include "vtkVector.h"

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
  //  1 if second is farther than first
  template <typename TT>
  int CompareOrderWithUncertainty(TT& first, TT& second)
  {
    double abounds[6], bbounds[6];
    vtkBlockSortHelper::GetBounds<TT>(first, abounds);
    vtkBlockSortHelper::GetBounds<TT>(second, bbounds);
    return CompareBoundsOrderWithUncertainty(abounds, bbounds);
  }

  // -1 if first is closer than second
  //  0 if unknown
  //  1 if second is farther than first
  int CompareBoundsOrderWithUncertainty(const double abounds[6], const double bbounds[6])
  {
    double bboundsP[6];
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
    if (fabs(aboundsP[degenAxes[0] * 2] - bboundsP[degenAxes[0] * 2]) > 0.01 * atoblength)
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

#ifdef MB_DEBUG
template <class RandomIt>
class gnode
{
public:
  RandomIt Value;
  bool Visited = false;
  std::vector<gnode<RandomIt>*> Closer;
};

template <class RandomIt>
bool operator==(gnode<RandomIt> const& lhs, gnode<RandomIt> const& rhs)
{
  return lhs.Value == rhs.Value && lhs.Closer == rhs.Closer;
}

template <class RandomIt>
bool findCycle(gnode<RandomIt>& start, std::vector<gnode<RandomIt>>& graph,
  std::vector<gnode<RandomIt>>& active, std::vector<gnode<RandomIt>>& loop)
{
  if (start.Visited)
  {
    return false;
  }

  // add the current node to active list
  active.push_back(start);

  // traverse the closer nodes one by one depth first
  for (auto& close : start.Closer)
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
#endif

template <class RandomIt, typename T>
inline void Sort(RandomIt bitr, RandomIt eitr, BackToFront<T>& me)
{
  auto start = bitr;

  // brute force for testing

  std::vector<typename RandomIt::value_type> working;
  std::vector<typename RandomIt::value_type> result;
  working.assign(bitr, eitr);
  size_t numNodes = working.size();

#ifdef MB_DEBUG
  // check for any short loops and debug
  for (auto it = working.begin(); it != working.end(); ++it)
  {
    auto it2 = it;
    it2++;
    for (; it2 != working.end(); ++it2)
    {
      int comp1 = me.CompareOrderWithUncertainty(*it, *it2);
      int comp2 = me.CompareOrderWithUncertainty(*it2, *it);
      if (comp1 * comp2 > 0)
      {
        me.CompareOrderWithUncertainty(*it, *it2);
        me.CompareOrderWithUncertainty(*it2, *it);
      }
    }
  }

  // build the graph
  std::vector<gnode<RandomIt>> graph;
  for (auto it = working.begin(); it != working.end(); ++it)
  {
    gnode<RandomIt> anode;
    anode.Value = it;
    graph.push_back(anode);
  }
  for (auto& git : graph)
  {
    for (auto& next : graph)
    {
      if (git.Value != next.Value && me.CompareOrderWithUncertainty(*git.Value, *next.Value) > 0)
      {
        git.Closer.push_back(&next);
      }
    }
  }

  // graph constructed, now look for a loop
  std::vector<gnode<RandomIt>> active;
  std::vector<gnode<RandomIt>> loop;
  for (auto& gval : graph)
  {
    loop.clear();
    if (findCycle(gval, graph, active, loop))
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

  // loop over items and find the first that is not in front of any others
  for (auto it = working.begin(); it != working.end();)
  {
    auto it2 = working.begin();
    for (; it2 != working.end(); ++it2)
    {
      // if another block is in front of this block then this is not the
      // closest block
      if (it != it2 && me.CompareOrderWithUncertainty(*it, *it2) > 0)
      {
        // not a winner
        break;
      }
    }
    if (it2 == working.end())
    {
      // found a winner, add it to the results, remove from the working set and then restart
      result.push_back(*it);
      working.erase(it);
      it = working.begin();
    }
    else
    {
      ++it;
    }
  }

  if (result.size() != numNodes)
  {
    vtkGenericWarningMacro("sorting failed");
  }

  // copy results to original container
  std::reverse_copy(result.begin(), result.end(), start);
};
VTK_ABI_NAMESPACE_END
}

#endif // vtkBlockSortHelper_h
// VTK-HeaderTest-Exclude: vtkBlockSortHelper.h
