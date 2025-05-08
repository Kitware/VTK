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
#ifndef viskores_testing_VecTraitsTest_h
#define viskores_testing_VecTraitsTest_h

//GCC 4+ when running the test code have false positive warnings
//about uninitialized viskores::VecC<> when filled by VecTraits<T>::CopyInto.
//The testing code already verifies that CopyInto works by verifying the
//results, so we are going to suppress `-Wmaybe-uninitialized` for this
//file
//This block has to go before we include any viskores file that brings in
//<viskores/Types.h> otherwise the warning suppression will not work
#include <viskores/internal/Configure.h>
#if (defined(VISKORES_GCC) && __GNUC__ >= 4)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif // gcc  4+

#include <viskores/VecTraits.h>

#include <viskores/StaticAssert.h>
#include <viskores/TypeTraits.h>

#include <viskores/testing/Testing.h>

namespace viskores
{
namespace testing
{

namespace detail
{

inline void CompareDimensionalityTags(viskores::TypeTraitsScalarTag,
                                      viskores::VecTraitsTagSingleComponent)
{
  // If we are here, everything is fine.
}
inline void CompareDimensionalityTags(viskores::TypeTraitsVectorTag,
                                      viskores::VecTraitsTagMultipleComponents)
{
  // If we are here, everything is fine.
}
inline void CompareDimensionalityTags(viskores::TypeTraitsUnknownTag,
                                      viskores::VecTraitsTagSingleComponent)
{
  // If we are here, type traits are probably not defined (and default to unknown). In this case,
  // we expect VecTraits to have the default implementation, in which case it is treated as a
  // single component.
}

template <viskores::IdComponent NUM_COMPONENTS, typename T>
inline void CheckIsStatic(const T&, viskores::VecTraitsTagSizeStatic)
{
  VISKORES_TEST_ASSERT(viskores::VecTraits<T>::NUM_COMPONENTS == NUM_COMPONENTS,
                       "Traits returns unexpected number of components");
}

template <viskores::IdComponent NUM_COMPONENTS, typename T>
inline void CheckIsStatic(const T&, viskores::VecTraitsTagSizeVariable)
{
  // If we are here, everything is fine.
}

template <typename VecType>
struct VecIsWritable
{
  using type = std::true_type;
};

template <typename ComponentType>
struct VecIsWritable<viskores::VecCConst<ComponentType>>
{
  using type = std::false_type;
};

template <typename ComponentType>
struct VecIsWritable<viskores::VecCConst<ComponentType>*>
{
  using type = std::false_type;
};

// Part of TestVecTypeImpl that writes to the Vec type
template <viskores::IdComponent NUM_COMPONENTS, typename T, typename VecCopyType>
static void TestVecTypeWritableImpl(const T& inVector,
                                    const VecCopyType& vectorCopy,
                                    T& outVector,
                                    std::true_type)
{
  using Traits = viskores::VecTraits<T>;
  using ComponentType = typename Traits::ComponentType;

  {
    const ComponentType multiplier = 4;
    for (viskores::IdComponent i = 0; i < NUM_COMPONENTS; i++)
    {
      Traits::SetComponent(
        outVector, i, ComponentType(multiplier * Traits::GetComponent(inVector, i)));
    }
    viskores::Vec<ComponentType, NUM_COMPONENTS> resultCopy;
    Traits::CopyInto(outVector, resultCopy);
    VISKORES_TEST_ASSERT(test_equal(resultCopy, multiplier * vectorCopy),
                         "Got bad result for scalar multiple");
  }

  {
    const ComponentType multiplier = 7;
    for (viskores::IdComponent i = 0; i < NUM_COMPONENTS; i++)
    {
      Traits::GetComponent(outVector, i) =
        ComponentType(multiplier * Traits::GetComponent(inVector, i));
    }
    viskores::Vec<ComponentType, NUM_COMPONENTS> resultCopy;
    Traits::CopyInto(outVector, resultCopy);
    VISKORES_TEST_ASSERT(test_equal(resultCopy, multiplier * vectorCopy),
                         "Got bad result for scalar multiple");
  }
}

template <viskores::IdComponent NUM_COMPONENTS, typename T, typename VecCopyType>
static void TestVecTypeWritableImpl(const T& viskoresNotUsed(inVector),
                                    const VecCopyType& viskoresNotUsed(vectorCopy),
                                    T& viskoresNotUsed(outVector),
                                    std::false_type)
{
  // Skip writable functionality.
}

/// Compares some manual arithmetic through type traits to arithmetic with
/// the Tuple class.
template <viskores::IdComponent NUM_COMPONENTS, typename T>
static void TestVecTypeImpl(const typename std::remove_const<T>::type& inVector,
                            typename std::remove_const<T>::type& outVector)
{
  using Traits = viskores::VecTraits<T>;
  using ComponentType = typename Traits::ComponentType;
  using NonConstT = typename std::remove_const<T>::type;

  CheckIsStatic<NUM_COMPONENTS>(inVector, typename Traits::IsSizeStatic());

  VISKORES_TEST_ASSERT(Traits::GetNumberOfComponents(inVector) == NUM_COMPONENTS,
                       "Traits returned wrong number of components.");

  viskores::Vec<ComponentType, NUM_COMPONENTS> vectorCopy;
  Traits::CopyInto(inVector, vectorCopy);
  VISKORES_TEST_ASSERT(test_equal(vectorCopy, inVector), "CopyInto does not work.");

  {
    auto expected = viskores::Dot(vectorCopy, vectorCopy);
    decltype(expected) result = 0;
    for (viskores::IdComponent i = 0; i < NUM_COMPONENTS; i++)
    {
      ComponentType component = Traits::GetComponent(inVector, i);
      result = result + (component * component);
    }
    VISKORES_TEST_ASSERT(test_equal(result, expected), "Got bad result for dot product");
  }

  // This will fail to compile if the tags are wrong.
  detail::CompareDimensionalityTags(
    typename viskores::TypeTraits<std::remove_pointer_t<T>>::DimensionalityTag(),
    typename viskores::VecTraits<T>::HasMultipleComponents());

  TestVecTypeWritableImpl<NUM_COMPONENTS, NonConstT>(
    inVector, vectorCopy, outVector, typename VecIsWritable<NonConstT>::type());

  // Compiler checks for base component types
  using BaseComponentType = typename viskores::VecTraits<T>::BaseComponentType;
  VISKORES_STATIC_ASSERT(
    (std::is_same<typename viskores::TypeTraits<BaseComponentType>::DimensionalityTag,
                  viskores::TypeTraitsScalarTag>::value) ||
    (std::is_same<typename viskores::TypeTraits<BaseComponentType>::DimensionalityTag,
                  viskores::TypeTraitsUnknownTag>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<typename viskores::VecTraits<ComponentType>::BaseComponentType,
                  BaseComponentType>::value));

  // Compiler checks for replacing component types
  using ReplaceWithVecComponent =
    typename viskores::VecTraits<T>::template ReplaceComponentType<viskores::Vec<char, 2>>;
  VISKORES_STATIC_ASSERT(
    (std::is_same<typename viskores::TypeTraits<std::remove_pointer_t<T>>::DimensionalityTag,
                  viskores::TypeTraitsVectorTag>::value &&
     std::is_same<typename viskores::VecTraits<ReplaceWithVecComponent>::ComponentType,
                  viskores::Vec<char, 2>>::value) ||
    (!std::is_same<typename viskores::TypeTraits<std::remove_pointer_t<T>>::DimensionalityTag,
                   viskores::TypeTraitsVectorTag>::value &&
     std::is_same<typename viskores::VecTraits<ReplaceWithVecComponent>::ComponentType,
                  char>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<typename viskores::VecTraits<ReplaceWithVecComponent>::BaseComponentType,
                  char>::value));
  using ReplaceBaseComponent =
    typename viskores::VecTraits<ReplaceWithVecComponent>::template ReplaceBaseComponentType<short>;
  VISKORES_STATIC_ASSERT(
    (std::is_same<typename viskores::TypeTraits<std::remove_pointer_t<T>>::DimensionalityTag,
                  viskores::TypeTraitsVectorTag>::value &&
     std::is_same<typename viskores::VecTraits<ReplaceBaseComponent>::ComponentType,
                  viskores::Vec<short, 2>>::value) ||
    (!std::is_same<typename viskores::TypeTraits<std::remove_pointer_t<T>>::DimensionalityTag,
                   viskores::TypeTraitsVectorTag>::value &&
     std::is_same<typename viskores::VecTraits<ReplaceBaseComponent>::ComponentType,
                  short>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<typename viskores::VecTraits<ReplaceBaseComponent>::BaseComponentType,
                  short>::value));
}

inline void CheckVecComponentsTag(viskores::VecTraitsTagMultipleComponents)
{
  // If we are running here, everything is fine.
}

} // namespace detail

/// Checks to make sure that the HasMultipleComponents tag is actually for
/// multiple components. Should only be called for vector classes that actually
/// have multiple components.
///
template <class T>
inline void TestVecComponentsTag()
{
  // This will fail to compile if the tag is wrong
  // (i.e. not viskores::VecTraitsTagMultipleComponents)
  detail::CheckVecComponentsTag(typename viskores::VecTraits<T>::HasMultipleComponents());
}

namespace detail
{

inline void CheckScalarComponentsTag(viskores::VecTraitsTagSingleComponent)
{
  // If we are running here, everything is fine.
}

} // namespace detail

/// Compares some manual arithmetic through type traits to arithmetic with
/// the Tuple class.
template <viskores::IdComponent NUM_COMPONENTS, typename T>
static void TestVecType(const T& inVector, T& outVector)
{
  detail::TestVecTypeImpl<NUM_COMPONENTS, T>(inVector, outVector);
  detail::TestVecTypeImpl<NUM_COMPONENTS, const T>(inVector, outVector);
  // The local pointer variables are for some weirdness about `TestVecTypeImpl` taking references
  // of its argument type.
  T* inPointer = const_cast<T*>(&inVector);
  T* outPointer = &outVector;
  detail::TestVecTypeImpl<NUM_COMPONENTS, T*>(inPointer, outPointer);
  VISKORES_STATIC_ASSERT_MSG(
    (std::is_base_of<viskores::VecTraits<T*>, viskores::VecTraits<const T*>>::value),
    "Constant pointer should have same implementation as pointer.");
}

/// Checks to make sure that the HasMultipleComponents tag is actually for a
/// single component. Should only be called for "vector" classes that actually
/// have only a single component (that is, are really scalars).
///
template <class T>
inline void TestScalarComponentsTag()
{
  // This will fail to compile if the tag is wrong
  // (i.e. not viskores::VecTraitsTagSingleComponent)
  detail::CheckScalarComponentsTag(typename viskores::VecTraits<T>::HasMultipleComponents());
}
}
} // namespace viskores::testing

#if (defined(VISKORES_GCC) && __GNUC__ > 4 && __GNUC__ < 7)
#pragma GCC diagnostic pop
#endif // gcc  5 or 6

#endif //viskores_testing_VecTraitsTest_h
