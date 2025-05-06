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

#include <viskores/cont/UncertainArrayHandle.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ArrayHandleMultiplexer.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>

#include <viskores/TypeTraits.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

// Make an "unusual" type to use in the test. This is simply a type that
// is sure not to be declared elsewhere.
struct UnusualType
{
  using T = viskores::Id;
  T X;
  UnusualType() = default;
  UnusualType(T x)
    : X(x)
  {
  }
  UnusualType& operator=(T x)
  {
    this->X = x;
    return *this;
  }
  operator T() const { return this->X; }
};

const viskores::Id ARRAY_SIZE = 10;

struct CheckFunctor
{
  template <typename T, typename S>
  static void CheckArray(const viskores::cont::ArrayHandle<T, S>& array)
  {
    VISKORES_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Unexpected array size.");
    CheckPortal(array.ReadPortal());
  }

  template <typename S>
  static void CheckArray(const viskores::cont::ArrayHandle<UnusualType, S>& array)
  {
    VISKORES_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Unexpected array size.");
    auto portal = array.ReadPortal();
    for (viskores::Id index = 0; index < array.GetNumberOfValues(); ++index)
    {
      VISKORES_TEST_ASSERT(portal.Get(index) == TestValue(index, UnusualType::T{}));
    }
  }

  template <typename T, typename S>
  void operator()(const viskores::cont::ArrayHandle<T, S>& array, bool& called) const
  {
    called = true;
    std::cout << "  Checking for array type " << viskores::cont::TypeToString<T>()
              << " with storage " << viskores::cont::TypeToString<S>() << std::endl;

    CheckArray(array);
  }
};

void BasicUnknownArrayChecks(const viskores::cont::UnknownArrayHandle& array,
                             viskores::IdComponent numComponents)
{
  std::cout << "  Checking an UnknownArrayHandle containing " << array.GetArrayTypeName()
            << std::endl;
  VISKORES_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE,
                       "Dynamic array reports unexpected size.");
  VISKORES_TEST_ASSERT(array.GetNumberOfComponentsFlat() == numComponents,
                       "Dynamic array reports unexpected number of components.");
}

void CheckUnknownArrayDefaults(const viskores::cont::UnknownArrayHandle& array,
                               viskores::IdComponent numComponents)
{
  BasicUnknownArrayChecks(array, numComponents);

  std::cout << "  CastAndCall with default types" << std::endl;
  bool called = false;
  viskores::cont::CastAndCall(array, CheckFunctor(), called);
}

template <typename TypeList, typename StorageList>
void CheckUnknownArray(const viskores::cont::UnknownArrayHandle& array,
                       viskores::IdComponent numComponents)
{
  VISKORES_IS_LIST(TypeList);
  VISKORES_IS_LIST(StorageList);

  BasicUnknownArrayChecks(array, numComponents);

  std::cout << "  CastAndCall with given types" << std::endl;
  bool called = false;
  array.CastAndCallForTypes<TypeList, StorageList>(CheckFunctor{}, called);
  VISKORES_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");

  std::cout << "  Check CastAndCall again with UncertainArrayHandle" << std::endl;
  called = false;
  viskores::cont::CastAndCall(array.ResetTypes<TypeList, StorageList>(), CheckFunctor{}, called);
  VISKORES_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");
}

template <typename T>
viskores::cont::ArrayHandle<T> CreateArray(T)
{
  viskores::cont::ArrayHandle<T> array;
  array.Allocate(ARRAY_SIZE);
  SetPortal(array.WritePortal());
  return array;
}

viskores::cont::ArrayHandle<UnusualType> CreateArray(UnusualType)
{
  viskores::cont::ArrayHandle<UnusualType> array;
  array.Allocate(ARRAY_SIZE);
  auto portal = array.WritePortal();
  for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    portal.Set(index, TestValue(index, UnusualType::T{}));
  }
  return array;
}

template <typename T>
viskores::cont::UnknownArrayHandle CreateArrayUnknown(T t)
{
  return viskores::cont::UnknownArrayHandle(CreateArray(t));
}

