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

#include <math.h>

#include <viskores/Math.h>
#include <viskores/VectorAnalysis.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/cont/AtomicArray.h>

#include <viskores/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/MortonCodes.h>
#include <viskores/rendering/raytracing/RayTracingTypeDefs.h>
#include <viskores/rendering/raytracing/Worklets.h>

#include <viskores/worklet/WorkletMapField.h>

#define AABB_EPSILON 0.00001f
namespace viskores
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class LinearBVHBuilder
{
public:
  class CountingIterator;

  class GatherFloat32;

  class CreateLeafs;

  class BVHData;

  class PropagateAABBs;

  class TreeBuilder;

  VISKORES_CONT
  LinearBVHBuilder() {}

  VISKORES_CONT void SortAABBS(BVHData& bvh, bool);

  VISKORES_CONT void BuildHierarchy(BVHData& bvh);

  VISKORES_CONT void Build(LinearBVH& linearBVH);
}; // class LinearBVHBuilder

class LinearBVHBuilder::CountingIterator : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  CountingIterator() {}
  using ControlSignature = void(FieldOut);
  using ExecutionSignature = void(WorkIndex, _1);
  VISKORES_EXEC
  void operator()(const viskores::Id& index, viskores::Id& outId) const { outId = index; }
}; //class countingIterator

class LinearBVHBuilder::GatherFloat32 : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  GatherFloat32() {}
  using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(WorkIndex, _1, _2, _3);

  template <typename InType, typename OutType>
  VISKORES_EXEC void operator()(const viskores::Id& outIndex,
                                const viskores::Id& inIndex,
                                const InType& inPortal,
                                OutType& outPortal) const
  {
    outPortal.Set(outIndex, inPortal.Get(inIndex));
  }
}; //class GatherFloat

class LinearBVHBuilder::CreateLeafs : public viskores::worklet::WorkletMapField
{

public:
  VISKORES_CONT
  CreateLeafs() {}

  typedef void ControlSignature(FieldIn, WholeArrayOut);
  typedef void ExecutionSignature(_1, _2, WorkIndex);

  template <typename LeafPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& dataIndex,
                                LeafPortalType& leafs,
                                const viskores::Id& index) const
  {
    const viskores::Id offset = index * 2;
    leafs.Set(offset, 1);             // number of primitives
    leafs.Set(offset + 1, dataIndex); // number of primitives
  }
}; //class createLeafs

class LinearBVHBuilder::BVHData
{
public:
  viskores::cont::ArrayHandle<viskores::UInt32> mortonCodes;
  viskores::cont::ArrayHandle<viskores::Id> parent;
  viskores::cont::ArrayHandle<viskores::Id> leftChild;
  viskores::cont::ArrayHandle<viskores::Id> rightChild;
  viskores::cont::ArrayHandle<viskores::Id> leafs;
  viskores::cont::ArrayHandle<viskores::Bounds> innerBounds;
  viskores::cont::ArrayHandleCounting<viskores::Id> leafOffsets;
  AABBs& AABB;

  VISKORES_CONT BVHData(viskores::Id numPrimitives, AABBs& aabbs)
    : leafOffsets(0, 2, numPrimitives)
    , AABB(aabbs)
    , NumPrimitives(numPrimitives)
  {
    InnerNodeCount = NumPrimitives - 1;
    viskores::Id size = NumPrimitives + InnerNodeCount;

    parent.Allocate(size);
    leftChild.Allocate(InnerNodeCount);
    rightChild.Allocate(InnerNodeCount);
    innerBounds.Allocate(InnerNodeCount);
    mortonCodes.Allocate(NumPrimitives);
  }

  VISKORES_CONT
  ~BVHData() {}

  VISKORES_CONT
  viskores::Id GetNumberOfPrimitives() const { return NumPrimitives; }
  VISKORES_CONT
  viskores::Id GetNumberOfInnerNodes() const { return InnerNodeCount; }

private:
  viskores::Id NumPrimitives;
  viskores::Id InnerNodeCount;

}; // class BVH

