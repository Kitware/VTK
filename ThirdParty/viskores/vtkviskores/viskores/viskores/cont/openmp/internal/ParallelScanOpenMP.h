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

#include <viskores/cont/RuntimeDeviceInformation.h>
#include <viskores/cont/openmp/internal/DeviceAdapterTagOpenMP.h>
#include <viskores/cont/openmp/internal/FunctorsOpenMP.h>

#include <viskores/cont/internal/FunctorsGeneral.h>

#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>

#include <omp.h>

namespace viskores
{
namespace cont
{
namespace openmp
{
namespace scan
{

enum class ChildType
{
  Left,
  Right
};

// Generic implementation of modified Ladner & Fischer 1977 "adder" algorithm
// used for backbone of exclusive/inclusive scans. Language in comments is
// specific to computing a sum, but the implementation should be generic enough
// for any scan operation.
//
// The basic idea is that a tree structure is used to partition the input into
// sets of LeafSize. Each leaf of the tree is processed in two stages: First,
// the sum of each leaf is computed, and this information is pushed up the tree
// to compute the sum of each node's child leaves. Then the partial sum at the
// start of each node is computed and pushed down the tree (the "carry"
// values). In the second pass through each leaf's data, these partial sums are
// used to compute the final output from the carry value and the input data.
//
// The passes will likely overlap due to the "leftEdge" optimizations, which
// allow each leaf to start the second pass as soon as the first pass of all
// previous leaves is completed. Additionally, the first leaf in the data will
// combine both passes into one, computing the final output data while
// generating its sum for the communication stage.
template <typename ScanBody>
struct Adder : public ScanBody
{
  template <typename NodeImpl>
  struct NodeWrapper : public NodeImpl
  {
    // Range of IDs this node represents
    viskores::Id2 Range{ -1, -1 };

    // Connections:
    NodeWrapper* Parent{ nullptr };
    NodeWrapper* Left{ nullptr };
    NodeWrapper* Right{ nullptr };

    // Special flag to mark nodes on the far left edge of the tree. This allows
    // various optimization that start the second pass sooner on some ranges.
    bool LeftEdge{ false };

    // Pad the node out to the size of a cache line to prevent false sharing:
    static constexpr size_t DataSize =
      sizeof(NodeImpl) + sizeof(viskores::Id2) + 3 * sizeof(NodeWrapper*) + sizeof(bool);
    static constexpr size_t NumCacheLines = CeilDivide<size_t>(DataSize, VISKORES_CACHE_LINE_SIZE);
    static constexpr size_t PaddingSize = NumCacheLines * VISKORES_CACHE_LINE_SIZE - DataSize;
    unsigned char Padding[PaddingSize];
  };

  using Node = NodeWrapper<typename ScanBody::Node>;
  using ValueType = typename ScanBody::ValueType;

  viskores::Id LeafSize;
  std::vector<Node> Nodes;
  size_t NextNode;

  // Use ScanBody's ctor:
  using ScanBody::ScanBody;

  // Returns the total array sum:
  ValueType Execute(const viskores::Id2& range)
  {
    Node* rootNode = nullptr;

    VISKORES_OPENMP_DIRECTIVE(parallel default(shared))
    {
      VISKORES_OPENMP_DIRECTIVE(single)
      {
        // Allocate nodes, prep metadata:
        this->Prepare(range);

        // Compute the partition and node sums:
        rootNode = this->AllocNode();
        rootNode->Range = range;
        rootNode->LeftEdge = true;
        ScanBody::InitializeRootNode(rootNode);

        this->Scan(rootNode);
      } // end single
    }   // end parallel

    return rootNode ? ScanBody::GetFinalResult(rootNode)
                    : viskores::TypeTraits<ValueType>::ZeroInitialization();
  }

private:
  // Returns the next available node in a thread-safe manner.
  Node* AllocNode()
  {
    size_t nodeIdx;

// GCC emits a false positive "value computed but not used" for this block:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"

    VISKORES_OPENMP_DIRECTIVE(atomic capture)
    {
      nodeIdx = this->NextNode;
      ++this->NextNode;
    }

#pragma GCC diagnostic pop

    VISKORES_ASSERT(nodeIdx < this->Nodes.size());

    return &this->Nodes[nodeIdx];
  }