template <typename ArrayHandleType>
void CheckAsArrayHandle(const ArrayHandleType& array)
{
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);
  using T = typename ArrayHandleType::ValueType;

  viskores::cont::UnknownArrayHandle arrayUnknown = array;
  VISKORES_TEST_ASSERT(!arrayUnknown.IsType<viskores::cont::ArrayHandle<UnusualType>>(),
                       "Dynamic array reporting is wrong type.");

  {
    std::cout << "    Normal get ArrayHandle" << std::endl;
    ArrayHandleType retreivedArray1;
    arrayUnknown.AsArrayHandle(retreivedArray1);
    VISKORES_TEST_ASSERT(arrayUnknown.CanConvert<ArrayHandleType>(),
                         "Did not query handle correctly.");
    VISKORES_TEST_ASSERT(array == retreivedArray1, "Did not get back same array.");

    ArrayHandleType retreivedArray2 = arrayUnknown.AsArrayHandle<ArrayHandleType>();
    VISKORES_TEST_ASSERT(array == retreivedArray2, "Did not get back same array.");
  }

  {
    std::cout << "    Put in cast array, get actual array" << std::endl;
    auto castArray = viskores::cont::make_ArrayHandleCast<viskores::Float64>(array);
    viskores::cont::UnknownArrayHandle arrayUnknown2(castArray);
    VISKORES_TEST_ASSERT(arrayUnknown2.IsType<ArrayHandleType>());
    ArrayHandleType retrievedArray = arrayUnknown2.AsArrayHandle<ArrayHandleType>();
    VISKORES_TEST_ASSERT(array == retrievedArray);
  }

  {
    std::cout << "    Get array as cast" << std::endl;
    viskores::cont::ArrayHandleCast<viskores::Float64, ArrayHandleType> castArray;
    arrayUnknown.AsArrayHandle(castArray);
    VISKORES_TEST_ASSERT(test_equal_portals(array.ReadPortal(), castArray.ReadPortal()));
  }

  {
    std::cout << "    Put in multiplexer, get actual array" << std::endl;
    viskores::cont::UnknownArrayHandle arrayUnknown2 = viskores::cont::ArrayHandleMultiplexer<
      ArrayHandleType,
      viskores::cont::ArrayHandleConstant<typename ArrayHandleType::ValueType>>(array);
    VISKORES_TEST_ASSERT(arrayUnknown2.IsType<ArrayHandleType>(),
                         "Putting in multiplexer did not pull out array.");
  }

  {
    std::cout << "    Make sure multiplex array prefers direct array (1st arg)" << std::endl;
    using MultiplexerType =
      viskores::cont::ArrayHandleMultiplexer<ArrayHandleType,
                                             viskores::cont::ArrayHandleCast<T, ArrayHandleType>>;
    MultiplexerType multiplexArray = arrayUnknown.AsArrayHandle<MultiplexerType>();

    VISKORES_TEST_ASSERT(multiplexArray.IsValid());
    VISKORES_TEST_ASSERT(multiplexArray.GetArrayHandleVariant().GetIndex() == 0);
    VISKORES_TEST_ASSERT(test_equal_portals(multiplexArray.ReadPortal(), array.ReadPortal()));
  }

  {
    std::cout << "    Make sure multiplex array prefers direct array (2nd arg)" << std::endl;
    using MultiplexerType = viskores::cont::ArrayHandleMultiplexer<
      viskores::cont::ArrayHandleCast<T, viskores::cont::ArrayHandle<T>>,
      ArrayHandleType>;
    MultiplexerType multiplexArray = arrayUnknown.AsArrayHandle<MultiplexerType>();

    VISKORES_TEST_ASSERT(multiplexArray.IsValid());
    VISKORES_TEST_ASSERT(multiplexArray.GetArrayHandleVariant().GetIndex() == 1);
    VISKORES_TEST_ASSERT(test_equal_portals(multiplexArray.ReadPortal(), array.ReadPortal()));
  }

  {
    std::cout << "    Make sure adding arrays follows nesting of special arrays" << std::endl;
    viskores::cont::ArrayHandleMultiplexer<
      viskores::cont::ArrayHandle<viskores::Int64>,
      viskores::cont::ArrayHandleCast<viskores::Int64, ArrayHandleType>>
      multiplexer(viskores::cont::make_ArrayHandleCast<viskores::Int64>(array));
    auto crazyArray = viskores::cont::make_ArrayHandleCast<viskores::Float64>(multiplexer);
    viskores::cont::UnknownArrayHandle arrayUnknown2(crazyArray);
    VISKORES_TEST_ASSERT(arrayUnknown2.IsType<ArrayHandleType>());
    ArrayHandleType retrievedArray = arrayUnknown2.AsArrayHandle<ArrayHandleType>();
    VISKORES_TEST_ASSERT(array == retrievedArray);
  }

  {
    std::cout << "    Try adding arrays with variable amounts of components" << std::endl;
    // There might be some limited functionality, but you should still be able
    // to get arrays in and out.

    // Note, this is a bad way to implement this array. You should something like
    // ArrayHandleGroupVec instead.
    using VariableVecArrayType = viskores::cont::ArrayHandleGroupVecVariable<
      ArrayHandleType,
      viskores::cont::ArrayHandleCounting<viskores::Id>>;
    VariableVecArrayType inArray = viskores::cont::make_ArrayHandleGroupVecVariable(
      array, viskores::cont::make_ArrayHandleCounting<viskores::Id>(0, 2, ARRAY_SIZE / 2 + 1));
    VISKORES_TEST_ASSERT(inArray.GetNumberOfValues() == ARRAY_SIZE / 2);
    viskores::cont::UnknownArrayHandle arrayUnknown2 = inArray;
    VISKORES_TEST_ASSERT(arrayUnknown2.IsType<VariableVecArrayType>());
    VariableVecArrayType retrievedArray = arrayUnknown2.AsArrayHandle<VariableVecArrayType>();
    VISKORES_TEST_ASSERT(retrievedArray == inArray);
  }
}