class LinearBVHBuilder::PropagateAABBs : public viskores::worklet::WorkletMapField
{
private:
  viskores::Int32 LeafCount;

public:
  VISKORES_CONT
  PropagateAABBs(viskores::Int32 leafCount)
    : LeafCount(leafCount)

  {
  }
  using ControlSignature = void(WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,     //Parents
                                WholeArrayIn,     //lchild
                                WholeArrayIn,     //rchild
                                AtomicArrayInOut, //counters
                                WholeArrayInOut   // flatbvh
  );
  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12);

  template <typename InputPortalType,
            typename OffsetPortalType,
            typename IdPortalType,
            typename AtomicType,
            typename BVHType>
  VISKORES_EXEC_CONT void operator()(const viskores::Id workIndex,
                                     const InputPortalType& xmin,
                                     const InputPortalType& ymin,
                                     const InputPortalType& zmin,
                                     const InputPortalType& xmax,
                                     const InputPortalType& ymax,
                                     const InputPortalType& zmax,
                                     const OffsetPortalType& leafOffsets,
                                     const IdPortalType& parents,
                                     const IdPortalType& leftChildren,
                                     const IdPortalType& rightChildren,
                                     AtomicType& counters,
                                     BVHType& flatBVH) const

  {
    //move up into the inner nodes
    viskores::Id currentNode = LeafCount - 1 + workIndex;
    viskores::Id2 childVector;
    while (currentNode != 0)
    {
      currentNode = parents.Get(currentNode);
      viskores::Int32 oldCount = counters.Add(currentNode, 1);
      if (oldCount == 0)
      {
        return;
      }
      viskores::Id currentNodeOffset = currentNode * 4;
      childVector[0] = leftChildren.Get(currentNode);
      childVector[1] = rightChildren.Get(currentNode);
      if (childVector[0] > (LeafCount - 2))
      {
        //our left child is a leaf, so just grab the AABB
        //and set it in the current node
        childVector[0] = childVector[0] - LeafCount + 1;

        viskores::Vec4f_32
          first4Vec; // = FlatBVH.Get(currentNode); only this one needs effects this

        first4Vec[0] = xmin.Get(childVector[0]);
        first4Vec[1] = ymin.Get(childVector[0]);
        first4Vec[2] = zmin.Get(childVector[0]);
        first4Vec[3] = xmax.Get(childVector[0]);
        flatBVH.Set(currentNodeOffset, first4Vec);

        viskores::Vec4f_32 second4Vec = flatBVH.Get(currentNodeOffset + 1);
        second4Vec[0] = ymax.Get(childVector[0]);
        second4Vec[1] = zmax.Get(childVector[0]);
        flatBVH.Set(currentNodeOffset + 1, second4Vec);
        // set index to leaf
        viskores::Id leafIndex = leafOffsets.Get(childVector[0]);
        childVector[0] = -(leafIndex + 1);
      }
      else
      {
        //our left child is an inner node, so gather
        //both AABBs in the child and join them for
        //the current node left AABB.
        viskores::Id child = childVector[0] * 4;

        viskores::Vec4f_32 cFirst4Vec = flatBVH.Get(child);
        viskores::Vec4f_32 cSecond4Vec = flatBVH.Get(child + 1);
        viskores::Vec4f_32 cThird4Vec = flatBVH.Get(child + 2);

        cFirst4Vec[0] = viskores::Min(cFirst4Vec[0], cSecond4Vec[2]);
        cFirst4Vec[1] = viskores::Min(cFirst4Vec[1], cSecond4Vec[3]);
        cFirst4Vec[2] = viskores::Min(cFirst4Vec[2], cThird4Vec[0]);
        cFirst4Vec[3] = viskores::Max(cFirst4Vec[3], cThird4Vec[1]);
        flatBVH.Set(currentNodeOffset, cFirst4Vec);

        viskores::Vec4f_32 second4Vec = flatBVH.Get(currentNodeOffset + 1);
        second4Vec[0] = viskores::Max(cSecond4Vec[0], cThird4Vec[2]);
        second4Vec[1] = viskores::Max(cSecond4Vec[1], cThird4Vec[3]);

        flatBVH.Set(currentNodeOffset + 1, second4Vec);
      }

      if (childVector[1] > (LeafCount - 2))
      {
        //our right child is a leaf, so just grab the AABB
        //and set it in the current node
        childVector[1] = childVector[1] - LeafCount + 1;


        viskores::Vec4f_32 second4Vec = flatBVH.Get(currentNodeOffset + 1);

        second4Vec[2] = xmin.Get(childVector[1]);
        second4Vec[3] = ymin.Get(childVector[1]);
        flatBVH.Set(currentNodeOffset + 1, second4Vec);

        viskores::Vec4f_32 third4Vec;
        third4Vec[0] = zmin.Get(childVector[1]);
        third4Vec[1] = xmax.Get(childVector[1]);
        third4Vec[2] = ymax.Get(childVector[1]);
        third4Vec[3] = zmax.Get(childVector[1]);
        flatBVH.Set(currentNodeOffset + 2, third4Vec);

        // set index to leaf
        viskores::Id leafIndex = leafOffsets.Get(childVector[1]);
        childVector[1] = -(leafIndex + 1);
      }
      else
      {
        //our left child is an inner node, so gather
        //both AABBs in the child and join them for
        //the current node left AABB.
        viskores::Id child = childVector[1] * 4;

        viskores::Vec4f_32 cFirst4Vec = flatBVH.Get(child);
        viskores::Vec4f_32 cSecond4Vec = flatBVH.Get(child + 1);
        viskores::Vec4f_32 cThird4Vec = flatBVH.Get(child + 2);

        viskores::Vec4f_32 second4Vec = flatBVH.Get(currentNodeOffset + 1);
        second4Vec[2] = viskores::Min(cFirst4Vec[0], cSecond4Vec[2]);
        second4Vec[3] = viskores::Min(cFirst4Vec[1], cSecond4Vec[3]);
        flatBVH.Set(currentNodeOffset + 1, second4Vec);

        cThird4Vec[0] = viskores::Min(cFirst4Vec[2], cThird4Vec[0]);
        cThird4Vec[1] = viskores::Max(cFirst4Vec[3], cThird4Vec[1]);
        cThird4Vec[2] = viskores::Max(cSecond4Vec[0], cThird4Vec[2]);
        cThird4Vec[3] = viskores::Max(cSecond4Vec[1], cThird4Vec[3]);
        flatBVH.Set(currentNodeOffset + 2, cThird4Vec);
      }
      viskores::Vec4f_32 fourth4Vec{ 0.0f };
      viskores::Int32 leftChild =
        static_cast<viskores::Int32>((childVector[0] >= 0) ? childVector[0] * 4 : childVector[0]);
      memcpy(&fourth4Vec[0], &leftChild, 4);
      viskores::Int32 rightChild =
        static_cast<viskores::Int32>((childVector[1] >= 0) ? childVector[1] * 4 : childVector[1]);
      memcpy(&fourth4Vec[1], &rightChild, 4);
      flatBVH.Set(currentNodeOffset + 3, fourth4Vec);
    }
  }
}; //class PropagateAABBs