  // Does the range represent a leave node?
  bool IsLeaf(const viskores::Id2& range) const { return (range[1] - range[0]) <= this->LeafSize; }

  // Use to split ranges. Ensures that the first range is always a multiple of
  // LeafSize, when possible.
  viskores::Id ComputeMidpoint(const viskores::Id2& range) const
  {
    const viskores::Id n = range[1] - range[0];
    const viskores::Id np = this->LeafSize;

    return (((n / 2) + (np - 1)) / np) * np + range[0];
  }

  void Prepare(const viskores::Id2& range)
  {
    // Figure out how many values each thread should handle:
    viskores::Id numVals = range[1] - range[0];
    viskores::Id numThreads = 0;
    viskores::cont::RuntimeDeviceInformation{}
      .GetRuntimeConfiguration(viskores::cont::DeviceAdapterTagOpenMP())
      .GetThreads(numThreads);
    viskores::Id chunksPerThread = 8;
    viskores::Id numChunks;
    ComputeChunkSize(
      numVals, numThreads, chunksPerThread, sizeof(ValueType), numChunks, this->LeafSize);

    // Compute an upper-bound of the number of nodes in the tree:
    std::size_t numNodes = static_cast<std::size_t>(numChunks);
    while (numChunks > 1)
    {
      numChunks = (numChunks + 1) / 2;
      numNodes += static_cast<std::size_t>(numChunks);
    }
    this->Nodes.resize(numNodes);
    this->NextNode = 0;
  }

  // Build the tree and compute the sums:
  void Scan(Node* node)
  {
    if (!this->IsLeaf(node->Range))
    { // split range:
      viskores::Id midpoint = this->ComputeMidpoint(node->Range);

      Node* right = this->AllocNode();
      right->Parent = node;
      node->Right = right;
      right->Range = viskores::Id2(midpoint, node->Range[1]);
      ScanBody::InitializeChildNode(right, node, ChildType::Right, false);

      // Intel compilers seem to have trouble following the 'this' pointer
      // when launching tasks, resulting in a corrupt task environment.
      // Explicitly copying the pointer into a local variable seems to fix this.
      auto explicitThis = this;

      VISKORES_OPENMP_DIRECTIVE(taskgroup)
      {
        VISKORES_OPENMP_DIRECTIVE(task)
        {
          explicitThis->Scan(right);
        } // end right task

        Node* left = this->AllocNode();
        left->Parent = node;
        node->Left = left;
        left->Range = viskores::Id2(node->Range[0], midpoint);
        left->LeftEdge = node->LeftEdge;
        ScanBody::InitializeChildNode(left, node, ChildType::Left, left->LeftEdge);
        this->Scan(left);

      } // end task group. Both l/r sums will be finished here.

      ScanBody::CombineSummaries(node, node->Left, node->Right);
      if (node->LeftEdge)
      {
        this->UpdateOutput(node);
      }
    }
    else
    { // Compute sums:
      ScanBody::ComputeSummary(node, node->Range, node->LeftEdge);
    }
  }