// A viskores::Vec if NumComps > 1, otherwise a scalar
template <typename T, viskores::IdComponent NumComps>
using VecOrScalar = typename std::conditional<(NumComps > 1), viskores::Vec<T, NumComps>, T>::type;

template <typename T>
void TryNewInstance(viskores::cont::UnknownArrayHandle originalArray)
{
  // This check should already have been performed by caller, but just in case.
  CheckUnknownArray<viskores::List<T>, VISKORES_DEFAULT_STORAGE_LIST>(
    originalArray, viskores::VecTraits<T>::NUM_COMPONENTS);

  std::cout << "Create new instance of array." << std::endl;
  viskores::cont::UnknownArrayHandle newArray = originalArray.NewInstance();

  std::cout << "Get a static instance of the new array (which checks the type)." << std::endl;
  viskores::cont::ArrayHandle<T> staticArray;
  newArray.AsArrayHandle(staticArray);

  std::cout << "Fill the new array with invalid values and make sure the original" << std::endl
            << "is uneffected." << std::endl;
  staticArray.Allocate(ARRAY_SIZE);
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    staticArray.WritePortal().Set(index, TestValue(index + 100, T()));
  }
  CheckUnknownArray<viskores::List<T>, VISKORES_DEFAULT_STORAGE_LIST>(
    originalArray, viskores::VecTraits<T>::NUM_COMPONENTS);

  std::cout << "Set the new static array to expected values and make sure the new" << std::endl
            << "dynamic array points to the same new values." << std::endl;
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    staticArray.WritePortal().Set(index, TestValue(index, T()));
  }
  CheckUnknownArray<viskores::List<T>, VISKORES_DEFAULT_STORAGE_LIST>(
    newArray, viskores::VecTraits<T>::NUM_COMPONENTS);

  std::cout << "Get a new instance as a float array and make sure the type is as expected."
            << std::endl;
  viskores::cont::UnknownArrayHandle floatArray = originalArray.NewInstanceFloatBasic();
  viskores::cont::ArrayHandle<
    typename viskores::VecTraits<T>::template ReplaceBaseComponentType<viskores::FloatDefault>>
    staticFloatArray;
  floatArray.AsArrayHandle(staticFloatArray);
}