class LinearBVHBuilder::TreeBuilder : public viskores::worklet::WorkletMapField
{
private:
  viskores::Id LeafCount;
  viskores::Id InnerCount;
  //TODO: get intrinsic support
  VISKORES_EXEC
  inline viskores::Int32 CountLeadingZeros(viskores::UInt32& x) const
  {
    viskores::UInt32 y;
    viskores::UInt32 n = 32;
    y = x >> 16;
    if (y != 0)
    {
      n = n - 16;
      x = y;
    }
    y = x >> 8;
    if (y != 0)
    {
      n = n - 8;
      x = y;
    }
    y = x >> 4;
    if (y != 0)
    {
      n = n - 4;
      x = y;
    }
    y = x >> 2;
    if (y != 0)
    {
      n = n - 2;
      x = y;
    }
    y = x >> 1;
    if (y != 0)
      return viskores::Int32(n - 2);
    return viskores::Int32(n - x);
  }

  // returns the count of largest shared prefix between
  // two morton codes. Ties are broken by the indexes
  // a and b.
  //
  // returns count of the largest binary prefix

  template <typename MortonType>
  VISKORES_EXEC inline viskores::Int32 delta(const viskores::Int32& a,
                                             const viskores::Int32& b,
                                             const MortonType& mortonCodePortal) const
  {
    bool tie = false;
    bool outOfRange = (b < 0 || b > LeafCount - 1);
    //still make the call but with a valid adderss
    viskores::Int32 bb = (outOfRange) ? 0 : b;
    viskores::UInt32 aCode = mortonCodePortal.Get(a);
    viskores::UInt32 bCode = mortonCodePortal.Get(bb);
    //use xor to find where they differ
    viskores::UInt32 exOr = aCode ^ bCode;
    tie = (exOr == 0);
    //break the tie, a and b must always differ
    exOr = tie ? viskores::UInt32(a) ^ viskores::UInt32(bb) : exOr;
    viskores::Int32 count = CountLeadingZeros(exOr);
    if (tie)
      count += 32;
    count = (outOfRange) ? -1 : count;
    return count;
  }

public:
  VISKORES_CONT
  TreeBuilder(const viskores::Id& leafCount)
    : LeafCount(leafCount)
    , InnerCount(leafCount - 1)
  {
  }
  using ControlSignature = void(FieldOut, FieldOut, WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4);