  void UpdateOutput(Node* node)
  {
    if (node->Left != nullptr)
    {
      assert(node->Right != nullptr);
      ScanBody::PropagateSummaries(node, node->Left, node->Right, node->LeftEdge);

      // if this node is on the left edge, we know that the left child's
      // output is already updated, so only descend to the right:
      if (node->LeftEdge)
      {
        this->UpdateOutput(node->Right);
      }
      else // Otherwise descent into both:
      {
        // Intel compilers seem to have trouble following the 'this' pointer
        // when launching tasks, resulting in a corrupt task environment.
        // Explicitly copying the pointer into a local variable seems to fix
        // this.
        auto explicitThis = this;

        // no taskgroup/sync needed other than the final barrier of the parallel
        // section.
        VISKORES_OPENMP_DIRECTIVE(task)
        {
          explicitThis->UpdateOutput(node->Right);
        } // end task
        this->UpdateOutput(node->Left);
      }
    }
    else
    {
      ScanBody::UpdateOutput(node, node->Range, node->LeftEdge);
    }
  }
};

template <typename InPortalT, typename OutPortalT, typename RawFunctorT>
struct ScanExclusiveBody
{
  using ValueType = typename InPortalT::ValueType;
  using FunctorType = internal::WrappedBinaryOperator<ValueType, RawFunctorT>;

  InPortalT InPortal;
  OutPortalT OutPortal;
  FunctorType Functor;
  ValueType InitialValue;

  struct Node
  {
    // Sum of all values in range
    ValueType Sum{ viskores::TypeTraits<ValueType>::ZeroInitialization() };

    // The sum of all elements prior to this node's range
    ValueType Carry{ viskores::TypeTraits<ValueType>::ZeroInitialization() };
  };

  ScanExclusiveBody(const InPortalT& inPortal,
                    const OutPortalT& outPortal,
                    const RawFunctorT& functor,
                    const ValueType& init)
    : InPortal(inPortal)
    , OutPortal(outPortal)
    , Functor(functor)
    , InitialValue(init)
  {
  }

  // Initialize the root of the node tree
  void InitializeRootNode(Node* /*root*/) {}

  void InitializeChildNode(Node* /*node*/,
                           const Node* /*parent*/,
                           ChildType /*type*/,
                           bool /*leftEdge*/)
  {
  }

  void ComputeSummary(Node* node, const viskores::Id2& range, bool leftEdge)
  {
    auto input = viskores::cont::ArrayPortalToIteratorBegin(this->InPortal);
    node->Sum = input[range[0]];

    // If this block is on the left edge, we can update the output while we
    // compute the sum:
    if (leftEdge)
    {
      // Set leftEdge arg to false to force the update:
      node->Sum = UpdateOutputImpl(node, range, false, true);
    }
    else // Otherwise, only compute the sum and update the output in pass 2.
    {
      for (viskores::Id i = range[0] + 1; i < range[1]; ++i)
      {
        node->Sum = this->Functor(node->Sum, input[i]);
      }
    }
  }

  void CombineSummaries(Node* parent, const Node* left, const Node* right)
  {
    parent->Sum = this->Functor(left->Sum, right->Sum);
  }

  void PropagateSummaries(const Node* parent, Node* left, Node* right, bool leftEdge)
  {
    left->Carry = parent->Carry;
    right->Carry = leftEdge ? left->Sum : this->Functor(parent->Carry, left->Sum);
  }

  void UpdateOutput(const Node* node, const viskores::Id2& range, bool leftEdge)
  {
    this->UpdateOutputImpl(node, range, leftEdge, false);
  }

  ValueType UpdateOutputImpl(const Node* node, const viskores::Id2& range, bool skip, bool useInit)
  {
    if (skip)
    {
      // Do nothing; this was already done in ComputeSummary.
      return viskores::TypeTraits<ValueType>::ZeroInitialization();
    }

    auto input = viskores::cont::ArrayPortalToIteratorBegin(this->InPortal);
    auto output = viskores::cont::ArrayPortalToIteratorBegin(this->OutPortal);

    // Be careful with the order input/output are modified. They might be
    // pointing at the same data:
    ValueType carry = useInit ? this->InitialValue : node->Carry;
    viskores::Id end = range[1];

    for (viskores::Id i = range[0]; i < end; ++i)
    {
      output[i] = this->Functor(carry, input[i]);

      using std::swap; // Enable ADL
      swap(output[i], carry);
    }

    return carry;
  }