template <typename ActualT>
struct CheckActualTypeFunctor
{
  template <typename T, typename S>
  void operator()(const viskores::cont::ArrayHandle<T, S>& array, bool& called) const
  {
    called = true;
    VISKORES_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Unexpected array size.");
    auto portal = array.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      T retrieved = portal.Get(i);
      ActualT expected = TestValue(i, ActualT{});
      VISKORES_TEST_ASSERT(test_equal(retrieved, expected));
    }
  }
};

template <typename T>
void TryCastAndCallFallback()
{
  viskores::cont::UnknownArrayHandle array = CreateArrayUnknown(T{});

  using FallbackTypes = viskores::List<viskores::FloatDefault,
                                       viskores::Vec2f,
                                       viskores::Vec3f,
                                       viskores::Vec4f,
                                       viskores::Vec<viskores::Vec2f, 3>,
                                       viskores::Vec<viskores::Vec<viskores::Vec4f, 3>, 2>>;
  bool called = false;
  array.CastAndCallForTypesWithFloatFallback<FallbackTypes, viskores::cont::StorageListBasic>(
    CheckActualTypeFunctor<T>{}, called);
  VISKORES_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");
}

void TryCastAndCallFallback()
{
  std::cout << "  Scalar array." << std::endl;
  TryCastAndCallFallback<viskores::Float64>();

  std::cout << "  Equivalent scalar." << std::endl;
  TryCastAndCallFallback<VISKORES_UNUSED_INT_TYPE>();

  std::cout << "  Basic Vec." << std::endl;
  TryCastAndCallFallback<viskores::Id3>();

  std::cout << "  Vec of Vecs." << std::endl;
  TryCastAndCallFallback<viskores::Vec<viskores::Vec2f_32, 3>>();

  std::cout << "  Vec of Vecs of Vecs." << std::endl;
  TryCastAndCallFallback<viskores::Vec<viskores::Vec<viskores::Id4, 3>, 2>>();
}

template <typename T>
void TryAsMultiplexer(viskores::cont::UnknownArrayHandle sourceArray)
{
  auto originalArray = sourceArray.AsArrayHandle<viskores::cont::ArrayHandle<T>>();

  {
    std::cout << "Get multiplex array through direct type." << std::endl;
    using MultiplexerType =
      viskores::cont::ArrayHandleMultiplexer<viskores::cont::ArrayHandle<T>,
                                             viskores::cont::ArrayHandleConstant<T>>;
    VISKORES_TEST_ASSERT(sourceArray.CanConvert<MultiplexerType>());
    MultiplexerType multiplexArray = sourceArray.AsArrayHandle<MultiplexerType>();

    VISKORES_TEST_ASSERT(multiplexArray.IsValid());
    VISKORES_TEST_ASSERT(
      test_equal_portals(multiplexArray.ReadPortal(), originalArray.ReadPortal()));
  }

  {
    std::cout << "Get multiplex array through cast type." << std::endl;
    using CastT =
      typename viskores::VecTraits<T>::template ReplaceBaseComponentType<viskores::Float64>;
    using MultiplexerType = viskores::cont::ArrayHandleMultiplexer<
      viskores::cont::ArrayHandle<CastT>,
      viskores::cont::ArrayHandleCast<CastT, viskores::cont::ArrayHandle<T>>>;
    VISKORES_TEST_ASSERT(sourceArray.CanConvert<MultiplexerType>());
    MultiplexerType multiplexArray = sourceArray.AsArrayHandle<MultiplexerType>();

    VISKORES_TEST_ASSERT(multiplexArray.IsValid());
    VISKORES_TEST_ASSERT(
      test_equal_portals(multiplexArray.ReadPortal(), originalArray.ReadPortal()));
  }

#if 0
  // Maybe we should support this, but right now we don't
  {
    std::cout << "Make sure multiplex array prefers direct array (1st arg)" << std::endl;
    using MultiplexerType = viskores::cont::ArrayHandleMultiplexer<
      viskores::cont::ArrayHandle<T>,
      viskores::cont::ArrayHandleCast<T, viskores::cont::ArrayHandle<T>>>;
    MultiplexerType multiplexArray = sourceArray.AsArrayHandle<MultiplexerType>();

    VISKORES_TEST_ASSERT(multiplexArray.IsValid());
    VISKORES_TEST_ASSERT(multiplexArray.GetStorage().GetArrayHandleVariant().GetIndex() == 0);
    VISKORES_TEST_ASSERT(test_equal_portals(multiplexArray.ReadPortal(), originalArray.ReadPortal()));
  }

  {
    std::cout << "Make sure multiplex array prefers direct array (2nd arg)" << std::endl;
    using MultiplexerType =
      viskores::cont::ArrayHandleMultiplexer<viskores::cont::ArrayHandleCast<T, viskores::cont::ArrayHandle<T>>,
                                         viskores::cont::ArrayHandle<T>>;
    MultiplexerType multiplexArray = sourceArray.AsArrayHandle<MultiplexerType>();

    VISKORES_TEST_ASSERT(multiplexArray.IsValid());
    VISKORES_TEST_ASSERT(multiplexArray.GetStorage().GetArrayHandleVariant().GetIndex() == 1);
    VISKORES_TEST_ASSERT(test_equal_portals(multiplexArray.ReadPortal(), originalArray.ReadPortal()));
  }
#endif
}