  template <typename MortonType, typename ParentType>
  VISKORES_EXEC void operator()(const viskores::Id& index,
                                viskores::Id& leftChild,
                                viskores::Id& rightChild,
                                const MortonType& mortonCodePortal,
                                ParentType& parentPortal) const
  {
    viskores::Int32 idx = viskores::Int32(index);
    //determine range direction
    viskores::Int32 d =
      0 > (delta(idx, idx + 1, mortonCodePortal) - delta(idx, idx - 1, mortonCodePortal)) ? -1 : 1;

    //find upper bound for the length of the range
    viskores::Int32 minDelta = delta(idx, idx - d, mortonCodePortal);
    viskores::Int32 lMax = 2;
    while (delta(idx, idx + lMax * d, mortonCodePortal) > minDelta)
      lMax *= 2;

    //binary search to find the lower bound
    viskores::Int32 l = 0;
    for (int t = lMax / 2; t >= 1; t /= 2)
    {
      if (delta(idx, idx + (l + t) * d, mortonCodePortal) > minDelta)
        l += t;
    }

    viskores::Int32 j = idx + l * d;
    viskores::Int32 deltaNode = delta(idx, j, mortonCodePortal);
    viskores::Int32 s = 0;
    viskores::Float32 divFactor = 2.f;
    //find the split position using a binary search
    for (viskores::Int32 t = (viskores::Int32)ceil(viskores::Float32(l) / divFactor);;
         divFactor *= 2, t = (viskores::Int32)ceil(viskores::Float32(l) / divFactor))
    {
      if (delta(idx, idx + (s + t) * d, mortonCodePortal) > deltaNode)
      {
        s += t;
      }

      if (t == 1)
        break;
    }

    viskores::Int32 split = idx + s * d + viskores::Min(d, 0);
    //assign parent/child pointers
    if (viskores::Min(idx, j) == split)
    {
      //leaf
      parentPortal.Set(split + InnerCount, idx);
      leftChild = split + InnerCount;
    }
    else
    {
      //inner node
      parentPortal.Set(split, idx);
      leftChild = split;
    }


    if (viskores::Max(idx, j) == split + 1)
    {
      //leaf
      parentPortal.Set(split + InnerCount + 1, idx);
      rightChild = split + InnerCount + 1;
    }
    else
    {
      parentPortal.Set(split + 1, idx);
      rightChild = split + 1;
    }
  }
}; // class TreeBuilder

