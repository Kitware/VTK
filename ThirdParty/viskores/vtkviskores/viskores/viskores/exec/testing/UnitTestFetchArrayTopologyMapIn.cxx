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

#include <viskores/testing/Testing.h>

#include <viskores/exec/arg/FetchTagArrayTopologyMapIn.h>
#include <viskores/exec/arg/ThreadIndicesTopologyMap.h>
#include <viskores/internal/FunctionInterface.h>
#include <viskores/internal/Invocation.h>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

template <typename T>
struct TestPortal
{
  using ValueType = T;

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    VISKORES_TEST_ASSERT(index >= 0, "Bad portal index.");
    VISKORES_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    return TestValue(index, ValueType());
  }
};

struct TestIndexPortal
{
  using ValueType = viskores::Id;

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const { return index; }
};

struct TestZeroPortal
{
  using ValueType = viskores::IdComponent;

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id) const { return 0; }
};

template <viskores::IdComponent IndexToReplace, typename U>
struct replace
{
  U theReplacement;

  template <typename T, viskores::IdComponent Index>
  struct ReturnType
  {
    using type = typename std::conditional<Index == IndexToReplace, U, T>::type;
  };

  template <typename T, viskores::IdComponent Index>
  VISKORES_CONT typename ReturnType<T, Index>::type operator()(
    T&& x,
    viskores::internal::IndexTag<Index>) const
  {
    return x;
  }

  template <typename T>
  VISKORES_CONT U operator()(T&&, viskores::internal::IndexTag<IndexToReplace>) const
  {
    return theReplacement;
  }
};


template <viskores::IdComponent InputDomainIndex, viskores::IdComponent ParamIndex, typename T>
struct FetchArrayTopologyMapInTests
{

  template <typename Invocation>
  void TryInvocation(const Invocation& invocation) const
  {
    using ConnectivityType = typename Invocation::InputDomainType;
    using ThreadIndicesType =
      viskores::exec::arg::ThreadIndicesTopologyMap<ConnectivityType,
                                                    viskores::exec::arg::CustomScatterOrMaskTag>;

    using FetchType = viskores::exec::arg::Fetch<viskores::exec::arg::FetchTagArrayTopologyMapIn,
                                                 viskores::exec::arg::AspectTagDefault,
                                                 TestPortal<T>>;

    FetchType fetch;

    const viskores::Id threadIndex = 0;
    const viskores::Id outputIndex = invocation.ThreadToOutputMap.Get(threadIndex);
    const viskores::Id inputIndex = invocation.OutputToInputMap.Get(outputIndex);
    const viskores::IdComponent visitIndex = invocation.VisitArray.Get(outputIndex);
    ThreadIndicesType indices(
      threadIndex, inputIndex, visitIndex, outputIndex, invocation.GetInputDomain());

    auto value =
      fetch.Load(indices, viskores::internal::ParameterGet<ParamIndex>(invocation.Parameters));
    VISKORES_TEST_ASSERT(value.GetNumberOfComponents() == 8,
                         "Topology fetch got wrong number of components.");

    VISKORES_TEST_ASSERT(test_equal(value[0], TestValue(0, T())), "Got invalid value from Load.");
    VISKORES_TEST_ASSERT(test_equal(value[1], TestValue(1, T())), "Got invalid value from Load.");
    VISKORES_TEST_ASSERT(test_equal(value[2], TestValue(3, T())), "Got invalid value from Load.");
    VISKORES_TEST_ASSERT(test_equal(value[3], TestValue(2, T())), "Got invalid value from Load.");
    VISKORES_TEST_ASSERT(test_equal(value[4], TestValue(4, T())), "Got invalid value from Load.");
    VISKORES_TEST_ASSERT(test_equal(value[5], TestValue(5, T())), "Got invalid value from Load.");
    VISKORES_TEST_ASSERT(test_equal(value[6], TestValue(7, T())), "Got invalid value from Load.");
    VISKORES_TEST_ASSERT(test_equal(value[7], TestValue(6, T())), "Got invalid value from Load.");
  }