struct SimpleRecombineCopy
{
  template <typename T>
  void operator()(const viskores::cont::ArrayHandleRecombineVec<T>& inputArray,
                  const viskores::cont::UnknownArrayHandle& output) const
  {
    viskores::cont::ArrayHandleRecombineVec<T> outputArray =
      output.ExtractArrayFromComponents<T>(viskores::CopyFlag::Off);
    viskores::Id size = inputArray.GetNumberOfValues();
    outputArray.Allocate(size);
    auto inputPortal = inputArray.ReadPortal();
    auto outputPortal = outputArray.WritePortal();

    for (viskores::Id index = 0; index < size; ++index)
    {
      outputPortal.Set(index, inputPortal.Get(index));
    }
  }
};

template <typename T>
void TryExtractArray(const viskores::cont::UnknownArrayHandle& originalArray)
{
  // This check should already have been performed by caller, but just in case.
  CheckUnknownArray<viskores::List<T>, VISKORES_DEFAULT_STORAGE_LIST>(
    originalArray, viskores::VecTraits<T>::NUM_COMPONENTS);

  std::cout << "Create new instance of array." << std::endl;
  viskores::cont::UnknownArrayHandle newArray = originalArray.NewInstanceBasic();

  std::cout << "Do CastAndCallWithExtractedArray." << std::endl;
  originalArray.CastAndCallWithExtractedArray(SimpleRecombineCopy{}, newArray);

  CheckUnknownArray<viskores::List<T>, VISKORES_DEFAULT_STORAGE_LIST>(
    newArray, viskores::VecTraits<T>::NUM_COMPONENTS);
}

template <typename T>
void TryDefaultType()
{
  viskores::cont::UnknownArrayHandle array = CreateArrayUnknown(T{});

  CheckUnknownArrayDefaults(array, viskores::VecTraits<T>::NUM_COMPONENTS);

  TryNewInstance<T>(array);

  TryAsMultiplexer<T>(array);

  TryExtractArray<T>(array);
}

struct TryBasicViskoresType
{
  template <typename T>
  void operator()(T) const
  {
    viskores::cont::UnknownArrayHandle array = CreateArrayUnknown(T());

    VISKORES_TEST_ASSERT(array.GetValueTypeName() == viskores::cont::TypeToString<T>());
    VISKORES_TEST_ASSERT(array.GetStorageTypeName() ==
                         viskores::cont::TypeToString<viskores::cont::StorageTagBasic>());

    CheckUnknownArray<viskores::TypeListAll, VISKORES_DEFAULT_STORAGE_LIST>(
      array, viskores::VecTraits<T>::NUM_COMPONENTS);

    TryNewInstance<T>(array);
  }
};

