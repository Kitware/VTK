//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_worklet_connectivity_union_find_h
#define viskores_worklet_connectivity_union_find_h

#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace connectivity
{

// Reference:
//     Jayanti, Siddhartha V., and Robert E. Tarjan.
//     "Concurrent Disjoint Set Union." arXiv preprint arXiv:2003.01203 (2020).
class UnionFind
{
public:
  // This is the naive findRoot() without path compaction in SV Jayanti et. al.
  // Since the parents array is read-only in this function, there is no data
  // race when it is called by multiple treads concurrently. We can just call
  // Get(). For cases where findRoot() is used in other functions that write
  // to parents (e.g. Unite()), Get() actually calls Load() with
  // memory_order_acquire ordering, which in turn ensure writes by other threads
  // are reflected.
  template <typename Parents>
  static VISKORES_EXEC viskores::Id findRoot(const Parents& parents, viskores::Id index)
  {
    while (parents.Get(index) != index)
      index = parents.Get(index);
    return index;
  }

  template <typename Parents>
  static VISKORES_EXEC void Unite(Parents& parents, viskores::Id u, viskores::Id v)
  {
    // Data Race Resolutions
    // Since this function modifies the Union-Find data structure, concurrent
    // invocation of it by 2 or more threads causes potential data race. Here
    // is a case analysis why the potential data race does no harm in the
    // context of the single pass connected component algorithm.

    // Case 1, Two threads calling Unite(u, v) (and/or Unite(v, u)) concurrently.
    // Problem: One thread might attach u to v while the other thread attach
    // v to u, causing a cycle in the Union-Find data structure.
    // Resolution: This is not so much of a race condition but a problem with
    // the consistency of the algorithm. This can also happen in serial.
    // This is resolved by "linking by index" as in SV Jayanti et.al. with less
    // than as the total order. The two threads will make the same decision on
    // how to Unite the two trees (e.g. from root with larger id to root with
    // smaller id.) This avoids cycles in the resulting graph and maintains the
    // rooted forest structure of Union-Find at the expense of duplicated (but
    // benign) work.

    // Case 2, T0 calling Unite(u, v) and T1 calling Unite(u, w) concurrently.
    // Problem I: There is a potential write after read data race. After T0
    // calls findRoot() for u but before actually updating the parent of root_u,
    // T1 might have changed root_u to root_w and made root_u "obsolete".
    // When the root of the tree to be attached to (e.g. root_u, when root_u <  root_v)
    // is changed, there is no hazard, since we are just attaching a tree to a
    // now a non-root node, root_u, (thus, root_w <- root_u <- root_v).
    // However, when the root of the attaching tree (root_v) is changed, it
    // means that the root_u has been attached to yet some other root_s and became
    // a non-root node. If we are now attaching this non-root node to root_w we
    // would leave root_s behind and undoing previous work.
    // Atomic Load/Store with memory_order_acquire are not able to detect this
    // data race. While Load sees all previous Stores by other threads, it can not
    // be aware of any Store after the Load.
    // Resolution: Use atomic Compare and Swap in a loop when updating root_u.
    // CAS will check if root of u has been updated by some other thread between
    // findRoot(u) is called and when root of u is going to be updated. This is
    // done by comparing the root_u = findRoot(u) to the current value at
    // parents[root_u]. If they are the same, no data race has happened and the
    // value in parents[root_u] is updated. However, if root_u != parent[root_u],
    // it means parent[root_u] has been updated by some other thread. CAS returns
    // the new value of parent[root_u] (root_s in the Problem description) which
    // we can use as the new root_u. We keep retrying until there is no more
    // data race and root_u == root_v i.e. they are in the same component.

    // Problem II: There is a potential concurrent write data race as it is
    // possible for the two threads to try to change the same old root to
    // different new roots, e.g. T0 calls parents.Set(root_u, root_v) while T1
    // calls parents.Set(root_u, root_w) where root_v < root_u and root_w < root_u
    // (but the order of root_v and root_w is unspecified.) Each thread assumes
    // success while the outcome is actually unspecified.
    // Resolution: Use an atomic Compare and Swap is suggested in SV Janati et. al.
    // as well as J. Jaiganesht et. al. to resolve the data race. The CAS
    // checks if the old root is the same as what we expected. If so, there is
    // no data race, CAS will set the root to the desired new value. The return
    // value from CAS will equal to our expected old root and signifies a
    // successful write which terminates the while loop.
    // If the old root is not what we expected, it has been updated by some
    // other thread and the update by this thread fails. The root as updated by
    // the other thread is returned. This returned value would not equal to
    // our desired new root, signifying the need to retry with the while loop.
    // We can use this return "new root" as is without calling findRoot() to
    // find the "new root". The while loop terminates when both u and v have
    // the same root (thus united).
    viskores::Id root_u = UnionFind::findRoot(parents, u);
    viskores::Id root_v = UnionFind::findRoot(parents, v);

    while (root_u != root_v)
    {
      // FIXME: we might be executing the loop one extra time than necessary.
      if (root_u < root_v)
        parents.CompareExchange(root_v, &root_v, root_u);
      else if (root_u > root_v)
        parents.CompareExchange(root_u, &root_u, root_v);
    }
  }

  // This compresses the path from each node to its root thus flattening the
  // trees and guarantees that the output trees will be rooted stars, i.e.
  // they all have depth of 1.

  // There is a "seemly" data race for this function. The root returned by
  // findRoot() in one thread might become out of date if some other
  // thread changed it before this function calls parents.Set() making the
  // result tree not "short" enough and thus calls for a CompareAndSwap retry
  // loop. However, this data race does not happen for the following reasons:
  // 1. Since the only way for a root of a tree to be changed is through Unite(),
  // as long as there is no concurrent invocation of Unite() and Flatten() there
  // is no data race. This applies even for a compacting findRoot() which can
  // still only change the parents of non-root nodes.
  // 2. By the same token, since findRoot() does not change root and most
  // "damage" parents.Set() can do is resetting root's parent to itself,
  // the root of a tree can never be changed by this function. Thus, here
  // is no data race between concurrent invocations of this function.

  // Since the current findRoot() does not do path compaction, this algorithm
  // has O(n) depth with O(n^2) of total work on a Parallel Random Access
  // Machine (PRAM). However, we don't live in a synchronous, infinite number
  // of processor PRAM world. In reality, since we put "parent pointers" in a
  // array and all the pointers are pointing from larger indices to smaller
  // ones, invocation for nodes with smaller ids are mostly likely be scheduled
  // before and completes earlier than nodes with larger ids. This makes
  // the "effective" path length shorter for nodes with larger ids.
  // In this way, concurrency actually helps with algorithm complexity.
  template <typename Parents>
  static VISKORES_EXEC void Flatten(Parents& parents, viskores::Id index)
  {
    auto root = findRoot(parents, index);
    parents.Set(index, root);
  }
};

class PointerJumping : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayInOut comp);
  using ExecutionSignature = void(WorkIndex, _1);
  using InputDomain = _1;

  template <typename InOutPortalType>
  VISKORES_EXEC void operator()(viskores::Id index, InOutPortalType& comps) const
  {
    UnionFind::Flatten(comps, index);
  }
};

} // connectivity
} // worklet
} // viskores
#endif // viskores_worklet_connectivity_union_find_h