  void operator()() const
  {
    std::cout << "Trying ArrayTopologyMapIn fetch on parameter " << ParamIndex << " with type "
              << viskores::testing::TypeName<T>::Name() << std::endl;

    viskores::internal::ConnectivityStructuredInternals<3> connectivityInternals;
    connectivityInternals.SetPointDimensions(viskores::Id3(2, 2, 2));
    viskores::exec::
      ConnectivityStructured<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint, 3>
        connectivity(connectivityInternals);

    using NullType = viskores::internal::NullType;
    auto baseFunctionInterface = viskores::internal::make_FunctionInterface<void>(
      NullType{}, NullType{}, NullType{}, NullType{}, NullType{});

    replace<InputDomainIndex, decltype(connectivity)> connReplaceFunctor{ connectivity };
    replace<ParamIndex, TestPortal<T>> portalReplaceFunctor{ TestPortal<T>{} };

    auto updatedInterface = baseFunctionInterface.StaticTransformCont(connReplaceFunctor)
                              .StaticTransformCont(portalReplaceFunctor);

    this->TryInvocation(viskores::internal::make_Invocation<InputDomainIndex>(updatedInterface,
                                                                              baseFunctionInterface,
                                                                              baseFunctionInterface,
                                                                              TestIndexPortal(),
                                                                              TestZeroPortal(),
                                                                              TestIndexPortal()));
  }
};


struct TryType
{
  template <typename T>
  void operator()(T) const
  {
    FetchArrayTopologyMapInTests<3, 1, T>()();
    FetchArrayTopologyMapInTests<1, 2, T>()();
    FetchArrayTopologyMapInTests<2, 3, T>()();
    FetchArrayTopologyMapInTests<1, 4, T>()();
    FetchArrayTopologyMapInTests<1, 5, T>()();
  }
};

template <viskores::IdComponent NumDimensions,
          viskores::IdComponent ParamIndex,
          typename Invocation>
void TryStructuredPointCoordinatesInvocation(const Invocation& invocation)
{
  using ConnectivityType = typename Invocation::InputDomainType;
  using ThreadIndicesType =
    viskores::exec::arg::ThreadIndicesTopologyMap<ConnectivityType,
                                                  viskores::exec::arg::CustomScatterOrMaskTag>;

  viskores::exec::arg::Fetch<viskores::exec::arg::FetchTagArrayTopologyMapIn,
                             viskores::exec::arg::AspectTagDefault,
                             viskores::internal::ArrayPortalUniformPointCoordinates>
    fetch;

  viskores::Vec3f origin = TestValue(0, viskores::Vec3f());
  viskores::Vec3f spacing = TestValue(1, viskores::Vec3f());

  {
    const viskores::Id threadIndex = 0;
    const viskores::Id outputIndex = invocation.ThreadToOutputMap.Get(threadIndex);
    const viskores::Id inputIndex = invocation.OutputToInputMap.Get(outputIndex);
    const viskores::IdComponent visitIndex = invocation.VisitArray.Get(outputIndex);
    viskores::VecAxisAlignedPointCoordinates<NumDimensions> value =
      fetch.Load(ThreadIndicesType(
                   threadIndex, inputIndex, visitIndex, outputIndex, invocation.GetInputDomain()),
                 viskores::internal::ParameterGet<ParamIndex>(invocation.Parameters));
    VISKORES_TEST_ASSERT(test_equal(value.GetOrigin(), origin), "Bad origin.");
    VISKORES_TEST_ASSERT(test_equal(value.GetSpacing(), spacing), "Bad spacing.");
  }

  origin[0] += spacing[0];
  {
    const viskores::Id threadIndex = 1;
    const viskores::Id outputIndex = invocation.ThreadToOutputMap.Get(threadIndex);
    const viskores::Id inputIndex = invocation.OutputToInputMap.Get(outputIndex);
    const viskores::IdComponent visitIndex = invocation.VisitArray.Get(outputIndex);
    viskores::VecAxisAlignedPointCoordinates<NumDimensions> value =
      fetch.Load(ThreadIndicesType(
                   threadIndex, inputIndex, visitIndex, outputIndex, invocation.GetInputDomain()),
                 viskores::internal::ParameterGet<ParamIndex>(invocation.Parameters));
    VISKORES_TEST_ASSERT(test_equal(value.GetOrigin(), origin), "Bad origin.");
    VISKORES_TEST_ASSERT(test_equal(value.GetSpacing(), spacing), "Bad spacing.");
  }
}