void TryUnusualType()
{
  // A string is an unlikely type to be declared elsewhere in Viskores.
  viskores::cont::UnknownArrayHandle array = CreateArrayUnknown(UnusualType{});

  try
  {
    CheckUnknownArray<VISKORES_DEFAULT_TYPE_LIST, VISKORES_DEFAULT_STORAGE_LIST>(array, 1);
    VISKORES_TEST_FAIL("CastAndCall failed to error for unrecognized type.");
  }
  catch (viskores::cont::ErrorBadType&)
  {
    std::cout << "  Caught exception for unrecognized type." << std::endl;
  }
  CheckUnknownArray<viskores::List<UnusualType>, VISKORES_DEFAULT_STORAGE_LIST>(array, 1);
  std::cout << "  Found type when type list was reset." << std::endl;
}

template <typename ArrayHandleType>
void TryAsArrayHandle(const ArrayHandleType& array)
{
  CheckAsArrayHandle(array);
}

void TryAsArrayHandle()
{
  std::cout << "  Normal array handle." << std::endl;
  viskores::Id buffer[ARRAY_SIZE];
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    buffer[index] = TestValue(index, viskores::Id());
  }

  viskores::cont::ArrayHandle<viskores::Id> array =
    viskores::cont::make_ArrayHandle(buffer, ARRAY_SIZE, viskores::CopyFlag::On);
  TryAsArrayHandle(array);

  std::cout << "  Constant array handle." << std::endl;
  TryAsArrayHandle(viskores::cont::make_ArrayHandleConstant(5, ARRAY_SIZE));
}

