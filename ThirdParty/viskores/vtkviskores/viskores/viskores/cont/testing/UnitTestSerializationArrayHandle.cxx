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
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/cont/ArrayHandleConcatenate.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleExtractComponent.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ArrayHandleImplicit.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleReverse.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/ArrayHandleSwizzle.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/ArrayHandleZip.h>

#include <viskores/cont/UncertainArrayHandle.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/cont/testing/TestingSerialization.h>

#include <viskores/VecTraits.h>

#include <ctime>
#include <type_traits>
#include <vector>

using namespace viskores::cont::testing::serialization;

namespace
{

using TestTypesListScalar = viskores::List<viskores::Int8, viskores::Id, viskores::FloatDefault>;
using TestTypesListVec = viskores::List<viskores::Vec3f_32, viskores::Vec3f_64>;
using TestTypesList = viskores::ListAppend<TestTypesListScalar, TestTypesListVec>;

using StorageListInefficientExtract =
  viskores::List<viskores::cont::StorageTagCast<viskores::Int8, viskores::cont::StorageTagBasic>,
                 viskores::cont::StorageTagConstant,
                 viskores::cont::StorageTagCounting,
                 viskores::cont::StorageTagIndex,
                 viskores::cont::StorageTagPermutation<viskores::cont::StorageTagBasic,
                                                       viskores::cont::StorageTagBasic>>;

//-----------------------------------------------------------------------------
struct TestEqualArrayHandle
{
public:
  template <typename ArrayHandle1, typename ArrayHandle2>
  VISKORES_CONT void operator()(const ArrayHandle1& array1, const ArrayHandle2& array2) const
  {
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(array1, array2));
  }

  template <typename TypeList, typename StorageList>
  VISKORES_CONT void operator()(
    const viskores::cont::UncertainArrayHandle<TypeList, StorageList>& array1,
    const viskores::cont::UnknownArrayHandle& array2) const
  {
    // This results in an excessive amount of compiling. However, we do it here to avoid
    // warnings about inefficient copies of the weirder arrays. That slowness might be OK
    // to test arrays, but we want to make sure that the serialization itself does not do
    // that.
    array1.CastAndCall(
      [array2](const auto& concreteArray1)
      {
        using ArrayType = std::decay_t<decltype(concreteArray1)>;
        ArrayType concreteArray2;
        array2.AsArrayHandle(concreteArray2);
        test_equal_ArrayHandles(concreteArray1, concreteArray2);
      });
  }

  template <typename TypeList1, typename StorageList1, typename TypeList2, typename StorageList2>
  VISKORES_CONT void operator()(
    const viskores::cont::UncertainArrayHandle<TypeList1, StorageList1>& array1,
    const viskores::cont::UncertainArrayHandle<TypeList2, StorageList2>& array2) const
  {
    (*this)(array1, viskores::cont::UnknownArrayHandle(array2));
  }

  VISKORES_CONT void operator()(const viskores::cont::UnknownArrayHandle& array1,
                                const viskores::cont::UnknownArrayHandle& array2) const
  {
    bool isInefficient = false;
    viskores::ListForEach(
      [&](auto type)
      {
        using StorageTag = std::decay_t<decltype(type)>;
        if (array1.IsStorageType<StorageTag>())
        {
          isInefficient = true;
        }
      },
      StorageListInefficientExtract{});

    if (isInefficient)
    {
      (*this)(array1.ResetTypes<TestTypesList, StorageListInefficientExtract>(), array2);
    }
    else
    {
      test_equal_ArrayHandles(array1, array2);
    }
  }
};

//-----------------------------------------------------------------------------
template <typename T>
inline void RunTest(const T& obj)
{
  TestSerialization(obj, TestEqualArrayHandle{});
}

//-----------------------------------------------------------------------------
constexpr viskores::Id ArraySize = 10;

template <typename T, typename S>
inline viskores::cont::UnknownArrayHandle MakeTestUnknownArrayHandle(
  const viskores::cont::ArrayHandle<T, S>& array)
{
  return array;
}

template <typename T, typename S>
inline viskores::cont::UncertainArrayHandle<viskores::List<T>, viskores::List<S>>
MakeTestUncertainArrayHandle(const viskores::cont::ArrayHandle<T, S>& array)
{
  return array;
}