VISKORES_CONT void LinearBVHBuilder::SortAABBS(BVHData& bvh, bool singleAABB)
{
  //create array of indexes to be sorted with morton codes
  viskores::cont::ArrayHandle<viskores::Id> iterator;
  iterator.Allocate(bvh.GetNumberOfPrimitives());

  viskores::worklet::DispatcherMapField<CountingIterator> iterDispatcher;
  iterDispatcher.Invoke(iterator);

  //sort the morton codes

  viskores::cont::Algorithm::SortByKey(bvh.mortonCodes, iterator);

  viskores::Id arraySize = bvh.GetNumberOfPrimitives();
  viskores::cont::ArrayHandle<viskores::Float32> temp1;
  viskores::cont::ArrayHandle<viskores::Float32> temp2;
  temp1.Allocate(arraySize);

  viskores::worklet::DispatcherMapField<GatherFloat32> gatherDispatcher;

  //xmins
  gatherDispatcher.Invoke(iterator, bvh.AABB.xmins, temp1);

  temp2 = bvh.AABB.xmins;
  bvh.AABB.xmins = temp1;
  temp1 = temp2;
  //ymins
  gatherDispatcher.Invoke(iterator, bvh.AABB.ymins, temp1);

  temp2 = bvh.AABB.ymins;
  bvh.AABB.ymins = temp1;
  temp1 = temp2;
  //zmins
  gatherDispatcher.Invoke(iterator, bvh.AABB.zmins, temp1);

  temp2 = bvh.AABB.zmins;
  bvh.AABB.zmins = temp1;
  temp1 = temp2;
  //xmaxs
  gatherDispatcher.Invoke(iterator, bvh.AABB.xmaxs, temp1);

  temp2 = bvh.AABB.xmaxs;
  bvh.AABB.xmaxs = temp1;
  temp1 = temp2;
  //ymaxs
  gatherDispatcher.Invoke(iterator, bvh.AABB.ymaxs, temp1);

  temp2 = bvh.AABB.ymaxs;
  bvh.AABB.ymaxs = temp1;
  temp1 = temp2;
  //zmaxs
  gatherDispatcher.Invoke(iterator, bvh.AABB.zmaxs, temp1);

  temp2 = bvh.AABB.zmaxs;
  bvh.AABB.zmaxs = temp1;
  temp1 = temp2;

  // Create the leaf references
  bvh.leafs.Allocate(arraySize * 2);
  // we only actually have a single primitive, but the algorithm
  // requires 2. Make sure they both point to the original
  // primitive
  if (singleAABB)
  {
    auto iterPortal = iterator.WritePortal();
    for (int i = 0; i < 2; ++i)
    {
      iterPortal.Set(i, 0);
    }
  }

  viskores::worklet::DispatcherMapField<CreateLeafs> leafDispatcher;
  leafDispatcher.Invoke(iterator, bvh.leafs);

} // method SortAABB