struct CheckExtractedArray
{
  template <typename ExtractedArray, typename OriginalArray>
  void operator()(const ExtractedArray& extractedArray, const OriginalArray& originalArray) const
  {
    using ValueType = typename OriginalArray::ValueType;
    using FlatVec = viskores::VecFlat<ValueType>;

    VISKORES_TEST_ASSERT(extractedArray.GetNumberOfComponents() == FlatVec::NUM_COMPONENTS);
    auto originalPortal = originalArray.ReadPortal();
    auto extractedPortal = extractedArray.ReadPortal();
    for (viskores::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
    {
      FlatVec originalData = originalPortal.Get(valueIndex);
      auto extractedData = extractedPortal.Get(valueIndex);
      VISKORES_TEST_ASSERT(test_equal(originalData, extractedData));
    }

    // Make sure an extracted array stuffed back into an UnknownArrayHandle works.
    // This can happen when working with an extracted array that is passed to functions
    // that are implemented with UnknownArrayHandle.
    viskores::cont::UnknownArrayHandle unknownArray{ extractedArray };

    using ComponentType =
      typename viskores::VecTraits<typename ExtractedArray::ValueType>::BaseComponentType;
    viskores::cont::UnknownArrayHandle newBasic = unknownArray.NewInstanceBasic();
    newBasic.AsArrayHandle<viskores::cont::ArrayHandleRuntimeVec<ComponentType>>();
    viskores::cont::UnknownArrayHandle newFloat = unknownArray.NewInstanceFloatBasic();
    newFloat.AsArrayHandle<viskores::cont::ArrayHandleRuntimeVec<viskores::FloatDefault>>();
  }
};

template <typename ArrayHandleType>
void TryExtractComponent()
{
  using ValueType = typename ArrayHandleType::ValueType;
  using FlatVec = viskores::VecFlat<ValueType>;
  using ComponentType = typename FlatVec::ComponentType;

  ArrayHandleType originalArray;
  originalArray.Allocate(ARRAY_SIZE);
  SetPortal(originalArray.WritePortal());

  viskores::cont::UnknownArrayHandle unknownArray(originalArray);

  VISKORES_TEST_ASSERT(unknownArray.GetNumberOfComponentsFlat() == FlatVec::NUM_COMPONENTS);

  CheckExtractedArray{}(unknownArray.ExtractArrayFromComponents<ComponentType>(), originalArray);

  unknownArray.CastAndCallWithExtractedArray(CheckExtractedArray{}, originalArray);
}

void TryExtractComponent()
{
  std::cout << "  Scalar array." << std::endl;
  TryExtractComponent<viskores::cont::ArrayHandle<viskores::FloatDefault>>();

  std::cout << "  Equivalent scalar." << std::endl;
  TryExtractComponent<viskores::cont::ArrayHandle<VISKORES_UNUSED_INT_TYPE>>();

  std::cout << "  Basic Vec." << std::endl;
  TryExtractComponent<viskores::cont::ArrayHandle<viskores::Id3>>();

  std::cout << "  Vec of Vecs." << std::endl;
  TryExtractComponent<viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec2f, 3>>>();

  std::cout << "  Vec of Vecs of Vecs." << std::endl;
  TryExtractComponent<
    viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec<viskores::Id4, 3>, 2>>>();
}

void TrySetCastArray()
{
  viskores::cont::ArrayHandle<viskores::Id> knownArray = CreateArray(viskores::Id{});
  viskores::cont::UnknownArrayHandle unknownArray(
    viskores::cont::make_ArrayHandleCast<viskores::Float32>(knownArray));

  // The unknownArray should actually hold the original knownArray type even though we gave it
  // a cast array.
  CheckUnknownArray<viskores::List<viskores::Id>, viskores::List<VISKORES_DEFAULT_STORAGE_TAG>>(
    unknownArray, 1);
}

void TrySetMultiplexerArray()
{
  viskores::cont::ArrayHandle<viskores::Id> knownArray = CreateArray(viskores::Id{});
  viskores::cont::ArrayHandleMultiplexer<viskores::cont::ArrayHandle<viskores::Id>,
                                         viskores::cont::ArrayHandleConstant<viskores::Id>>
    multiplexerArray(knownArray);
  viskores::cont::UnknownArrayHandle unknownArray(multiplexerArray);

  // The unknownArray should actually hold the original knownArray type even though we gave it
  // a multiplexer array.
  CheckUnknownArray<viskores::List<viskores::Id>, viskores::List<VISKORES_DEFAULT_STORAGE_TAG>>(
    unknownArray, 1);
}

template <typename T, typename BasicComponentType = typename viskores::VecFlat<T>::ComponentType>
void TryConvertRuntimeVec()
{
  using BasicArrayType = viskores::cont::ArrayHandle<T>;
  constexpr viskores::IdComponent numFlatComponents = viskores::VecFlat<T>::NUM_COMPONENTS;
  using RuntimeArrayType = viskores::cont::ArrayHandleRuntimeVec<BasicComponentType>;

  std::cout << "    Get basic array as ArrayHandleRuntimeVec" << std::endl;
  BasicArrayType inputArray;
  inputArray.Allocate(ARRAY_SIZE);
  SetPortal(inputArray.WritePortal());

  viskores::cont::UnknownArrayHandle unknownWithBasic{ inputArray };
  VISKORES_TEST_ASSERT(unknownWithBasic.GetNumberOfComponentsFlat() == numFlatComponents);

  VISKORES_TEST_ASSERT(unknownWithBasic.CanConvert<RuntimeArrayType>());
  RuntimeArrayType runtimeArray = unknownWithBasic.AsArrayHandle<RuntimeArrayType>();

  // Hack to convert the array handle to a flat array to make it easy to check the runtime array
  viskores::cont::ArrayHandle<viskores::VecFlat<T>> flatInput{ inputArray.GetBuffers() };
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(flatInput, runtimeArray));

  std::cout << "    Get ArrayHandleRuntimeVec as basic array" << std::endl;
  viskores::cont::UnknownArrayHandle unknownWithRuntimeVec{ runtimeArray };
  VISKORES_TEST_ASSERT(unknownWithRuntimeVec.GetNumberOfComponentsFlat() == numFlatComponents);