  // Compute the final sum from the node's metadata:
  ValueType GetFinalResult(const Node* node) const { return this->Functor(node->Sum, node->Carry); }
};

template <typename InPortalT, typename OutPortalT, typename RawFunctorT>
struct ScanInclusiveBody
{
  using ValueType = typename InPortalT::ValueType;
  using FunctorType = internal::WrappedBinaryOperator<ValueType, RawFunctorT>;

  InPortalT InPortal;
  OutPortalT OutPortal;
  FunctorType Functor;

  struct Node
  {
    // Sum of all values in range
    ValueType Sum{ viskores::TypeTraits<ValueType>::ZeroInitialization() };

    // The sum of all elements prior to this node's range
    ValueType Carry{ viskores::TypeTraits<ValueType>::ZeroInitialization() };
  };

  ScanInclusiveBody(const InPortalT& inPortal,
                    const OutPortalT& outPortal,
                    const RawFunctorT& functor)
    : InPortal(inPortal)
    , OutPortal(outPortal)
    , Functor(functor)
  {
  }

  // Initialize the root of the node tree
  void InitializeRootNode(Node*)
  {
    // no-op
  }

  void InitializeChildNode(Node*, const Node*, ChildType, bool)
  {
    // no-op
  }

  void ComputeSummary(Node* node, const viskores::Id2& range, bool leftEdge)
  {
    // If this block is on the left edge, we can update the output while we
    // compute the sum:
    if (leftEdge)
    {
      node->Sum = UpdateOutputImpl(node, range, false, false);
    }
    else // Otherwise, only compute the sum and update the output in pass 2.
    {
      auto input = viskores::cont::ArrayPortalToIteratorBegin(this->InPortal);
      node->Sum = input[range[0]];
      for (viskores::Id i = range[0] + 1; i < range[1]; ++i)
      {
        node->Sum = this->Functor(node->Sum, input[i]);
      }
    }
  }

  void CombineSummaries(Node* parent, const Node* left, const Node* right)
  {
    parent->Sum = this->Functor(left->Sum, right->Sum);
  }

  void PropagateSummaries(const Node* parent, Node* left, Node* right, bool leftEdge)
  {
    left->Carry = parent->Carry;
    right->Carry = leftEdge ? left->Sum : this->Functor(parent->Carry, left->Sum);
  }

  void UpdateOutput(const Node* node, const viskores::Id2& range, bool leftEdge)
  {
    UpdateOutputImpl(node, range, leftEdge, true);
  }

  ValueType UpdateOutputImpl(const Node* node, const viskores::Id2& range, bool skip, bool useCarry)
  {
    if (skip)
    {
      // Do nothing; this was already done in ComputeSummary.
      return viskores::TypeTraits<ValueType>::ZeroInitialization();
    }

    auto input = viskores::cont::ArrayPortalToIteratorBegin(this->InPortal);
    auto output = viskores::cont::ArrayPortalToIteratorBegin(this->OutPortal);

    viskores::Id start = range[0];
    viskores::Id end = range[1];
    ValueType carry = node->Carry;

    // Initialize with the first value if this is the first range:
    if (!useCarry && start < end)
    {
      carry = input[start];
      output[start] = carry;
      ++start;
    }

    for (viskores::Id i = start; i < end; ++i)
    {
      output[i] = this->Functor(carry, input[i]);
      carry = output[i];
    }

    return output[end - 1];
  }

  // Compute the final sum from the node's metadata:
  ValueType GetFinalResult(const Node* node) const { return node->Sum; }
};

} // end namespace scan

template <typename InPortalT, typename OutPortalT, typename FunctorT>
using ScanExclusiveHelper = scan::Adder<scan::ScanExclusiveBody<InPortalT, OutPortalT, FunctorT>>;

template <typename InPortalT, typename OutPortalT, typename FunctorT>
using ScanInclusiveHelper = scan::Adder<scan::ScanInclusiveBody<InPortalT, OutPortalT, FunctorT>>;
}
}
} // end namespace viskores::cont::openmp