VISKORES_CONT void LinearBVHBuilder::Build(LinearBVH& linearBVH)
{

  //
  //
  // This algorithm needs at least 2 AABBs
  //
  bool singleAABB = false;
  viskores::Id numberOfAABBs = linearBVH.GetNumberOfAABBs();
  if (numberOfAABBs == 1)
  {
    numberOfAABBs = 2;
    singleAABB = true;
    viskores::Float32 xmin = linearBVH.AABB.xmins.WritePortal().Get(0);
    viskores::Float32 ymin = linearBVH.AABB.ymins.WritePortal().Get(0);
    viskores::Float32 zmin = linearBVH.AABB.zmins.WritePortal().Get(0);
    viskores::Float32 xmax = linearBVH.AABB.xmaxs.WritePortal().Get(0);
    viskores::Float32 ymax = linearBVH.AABB.ymaxs.WritePortal().Get(0);
    viskores::Float32 zmax = linearBVH.AABB.zmaxs.WritePortal().Get(0);

    linearBVH.AABB.xmins.Allocate(2);
    linearBVH.AABB.ymins.Allocate(2);
    linearBVH.AABB.zmins.Allocate(2);
    linearBVH.AABB.xmaxs.Allocate(2);
    linearBVH.AABB.ymaxs.Allocate(2);
    linearBVH.AABB.zmaxs.Allocate(2);
    for (int i = 0; i < 2; ++i)
    {
      linearBVH.AABB.xmins.WritePortal().Set(i, xmin);
      linearBVH.AABB.ymins.WritePortal().Set(i, ymin);
      linearBVH.AABB.zmins.WritePortal().Set(i, zmin);
      linearBVH.AABB.xmaxs.WritePortal().Set(i, xmax);
      linearBVH.AABB.ymaxs.WritePortal().Set(i, ymax);
      linearBVH.AABB.zmaxs.WritePortal().Set(i, zmax);
    }
  }


  const viskores::Id numBBoxes = numberOfAABBs;
  BVHData bvh(numBBoxes, linearBVH.GetAABBs());


  // Find the extent of all bounding boxes to generate normalization for morton codes
  viskores::Vec3f_32 minExtent(
    viskores::Infinity32(), viskores::Infinity32(), viskores::Infinity32());
  viskores::Vec3f_32 maxExtent(
    viskores::NegativeInfinity32(), viskores::NegativeInfinity32(), viskores::NegativeInfinity32());
  maxExtent[0] = viskores::cont::Algorithm::Reduce(bvh.AABB.xmaxs, maxExtent[0], MaxValue());
  maxExtent[1] = viskores::cont::Algorithm::Reduce(bvh.AABB.ymaxs, maxExtent[1], MaxValue());
  maxExtent[2] = viskores::cont::Algorithm::Reduce(bvh.AABB.zmaxs, maxExtent[2], MaxValue());
  minExtent[0] = viskores::cont::Algorithm::Reduce(bvh.AABB.xmins, minExtent[0], MinValue());
  minExtent[1] = viskores::cont::Algorithm::Reduce(bvh.AABB.ymins, minExtent[1], MinValue());
  minExtent[2] = viskores::cont::Algorithm::Reduce(bvh.AABB.zmins, minExtent[2], MinValue());

  linearBVH.TotalBounds.X.Min = minExtent[0];
  linearBVH.TotalBounds.X.Max = maxExtent[0];
  linearBVH.TotalBounds.Y.Min = minExtent[1];
  linearBVH.TotalBounds.Y.Max = maxExtent[1];
  linearBVH.TotalBounds.Z.Min = minExtent[2];
  linearBVH.TotalBounds.Z.Max = maxExtent[2];

  viskores::Vec3f_32 deltaExtent = maxExtent - minExtent;
  viskores::Vec3f_32 inverseExtent;
  for (int i = 0; i < 3; ++i)
  {
    inverseExtent[i] = (deltaExtent[i] == 0.f) ? 0 : 1.f / deltaExtent[i];
  }

  //Generate the morton codes
  viskores::worklet::DispatcherMapField<MortonCodeAABB> mortonDispatch(
    MortonCodeAABB(inverseExtent, minExtent));
  mortonDispatch.Invoke(bvh.AABB.xmins,
                        bvh.AABB.ymins,
                        bvh.AABB.zmins,
                        bvh.AABB.xmaxs,
                        bvh.AABB.ymaxs,
                        bvh.AABB.zmaxs,
                        bvh.mortonCodes);
  linearBVH.Allocate(bvh.GetNumberOfPrimitives());

  SortAABBS(bvh, singleAABB);

  viskores::worklet::DispatcherMapField<TreeBuilder> treeDispatch(
    TreeBuilder(bvh.GetNumberOfPrimitives()));
  treeDispatch.Invoke(bvh.leftChild, bvh.rightChild, bvh.mortonCodes, bvh.parent);

  const viskores::Int32 primitiveCount = viskores::Int32(bvh.GetNumberOfPrimitives());

  viskores::cont::ArrayHandle<viskores::Int32> counters;
  counters.Allocate(bvh.GetNumberOfPrimitives() - 1);

  viskores::cont::ArrayHandleConstant<viskores::Int32> zero(0, bvh.GetNumberOfPrimitives() - 1);
  viskores::cont::Algorithm::Copy(zero, counters);

  viskores::worklet::DispatcherMapField<PropagateAABBs> propDispatch(
    PropagateAABBs{ primitiveCount });

  propDispatch.Invoke(bvh.AABB.xmins,
                      bvh.AABB.ymins,
                      bvh.AABB.zmins,
                      bvh.AABB.xmaxs,
                      bvh.AABB.ymaxs,
                      bvh.AABB.zmaxs,
                      bvh.leafOffsets,
                      bvh.parent,
                      bvh.leftChild,
                      bvh.rightChild,
                      counters,
                      linearBVH.FlatBVH);

  linearBVH.Leafs = bvh.leafs;
}
} //namespace detail