  VISKORES_TEST_ASSERT(unknownWithRuntimeVec.CanConvert<RuntimeArrayType>());
  VISKORES_TEST_ASSERT(unknownWithRuntimeVec.CanConvert<BasicArrayType>());
  BasicArrayType outputArray = unknownWithRuntimeVec.AsArrayHandle<BasicArrayType>();
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(inputArray, outputArray));

  std::cout << "    Copy ArrayHandleRuntimeVec to a new instance" << std::endl;
  viskores::cont::UnknownArrayHandle unknownCopy = unknownWithRuntimeVec.NewInstance();
  VISKORES_TEST_ASSERT(unknownWithRuntimeVec.GetNumberOfComponentsFlat() ==
                       unknownCopy.GetNumberOfComponentsFlat());
  viskores::cont::ArrayCopy(unknownWithRuntimeVec, unknownCopy);
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(inputArray, unknownCopy));

  std::cout << "    Copy ArrayHandleRuntimeVec as basic array" << std::endl;
  unknownCopy = unknownWithRuntimeVec.NewInstanceBasic();
  VISKORES_TEST_ASSERT(unknownWithRuntimeVec.GetNumberOfComponentsFlat() ==
                       unknownCopy.GetNumberOfComponentsFlat());
  viskores::cont::ArrayCopy(unknownWithRuntimeVec, unknownCopy);
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(inputArray, unknownCopy));

  std::cout << "    Copy ArrayHandleRuntimeVec to float array" << std::endl;
  unknownCopy = unknownWithRuntimeVec.NewInstanceFloatBasic();
  VISKORES_TEST_ASSERT(unknownWithRuntimeVec.GetNumberOfComponentsFlat() ==
                       unknownCopy.GetNumberOfComponentsFlat());
  viskores::cont::ArrayCopy(unknownWithRuntimeVec, unknownCopy);
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(inputArray, unknownCopy));
}

void TryConvertRuntimeVec()
{
  std::cout << "  Scalar array." << std::endl;
  TryConvertRuntimeVec<viskores::FloatDefault>();

  std::cout << "  Equivalent scalar." << std::endl;
  TryConvertRuntimeVec<VISKORES_UNUSED_INT_TYPE>();

  std::cout << "  Basic Vec." << std::endl;
  TryConvertRuntimeVec<viskores::Id3>();

  std::cout << "  Vec of Vecs." << std::endl;
  TryConvertRuntimeVec<viskores::Vec<viskores::Vec2f, 3>>();

  std::cout << "  Vec of Vecs of Vecs." << std::endl;
  TryConvertRuntimeVec<viskores::Vec<viskores::Vec<viskores::Id4, 3>, 2>>();

  std::cout << "  Compatible but different C types." << std::endl;
  if (sizeof(int) == sizeof(long))
  {
    TryConvertRuntimeVec<viskores::Vec<int, 4>, long>();
  }
  else // assuming sizeof(long long) == sizeof(long)
  {
    TryConvertRuntimeVec<viskores::Vec<long long, 4>, long>();
  }
}

struct DefaultTypeFunctor
{
  template <typename T>
  void operator()(T) const
  {
    TryDefaultType<T>();
  }
};

void TestUnknownArrayHandle()
{
  std::cout << "Try common types with default type lists." << std::endl;
  viskores::testing::Testing::TryTypes(DefaultTypeFunctor{}, VISKORES_DEFAULT_TYPE_LIST{});

  std::cout << "Try exemplar Viskores types." << std::endl;
  viskores::testing::Testing::TryTypes(TryBasicViskoresType{});

  std::cout << "Try unusual type." << std::endl;
  TryUnusualType();

  std::cout << "Try AsArrayHandle" << std::endl;
  TryAsArrayHandle();

  std::cout << "Try CastAndCall with fallback" << std::endl;
  TryCastAndCallFallback();

  std::cout << "Try ExtractComponent" << std::endl;
  TryExtractComponent();

  std::cout << "Try setting ArrayHandleCast" << std::endl;
  TrySetCastArray();

  std::cout << "Try setting ArrayHandleMultiplexer" << std::endl;
  TrySetMultiplexerArray();

  std::cout << "Try converting between ArrayHandleRuntimeVec and basic array" << std::endl;
  TryConvertRuntimeVec();
}

} // anonymous namespace

int UnitTestUnknownArrayHandle(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestUnknownArrayHandle, argc, argv);
}
