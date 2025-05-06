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

#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/connected_components/worklet/GraphConnectivity.h>

class AdjacentDifference : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn index, WholeArrayIn counts, FieldOut outputCount);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename WholeArrayType>
  VISKORES_EXEC void operator()(const viskores::Id& index,
                                const WholeArrayType& counts,
                                int& difference) const
  {
    difference = counts.Get(index + 1) - counts.Get(index);
  }
};

class SameComponent : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn start,
                                FieldIn degree,
                                WholeArrayIn conns,
                                WholeArrayIn comps,
                                AtomicArrayInOut same);
  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4, _5);

  template <typename Conn, typename Comp, typename AtomicSame>
  VISKORES_EXEC void operator()(viskores::Id index,
                                int start,
                                int degree,
                                const Conn& conns,
                                const Comp& comps,
                                AtomicSame& same) const
  {
    for (viskores::Id offset = start; offset < start + degree; ++offset)
    {
      viskores::Id neighbor = conns.Get(offset);
      if (comps.Get(index) != comps.Get(neighbor))
      {
        same.Set(0, 0);
      }
    }
  }
};

class TestGraphConnectivity
{
public:
  void TestECL_CC(const std::string& filename, int ncomps) const
  {
    auto pathname =
      viskores::cont::testing::Testing::GetTestDataBasePath() + "/third_party/ecl_cc/" + filename;
    std::ifstream stream(pathname, std::ios_base::in | std::ios_base::binary);

    int nnodes;
    stream.read(reinterpret_cast<char*>(&nnodes), sizeof(nnodes));

    int nedges;
    stream.read(reinterpret_cast<char*>(&nedges), sizeof(nedges));

    // CSR, there is one more element in offsets than the actual number of nodes.
    std::vector<int> offsets(nnodes + 1);
    std::vector<int> conns(nedges);

    stream.read(reinterpret_cast<char*>(offsets.data()), (nnodes + 1) * sizeof(int));
    stream.read(reinterpret_cast<char*>(conns.data()), nedges * sizeof(int));

    viskores::cont::ArrayHandle<int> counts_h;
    viskores::cont::Invoker invoke;
    invoke(AdjacentDifference{},
           viskores::cont::make_ArrayHandleCounting(0, 1, nnodes),
           viskores::cont::make_ArrayHandle<int>(offsets, viskores::CopyFlag::On),
           counts_h);

    offsets.pop_back();
    viskores::cont::ArrayHandle<int> offsets_h =
      viskores::cont::make_ArrayHandle(offsets, viskores::CopyFlag::On);

    viskores::cont::ArrayHandle<int> conns_h =
      viskores::cont::make_ArrayHandle(conns, viskores::CopyFlag::Off);

    viskores::cont::ArrayHandle<viskores::Id> comps_h;
    viskores::worklet::connectivity::GraphConnectivity::Run(counts_h, offsets_h, conns_h, comps_h);

    VISKORES_TEST_ASSERT(viskores::cont::Algorithm::Reduce(
                           comps_h, viskores::Id(0), viskores::Maximum{}) == ncomps - 1,
                         "number of components mismatch");

    viskores::cont::ArrayHandle<viskores::UInt32> atomicSame;
    atomicSame.Allocate(1);
    atomicSame.WritePortal().Set(0, 1);

    invoke(SameComponent{}, offsets_h, counts_h, conns_h, comps_h, atomicSame);
    VISKORES_TEST_ASSERT(atomicSame.ReadPortal().Get(0) == 1,
                         "Neighboring nodes don't have the same component id");
  }

  void TestECL_CC_DataSets() const { TestECL_CC("internet.egr", 1); }

  void TestSimpleGraph() const
  {
    viskores::cont::ArrayHandle<viskores::Id> counts_h =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 2, 2, 2 });
    viskores::cont::ArrayHandle<viskores::Id> offsets_h =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 2, 4, 6 });
    viskores::cont::ArrayHandle<viskores::Id> conn_h =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 2, 4, 0, 3, 2, 4, 1, 3 });
    viskores::cont::ArrayHandle<viskores::Id> comps;

    viskores::worklet::connectivity::GraphConnectivity().Run(counts_h, offsets_h, conn_h, comps);

    for (int i = 0; i < comps.GetNumberOfValues(); i++)
    {
      VISKORES_TEST_ASSERT(comps.ReadPortal().Get(i) == 0, "Components has unexpected value.");
    }
  }

  void operator()() const
  {
    TestSimpleGraph();
    TestECL_CC_DataSets();
  }
};

int UnitTestGraphConnectivityWorklet(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestGraphConnectivity{}, argc, argv);
}