LinearBVH::LinearBVH()
  : IsConstructed(false)
  , CanConstruct(false){};

VISKORES_CONT
LinearBVH::LinearBVH(AABBs& aabbs)
  : AABB(aabbs)
  , IsConstructed(false)
  , CanConstruct(true)
{
}

VISKORES_CONT
LinearBVH::LinearBVH(const LinearBVH& other)
  : AABB(other.AABB)
  , FlatBVH(other.FlatBVH)
  , Leafs(other.Leafs)
  , LeafCount(other.LeafCount)
  , IsConstructed(other.IsConstructed)
  , CanConstruct(other.CanConstruct)
{
}

VISKORES_CONT void LinearBVH::Allocate(const viskores::Id& leafCount)
{
  LeafCount = leafCount;
  FlatBVH.Allocate((leafCount - 1) * 4);
}

void LinearBVH::Construct()
{
  if (IsConstructed)
    return;
  if (!CanConstruct)
    throw viskores::cont::ErrorBadValue(
      "Linear BVH: coordinates and triangles must be set before calling construct!");

  detail::LinearBVHBuilder builder;
  builder.Build(*this);
}

VISKORES_CONT
void LinearBVH::SetData(AABBs& aabbs)
{
  AABB = aabbs;
  IsConstructed = false;
  CanConstruct = true;
}

// explicitly export
//template VISKORES_RENDERING_EXPORT void LinearBVH::ConstructOnDevice<
//  viskores::cont::DeviceAdapterTagSerial>(viskores::cont::DeviceAdapterTagSerial);
//#ifdef VISKORES_ENABLE_TBB
//template VISKORES_RENDERING_EXPORT void LinearBVH::ConstructOnDevice<viskores::cont::DeviceAdapterTagTBB>(
//  viskores::cont::DeviceAdapterTagTBB);
//#endif
//#ifdef VISKORES_ENABLE_OPENMP
//template VISKORES_CONT_EXPORT void LinearBVH::ConstructOnDevice<viskores::cont::DeviceAdapterTagOpenMP>(
//  viskores::cont::DeviceAdapterTagOpenMP);
//#endif
//#ifdef VISKORES_ENABLE_CUDA
//template VISKORES_RENDERING_EXPORT void LinearBVH::ConstructOnDevice<viskores::cont::DeviceAdapterTagCuda>(
//  viskores::cont::DeviceAdapterTagCuda);
//#endif
//
VISKORES_CONT
bool LinearBVH::GetIsConstructed() const
{
  return IsConstructed;
}

viskores::Id LinearBVH::GetNumberOfAABBs() const
{
  return AABB.xmins.GetNumberOfValues();
}

AABBs& LinearBVH::GetAABBs()
{
  return AABB;
}
}
}
} // namespace viskores::rendering::raytracing