template <viskores::IdComponent NumDimensions>
void TryStructuredPointCoordinates(
  const viskores::exec::ConnectivityStructured<viskores::TopologyElementTagCell,
                                               viskores::TopologyElementTagPoint,
                                               NumDimensions>& connectivity,
  const viskores::internal::ArrayPortalUniformPointCoordinates& coordinates)
{
  using NullType = viskores::internal::NullType;

  auto baseFunctionInterface = viskores::internal::make_FunctionInterface<void>(
    NullType{}, NullType{}, NullType{}, NullType{}, NullType{});

  auto firstFunctionInterface = viskores::internal::make_FunctionInterface<void>(
    connectivity, coordinates, NullType{}, NullType{}, NullType{});

  // Try with topology in argument 1 and point coordinates in argument 2
  TryStructuredPointCoordinatesInvocation<NumDimensions, 2>(
    viskores::internal::make_Invocation<1>(firstFunctionInterface,
                                           baseFunctionInterface,
                                           baseFunctionInterface,
                                           TestIndexPortal(),
                                           TestZeroPortal(),
                                           TestIndexPortal()));

  // Try again with topology in argument 3 and point coordinates in argument 1
  auto secondFunctionInterface = viskores::internal::make_FunctionInterface<void>(
    coordinates, NullType{}, connectivity, NullType{}, NullType{});

  TryStructuredPointCoordinatesInvocation<NumDimensions, 1>(
    viskores::internal::make_Invocation<3>(secondFunctionInterface,
                                           baseFunctionInterface,
                                           baseFunctionInterface,
                                           TestIndexPortal(),
                                           TestZeroPortal(),
                                           TestIndexPortal()));
}

void TryStructuredPointCoordinates()
{
  std::cout << "*** Fetching special case of uniform point coordinates. *****" << std::endl;

  viskores::internal::ArrayPortalUniformPointCoordinates coordinates(
    viskores::Id3(3, 2, 2), TestValue(0, viskores::Vec3f()), TestValue(1, viskores::Vec3f()));

  std::cout << "3D" << std::endl;
  viskores::internal::ConnectivityStructuredInternals<3> connectivityInternals3d;
  connectivityInternals3d.SetPointDimensions(viskores::Id3(3, 2, 2));
  viskores::exec::
    ConnectivityStructured<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint, 3>
      connectivity3d(connectivityInternals3d);
  TryStructuredPointCoordinates(connectivity3d, coordinates);

  std::cout << "2D" << std::endl;
  viskores::internal::ConnectivityStructuredInternals<2> connectivityInternals2d;
  connectivityInternals2d.SetPointDimensions(viskores::Id2(3, 2));
  viskores::exec::
    ConnectivityStructured<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint, 2>
      connectivity2d(connectivityInternals2d);
  TryStructuredPointCoordinates(connectivity2d, coordinates);

  std::cout << "1D" << std::endl;
  viskores::internal::ConnectivityStructuredInternals<1> connectivityInternals1d;
  connectivityInternals1d.SetPointDimensions(3);
  viskores::exec::
    ConnectivityStructured<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint, 1>
      connectivity1d(connectivityInternals1d);
  TryStructuredPointCoordinates(connectivity1d, coordinates);
}

void TestArrayTopologyMapIn()
{
  viskores::testing::Testing::TryTypes(TryType(), viskores::TypeListCommon());

  TryStructuredPointCoordinates();
}

} // anonymous namespace

int UnitTestFetchArrayTopologyMapIn(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestArrayTopologyMapIn, argc, argv);
}