struct TestArrayHandleBasic
{
  template <typename T>
  void operator()(T) const
  {
    auto array = RandomArrayHandle<T>::Make(ArraySize);
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleBasicEmpty
{
  template <typename T>
  void operator()(T) const
  {
    viskores::cont::ArrayHandle<T> array;
    array.Allocate(0);
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleSOA
{
  template <typename T>
  void operator()(T) const
  {
    viskores::cont::ArrayHandleSOA<T> array;
    viskores::cont::ArrayCopy(RandomArrayHandle<T>::Make(ArraySize), array);
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleCartesianProduct
{
  template <typename T>
  void operator()(T) const
  {
    auto array =
      viskores::cont::make_ArrayHandleCartesianProduct(RandomArrayHandle<T>::Make(ArraySize),
                                                       RandomArrayHandle<T>::Make(ArraySize),
                                                       RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleCast
{
  template <typename T>
  void operator()(T) const
  {
    auto array =
      viskores::cont::make_ArrayHandleCast<T>(RandomArrayHandle<viskores::Int8>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }

  template <typename T, viskores::IdComponent N>
  void operator()(viskores::Vec<T, N>) const
  {
    auto array = viskores::cont::make_ArrayHandleCast<viskores::Vec<T, N>>(
      RandomArrayHandle<viskores::Vec<viskores::Int8, N>>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleConstant
{
  template <typename T>
  void operator()(T) const
  {
    T cval = RandomValue<T>::Make();
    auto array = viskores::cont::make_ArrayHandleConstant(cval, ArraySize);
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleCounting
{
  template <typename T>
  void operator()(T) const
  {
    T start = RandomValue<T>::Make();
    T step = RandomValue<T>::Make(0, 5);
    auto array = viskores::cont::make_ArrayHandleCounting(start, step, ArraySize);
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleGroupVec
{
  template <typename T>
  void operator()(T) const
  {
    auto numComps = RandomValue<viskores::IdComponent>::Make(2, 4);
    auto flat = RandomArrayHandle<T>::Make(ArraySize * numComps);
    switch (numComps)
    {
      case 3:
      {
        auto array = viskores::cont::make_ArrayHandleGroupVec<3>(flat);
        RunTest(array);
        RunTest(MakeTestUnknownArrayHandle(array));
        RunTest(MakeTestUncertainArrayHandle(array));
        break;
      }
      case 4:
      {
        auto array = viskores::cont::make_ArrayHandleGroupVec<4>(flat);
        RunTest(array);
        RunTest(MakeTestUnknownArrayHandle(array));
        RunTest(MakeTestUncertainArrayHandle(array));
        break;
      }
      default:
      {
        auto array = viskores::cont::make_ArrayHandleGroupVec<2>(flat);
        RunTest(array);
        RunTest(MakeTestUnknownArrayHandle(array));
        RunTest(MakeTestUncertainArrayHandle(array));
        break;
      }
    }
  }
};

struct TestArrayHandleGroupVecVariable
{
  template <typename T>
  void operator()(T) const
  {
    auto rangen = UniformRandomValueGenerator<viskores::IdComponent>(1, 4);
    viskores::Id size = 0;

    std::vector<viskores::Id> comps(ArraySize);
    std::generate(comps.begin(),
                  comps.end(),
                  [&size, &rangen]()
                  {
                    auto offset = size;
                    size += rangen();
                    return offset;
                  });

    auto array = viskores::cont::make_ArrayHandleGroupVecVariable(
      RandomArrayHandle<T>::Make(size),
      viskores::cont::make_ArrayHandle(comps, viskores::CopyFlag::On));
    RunTest(array);

    // cannot make a UnknownArrayHandle containing ArrayHandleGroupVecVariable
    // because of the variable number of components of its values.
    // RunTest(MakeTestUnknownArrayHandle(array));
  }
};

struct TestArrayHandleRuntimeVec
{
  template <typename T>
  void operator()(T) const
  {
    auto numComps = RandomValue<viskores::IdComponent>::Make(1, 5);
    auto flat = RandomArrayHandle<T>::Make(ArraySize * numComps);
    auto array = viskores::cont::make_ArrayHandleRuntimeVec(numComps, flat);
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
  }
};

void TestArrayHandleIndex()
{
  auto size = RandomValue<viskores::Id>::Make(2, 10);
  auto array = viskores::cont::ArrayHandleIndex(size);
  RunTest(array);
  RunTest(MakeTestUnknownArrayHandle(array));
  RunTest(MakeTestUncertainArrayHandle(array));
}

struct TestArrayHandlePermutation
{
  template <typename T>
  void operator()(T) const
  {
    std::uniform_int_distribution<viskores::Id> distribution(0, ArraySize - 1);

    std::vector<viskores::Id> inds(ArraySize);
    std::generate(inds.begin(), inds.end(), [&distribution]() { return distribution(generator); });

    auto array = viskores::cont::make_ArrayHandlePermutation(
      RandomArrayHandle<viskores::Id>::Make(ArraySize, 0, ArraySize - 1),
      RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleReverse
{
  template <typename T>
  void operator()(T) const
  {
    auto array = viskores::cont::make_ArrayHandleReverse(RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleSwizzle
{
  template <typename T>
  void operator()(T) const
  {
    constexpr viskores::IdComponent NUM_COMPONENTS = viskores::VecTraits<T>::NUM_COMPONENTS;
    viskores::Vec<viskores::IdComponent, NUM_COMPONENTS> map;
    for (viskores::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      map[i] = NUM_COMPONENTS - (i + 1);
    }
    auto array =
      viskores::cont::make_ArrayHandleSwizzle(RandomArrayHandle<T>::Make(ArraySize), map);
    RunTest(array);
  }
};


viskores::cont::ArrayHandleUniformPointCoordinates MakeRandomArrayHandleUniformPointCoordinates()
{
  auto dimensions = RandomValue<viskores::Id3>::Make(1, 3);
  auto origin = RandomValue<viskores::Vec3f>::Make();
  auto spacing = RandomValue<viskores::Vec3f>::Make(0.1f, 10.0f);
  return viskores::cont::ArrayHandleUniformPointCoordinates(dimensions, origin, spacing);
}

void TestArrayHandleUniformPointCoordinates()
{
  auto array = MakeRandomArrayHandleUniformPointCoordinates();
  RunTest(array);
  RunTest(MakeTestUnknownArrayHandle(array));
  RunTest(MakeTestUncertainArrayHandle(array));
}


//-----------------------------------------------------------------------------
void TestArrayHandleSerialization()
{
  std::cout << "Testing ArrayHandleBasic\n";
  viskores::testing::Testing::TryTypes(TestArrayHandleBasic(), TestTypesList());
  viskores::testing::Testing::TryTypes(
    TestArrayHandleBasic(),
    viskores::List<char, long, long long, unsigned long, unsigned long long>());

  std::cout << "Testing empty ArrayHandleBasic\n";
  viskores::testing::Testing::TryTypes(TestArrayHandleBasicEmpty(), TestTypesList());
  viskores::testing::Testing::TryTypes(
    TestArrayHandleBasicEmpty(),
    viskores::List<char, long, long long, unsigned long, unsigned long long>());

  std::cout << "Testing ArrayHandleSOA\n";
  viskores::testing::Testing::TryTypes(TestArrayHandleSOA(), TestTypesListVec());

  std::cout << "Testing ArrayHandleCartesianProduct\n";
  viskores::testing::Testing::TryTypes(TestArrayHandleCartesianProduct(),
                                       viskores::List<viskores::Float32, viskores::Float64>());

  std::cout << "Testing TestArrayHandleCast\n";
  viskores::testing::Testing::TryTypes(TestArrayHandleCast(), TestTypesList());

  std::cout << "Testing ArrayHandleConstant\n";
  viskores::testing::Testing::TryTypes(TestArrayHandleConstant(), TestTypesList());

  std::cout << "Testing ArrayHandleCounting\n";
  viskores::testing::Testing::TryTypes(TestArrayHandleCounting(), TestTypesList());

  std::cout << "Testing ArrayHandleGroupVec\n";
  viskores::testing::Testing::TryTypes(TestArrayHandleGroupVec(), TestTypesListScalar());

  std::cout << "Testing ArrayHandleGroupVecVariable\n";
  viskores::testing::Testing::TryTypes(TestArrayHandleGroupVecVariable(), TestTypesList());

  std::cout << "Testing ArrayHandleRuntimeVec\n";
  viskores::testing::Testing::TryTypes(TestArrayHandleRuntimeVec(), TestTypesList());

  std::cout << "Testing ArrayHandleIndex\n";
  TestArrayHandleIndex();

  std::cout << "Testing ArrayHandlePermutation\n";
  viskores::testing::Testing::TryTypes(TestArrayHandlePermutation(), TestTypesList());

  std::cout << "Testing ArrayHandleReverse\n";
  viskores::testing::Testing::TryTypes(TestArrayHandleReverse(), TestTypesList());

  std::cout << "Testing ArrayHandleSwizzle\n";
  viskores::testing::Testing::TryTypes(TestArrayHandleSwizzle(), TestTypesList());

  std::cout << "Testing ArrayHandleUniformPointCoordinates\n";
  TestArrayHandleUniformPointCoordinates();
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestSerializationArrayHandle(int argc, char* argv[])
{
  // Normally Viskores `Testing::Run` would setup the diy MPI env,
  // but since we need to access it before execution we have
  // to manually set it  up
  viskoresdiy::mpi::environment env(argc, argv);
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();

  decltype(generator)::result_type seed = 0;
  if (comm.rank() == 0)
  {
    seed = static_cast<decltype(seed)>(std::time(nullptr));
    std::cout << "using seed: " << seed << "\n";
  }
  viskoresdiy::mpi::broadcast(comm, seed, 0);
  generator.seed(seed);

  return viskores::cont::testing::Testing::Run(TestArrayHandleSerialization, argc, argv);
}
