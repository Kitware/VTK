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
#ifndef viskores_VecTraits_h
#define viskores_VecTraits_h

#include <viskores/Deprecated.h>
#include <viskores/StaticAssert.h>
#include <viskores/Types.h>

namespace viskores
{

/// A tag for vectors that are "true" vectors (i.e. have more than one
/// component).
///
struct VecTraitsTagMultipleComponents
{
};

/// A tag for vectors that are really just scalars (i.e. have only one
/// component)
///
struct VecTraitsTagSingleComponent
{
};

/// A tag for vectors where the number of components are known at compile time.
///
struct VecTraitsTagSizeStatic
{
};

/// A tag for vectors where the number of components are not determined until
/// run time.
///
struct VecTraitsTagSizeVariable
{
};

/// \brief Traits that can be queried to treat any type as a `Vec`.
///
/// The VecTraits class gives several static members that define how
/// to use a given type as a vector. This is useful for templated
/// functions and methods that have a parameter that could be either
/// a standard scalar type or a `Vec` or some other `Vec`-like
/// object. When using this class, scalar objects are treated like
/// a `Vec` of size 1.
///
/// The default implementation of this template treats the type as
/// a scalar. Types that actually behave like vectors should
/// specialize this template to provide the proper information.
///
template <class T>
struct VISKORES_NEVER_EXPORT VecTraits
{
  // The base VecTraits should not be used with qualifiers.
  VISKORES_STATIC_ASSERT_MSG((std::is_same<std::remove_pointer_t<std::decay_t<T>>, T>::value),
                             "The base VecTraits should not be used with qualifiers.");

  /// \brief Type of the components in the vector.
  ///
  /// If the type is really a scalar, then the component type is the same as the scalar type.
  ///
  using ComponentType = T;

  /// \brief Base component type in the vector.
  ///
  /// Similar to ComponentType except that for nested vectors (e.g. Vec<Vec<T, M>, N>), it
  /// returns the base scalar type at the end of the composition (T in this example).
  ///
  using BaseComponentType = T;

  /// \brief Number of components in the vector.
  ///
  /// This is only defined for vectors of a static size. That is, `NUM_COMPONENTS`
  /// is not available when `IsSizeStatic` is set to `viskores::VecTraitsTagSizeVariable`.
  ///
  static constexpr viskores::IdComponent NUM_COMPONENTS = 1;

  /// @brief Returns the number of components in the given vector.
  ///
  /// The result of `GetNumberOfComponents()` is the same value of `NUM_COMPONENTS`
  /// for vector types that have a static size (that is, `IsSizeStatic` is
  /// `viskores::VecTraitsTagSizeStatic`). But unlike `NUM_COMPONENTS`, `GetNumberOfComponents()`
  /// works for vectors of any type.
  ///
  static constexpr viskores::IdComponent GetNumberOfComponents(const T&) { return NUM_COMPONENTS; }

  /// \brief A tag specifying whether this vector has multiple components (i.e. is a "real" vector).
  ///
  /// This type is set to either `viskores::VecTraitsTagSingleComponent` if the vector length
  /// is size 1 or `viskores::VecTraitsTagMultipleComponents` otherwise.
  /// This tag can be useful for creating specialized functions when a vector is really
  /// just a scalar. If the vector type is of variable size (that is, `IsSizeStatic` is
  /// `viskores::VecTraitsTagSizeVariable`), then `HasMultipleComponents` might be
  /// `viskores::VecTraitsTagMultipleComponents` even when at run time there is only one component.
  ///
  using HasMultipleComponents = viskores::VecTraitsTagSingleComponent;

  /// \brief A tag specifying whether the size of this vector is known at compile time.
  ///
  /// If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set. If
  /// set to \c VecTraitsTagSizeVariable, then the number of components is not
  /// known at compile time and must be queried with \c GetNumberOfComponents.
  ///
  using IsSizeStatic = viskores::VecTraitsTagSizeStatic;

  /// Returns the value in a given component of the vector.
  ///
  VISKORES_EXEC_CONT static const ComponentType& GetComponent(
    const T& vector,
    viskores::IdComponent viskoresNotUsed(component))
  {
    return vector;
  }
  /// @copydoc GetComponent
  VISKORES_EXEC_CONT static ComponentType& GetComponent(
    T& vector,
    viskores::IdComponent viskoresNotUsed(component))
  {
    return vector;
  }

  /// Changes the value in a given component of the vector.
  ///
  VISKORES_EXEC_CONT static void SetComponent(T& vector,
                                              viskores::IdComponent viskoresNotUsed(component),
                                              ComponentType value)
  {
    vector = value;
  }

  /// \brief Get a vector of the same type but with a different component.
  ///
  /// This type resolves to another vector with a different component type. For example,
  /// `viskores::VecTraits<viskores::Vec<T, N>>::%ReplaceComponentType<T2>` is `viskores::Vec<T2, N>`. This
  /// replacement is not recursive. So `VecTraits<Vec<Vec<T, M>, N>::%ReplaceComponentType<T2>` is
  /// `viskores::Vec<T2, N>`.
  ///
  // Note: the `%` in the code samples above is a hint to doxygen to avoid attempting
  // to link to the object (i.e. `ReplaceBaseComponentType`), which results in a warning.
  // The `%` is removed from the doxygen text.
  template <typename NewComponentType>
  using ReplaceComponentType = NewComponentType;

  /// \brief Get a vector of the same type but with a different base component.
  ///
  /// This type resolves to another vector with a different base component type. The replacement
  /// is recursive for nested types. For example,
  /// `VecTraits<Vec<Vec<T, M>, N>::%ReplaceBaseComponentType<T2>` is `Vec<Vec<T2, M>, N>`.
  ///
  // Note: the `%` in the code samples above is a hint to doxygen to avoid attempting
  // to link to the object (i.e. `ReplaceBaseComponentType`), which results in a warning.
  // The `%` is removed from the doxygen text.
  template <typename NewComponentType>
  using ReplaceBaseComponentType = NewComponentType;

  /// Copies the components in the given vector into a given Vec object.
  ///
  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(const T& src,
                                          viskores::Vec<ComponentType, destSize>& dest)
  {
    dest[0] = src;
  }
};

template <typename T>
using HasVecTraits VISKORES_DEPRECATED(2.1,
                                       "All types now have VecTraits defined.") = std::true_type;

// These partial specializations allow VecTraits to work with const and reference qualifiers.
template <typename T>
struct VISKORES_NEVER_EXPORT VecTraits<const T> : VecTraits<T>
{
};
template <typename T>
struct VISKORES_NEVER_EXPORT VecTraits<T&> : VecTraits<T>
{
};
template <typename T>
struct VISKORES_NEVER_EXPORT VecTraits<const T&> : VecTraits<T>
{
};

// This partial specialization allows VecTraits to work with pointers.
template <typename T>
struct VISKORES_NEVER_EXPORT VecTraits<T*> : VecTraits<T>
{
  VISKORES_EXEC_CONT static viskores::IdComponent GetNumberOfComponents(const T* vector)
  {
    return VecTraits<T>::GetNumberOfComponents(*vector);
  }
  VISKORES_EXEC_CONT static auto GetComponent(const T* vector, viskores::IdComponent component)
    -> decltype(VecTraits<T>::GetComponent(*vector, component))
  {
    return VecTraits<T>::GetComponent(*vector, component);
  }
  VISKORES_EXEC_CONT static auto GetComponent(T* vector, viskores::IdComponent component)
    -> decltype(VecTraits<T>::GetComponent(*vector, component))
  {
    return VecTraits<T>::GetComponent(*vector, component);
  }
  VISKORES_EXEC_CONT static void SetComponent(T* vector,
                                              viskores::IdComponent component,
                                              typename VecTraits<T>::ComponentType value)
  {
    VecTraits<T>::SetComponent(*vector, component, value);
  }
  template <typename NewComponentType>
  using ReplaceComponentType =
    typename VecTraits<T>::template ReplaceComponentType<NewComponentType>*;
  template <typename NewComponentType>
  using ReplaceBaseComponentType =
    typename VecTraits<T>::template ReplaceBaseComponentType<NewComponentType>*;
  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(
    const T* src,
    viskores::Vec<typename VecTraits<T>::ComponentType, destSize>& dest)
  {
    VecTraits<T>::CopyInto(*src, dest);
  }
};
template <typename T>
struct VISKORES_NEVER_EXPORT VecTraits<const T*> : VecTraits<T*>
{
};

#if defined(VISKORES_GCC) && (__GNUC__ <= 5)
namespace detail
{

template <typename NewT, viskores::IdComponent Size>
struct VecReplaceComponentTypeGCC4or5
{
  using type = viskores::Vec<NewT, Size>;
};

template <typename T, viskores::IdComponent Size, typename NewT>
struct VecReplaceBaseComponentTypeGCC4or5
{
  using type =
    viskores::Vec<typename viskores::VecTraits<T>::template ReplaceBaseComponentType<NewT>, Size>;
};

} // namespace detail
#endif // GCC Version 4.8

namespace internal
{

template <viskores::IdComponent numComponents, typename ComponentType>
struct VecTraitsMultipleComponentChooser
{
  using Type = viskores::VecTraitsTagMultipleComponents;
};

template <typename ComponentType>
struct VecTraitsMultipleComponentChooser<1, ComponentType>
{
  using Type = typename viskores::VecTraits<ComponentType>::HasMultipleComponents;
};

} // namespace internal

template <typename T, viskores::IdComponent Size>
struct VISKORES_NEVER_EXPORT VecTraits<viskores::Vec<T, Size>>
{
  using VecType = viskores::Vec<T, Size>;

  /// \brief Type of the components in the vector.
  ///
  /// If the type is really a scalar, then the component type is the same as the scalar type.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// \brief Base component type in the vector.
  ///
  /// Similar to ComponentType except that for nested vectors (e.g. Vec<Vec<T, M>, N>), it
  /// returns the base scalar type at the end of the composition (T in this example).
  ///
  using BaseComponentType = typename viskores::VecTraits<ComponentType>::BaseComponentType;

  /// Number of components in the vector.
  ///
  static constexpr viskores::IdComponent NUM_COMPONENTS = VecType::NUM_COMPONENTS;

  /// Number of components in the given vector.
  ///
  VISKORES_EXEC_CONT
  static viskores::IdComponent GetNumberOfComponents(const VecType&) { return NUM_COMPONENTS; }

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  using HasMultipleComponents =
    typename internal::VecTraitsMultipleComponentChooser<NUM_COMPONENTS, ComponentType>::Type;

  /// A tag specifying whether the size of this vector is known at compile
  /// time. If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set.
  /// If set to \c VecTraitsTagSizeVariable, then the number of components is
  /// not known at compile time and must be queried with \c
  /// GetNumberOfComponents.
  ///
  using IsSizeStatic = viskores::VecTraitsTagSizeStatic;

  /// Returns the value in a given component of the vector.
  ///
  VISKORES_EXEC_CONT
  static const ComponentType& GetComponent(const VecType& vector, viskores::IdComponent component)
  {
    return vector[component];
  }
  VISKORES_EXEC_CONT
  static ComponentType& GetComponent(VecType& vector, viskores::IdComponent component)
  {
    return vector[component];
  }

  /// Changes the value in a given component of the vector.
  ///
  VISKORES_EXEC_CONT static void SetComponent(VecType& vector,
                                              viskores::IdComponent component,
                                              ComponentType value)
  {
    vector[component] = value;
  }

/// \brief Get a vector of the same type but with a different component.
///
/// This type resolves to another vector with a different component type. For example,
/// @code viskores::VecTraits<viskores::Vec<T, N>>::ReplaceComponentType<T2> @endcode is viskores::Vec<T2, N>.
/// This replacement is not recursive. So @code VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> @endcode
/// is viskores::Vec<T2, N>.
///@{
#if defined(VISKORES_GCC) && (__GNUC__ <= 5)
  // Silly workaround for bug in GCC <= 5
  template <typename NewComponentType>
  using ReplaceComponentType =
    typename detail::VecReplaceComponentTypeGCC4or5<NewComponentType, Size>::type;
#else // !GCC <= 5
  template <typename NewComponentType>
  using ReplaceComponentType = viskores::Vec<NewComponentType, Size>;
#endif
///@}

/// \brief Get a vector of the same type but with a different base component.
///
/// This type resolves to another vector with a different base component type. The replacement
/// is recursive for nested types. For example,
/// @code VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> @endcode is Vec<Vec<T2, M>, N>.
///@{
#if defined(VISKORES_GCC) && (__GNUC__ <= 5)
  // Silly workaround for bug in GCC <= 5
  template <typename NewComponentType>
  using ReplaceBaseComponentType =
    typename detail::VecReplaceBaseComponentTypeGCC4or5<T, Size, NewComponentType>::type;
#else // !GCC <= 5
  template <typename NewComponentType>
  using ReplaceBaseComponentType =
    viskores::Vec<typename viskores::VecTraits<ComponentType>::template ReplaceBaseComponentType<
                    NewComponentType>,
                  Size>;
#endif
  ///@}

  /// Converts whatever type this vector is into the standard Viskores Tuple.
  ///
  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(const VecType& src,
                                          viskores::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

template <typename T>
struct VISKORES_NEVER_EXPORT VecTraits<viskores::VecC<T>>
{
  using VecType = viskores::VecC<T>;

  /// \brief Type of the components in the vector.
  ///
  /// If the type is really a scalar, then the component type is the same as the scalar type.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// \brief Base component type in the vector.
  ///
  /// Similar to ComponentType except that for nested vectors (e.g. Vec<Vec<T, M>, N>), it
  /// returns the base scalar type at the end of the composition (T in this example).
  ///
  using BaseComponentType = typename viskores::VecTraits<ComponentType>::BaseComponentType;

  /// Number of components in the given vector.
  ///
  VISKORES_EXEC_CONT
  static viskores::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  /// The size of a \c VecC is not known until runtime and can always
  /// potentially have multiple components, this is always set to \c
  /// HasMultipleComponents.
  ///
  using HasMultipleComponents = viskores::VecTraitsTagMultipleComponents;

  /// A tag specifying whether the size of this vector is known at compile
  /// time. If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set.
  /// If set to \c VecTraitsTagSizeVariable, then the number of components is
  /// not known at compile time and must be queried with \c
  /// GetNumberOfComponents.
  ///
  using IsSizeStatic = viskores::VecTraitsTagSizeVariable;

  /// Returns the value in a given component of the vector.
  ///
  VISKORES_EXEC_CONT
  static const ComponentType& GetComponent(const VecType& vector, viskores::IdComponent component)
  {
    return vector[component];
  }
  VISKORES_EXEC_CONT
  static ComponentType& GetComponent(VecType& vector, viskores::IdComponent component)
  {
    return vector[component];
  }

  /// Changes the value in a given component of the vector.
  ///
  VISKORES_EXEC_CONT
  static void SetComponent(VecType& vector, viskores::IdComponent component, ComponentType value)
  {
    vector[component] = value;
  }

  /// \brief Get a vector of the same type but with a different component.
  ///
  /// This type resolves to another vector with a different component type. For example,
  /// @code viskores::VecTraits<viskores::Vec<T, N>>::ReplaceComponentType<T2> @endcode is viskores::Vec<T2, N>.
  /// This replacement is not recursive. So @code VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> @endcode
  /// is viskores::Vec<T2, N>.
  ///
  template <typename NewComponentType>
  using ReplaceComponentType = viskores::VecC<NewComponentType>;

  /// \brief Get a vector of the same type but with a different base component.
  ///
  /// This type resolves to another vector with a different base component type. The replacement
  /// is recursive for nested types. For example,
  /// @code VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> @endcode is Vec<Vec<T2, M>, N>.
  ///
  template <typename NewComponentType>
  using ReplaceBaseComponentType = viskores::VecC<typename viskores::VecTraits<
    ComponentType>::template ReplaceBaseComponentType<NewComponentType>>;

  /// Converts whatever type this vector is into the standard Viskores Tuple.
  ///
  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(const VecType& src,
                                          viskores::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

template <typename T>
struct VISKORES_NEVER_EXPORT VecTraits<viskores::VecCConst<T>>
{
  using VecType = viskores::VecCConst<T>;

  /// \brief Type of the components in the vector.
  ///
  /// If the type is really a scalar, then the component type is the same as the scalar type.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// \brief Base component type in the vector.
  ///
  /// Similar to ComponentType except that for nested vectors (e.g. Vec<Vec<T, M>, N>), it
  /// returns the base scalar type at the end of the composition (T in this example).
  ///
  using BaseComponentType = typename viskores::VecTraits<ComponentType>::BaseComponentType;

  /// Number of components in the given vector.
  ///
  VISKORES_EXEC_CONT
  static viskores::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  /// The size of a \c VecCConst is not known until runtime and can always
  /// potentially have multiple components, this is always set to \c
  /// HasMultipleComponents.
  ///
  using HasMultipleComponents = viskores::VecTraitsTagMultipleComponents;

  /// A tag specifying whether the size of this vector is known at compile
  /// time. If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set.
  /// If set to \c VecTraitsTagSizeVariable, then the number of components is
  /// not known at compile time and must be queried with \c
  /// GetNumberOfComponents.
  ///
  using IsSizeStatic = viskores::VecTraitsTagSizeVariable;

  /// Returns the value in a given component of the vector.
  ///
  VISKORES_EXEC_CONT
  static const ComponentType& GetComponent(const VecType& vector, viskores::IdComponent component)
  {
    return vector[component];
  }

  /// Changes the value in a given component of the vector.
  ///
  VISKORES_EXEC_CONT
  static void SetComponent(VecType& vector, viskores::IdComponent component, ComponentType value)
  {
    vector[component] = value;
  }

  /// \brief Get a vector of the same type but with a different component.
  ///
  /// This type resolves to another vector with a different component type. For example,
  /// @code viskores::VecTraits<viskores::Vec<T, N>>::ReplaceComponentType<T2> @endcode is viskores::Vec<T2, N>.
  /// This replacement is not recursive. So @code VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> @endcode
  /// is viskores::Vec<T2, N>.
  ///
  template <typename NewComponentType>
  using ReplaceComponentType = viskores::VecCConst<NewComponentType>;

  /// \brief Get a vector of the same type but with a different base component.
  ///
  /// This type resolves to another vector with a different base component type. The replacement
  /// is recursive for nested types. For example,
  /// @code VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> @endcode is Vec<Vec<T2, M>, N>.
  ///
  template <typename NewComponentType>
  using ReplaceBaseComponentType = viskores::VecCConst<typename viskores::VecTraits<
    ComponentType>::template ReplaceBaseComponentType<NewComponentType>>;

  /// Converts whatever type this vector is into the standard Viskores Tuple.
  ///
  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(const VecType& src,
                                          viskores::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

namespace internal
{

/// Used for overriding VecTraits for basic scalar types.
///
template <typename ScalarType>
struct VISKORES_DEPRECATED(2.1, "VecTraitsBasic is now the default implementation for VecTraits.")
  VISKORES_NEVER_EXPORT VecTraitsBasic
{
  using ComponentType = ScalarType;
  using BaseComponentType = ScalarType;
  static constexpr viskores::IdComponent NUM_COMPONENTS = 1;
  using HasMultipleComponents = viskores::VecTraitsTagSingleComponent;
  using IsSizeStatic = viskores::VecTraitsTagSizeStatic;

  VISKORES_EXEC_CONT
  static viskores::IdComponent GetNumberOfComponents(const ScalarType&) { return 1; }

  VISKORES_EXEC_CONT
  static const ComponentType& GetComponent(const ScalarType& vector, viskores::IdComponent)
  {
    return vector;
  }
  VISKORES_EXEC_CONT
  static ComponentType& GetComponent(ScalarType& vector, viskores::IdComponent) { return vector; }

  VISKORES_EXEC_CONT static void SetComponent(ScalarType& vector,
                                              viskores::IdComponent,
                                              ComponentType value)
  {
    vector = value;
  }

  template <typename NewComponentType>
  using ReplaceComponentType = NewComponentType;

  template <typename NewComponentType>
  using ReplaceBaseComponentType = NewComponentType;

  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(const ScalarType& src,
                                          viskores::Vec<ScalarType, destSize>& dest)
  {
    dest[0] = src;
  }
};

template <typename T>
struct VISKORES_DEPRECATED(2.1 "VecTraits now safe to use on any type.")
  VISKORES_NEVER_EXPORT SafeVecTraits : viskores::VecTraits<T>
{
};

} // namespace internal

namespace detail
{

struct VISKORES_DEPRECATED(
  2.1,
  "VISKORES_BASIC_TYPE_VECTOR is no longer necessary because VecTraits implements "
  "basic type by default.") VISKORES_BASIC_TYPE_VECTOR_is_deprecated
{
};

template <typename T>
struct issue_VISKORES_BASIC_TYPE_VECTOR_deprecation_warning;

}

} // namespace viskores

#define VISKORES_BASIC_TYPE_VECTOR(type)                                \
  namespace viskores                                                    \
  {                                                                     \
  namespace detail                                                      \
  {                                                                     \
  template <>                                                           \
  struct issue_VISKORES_BASIC_TYPE_VECTOR_deprecation_warning<type>     \
    : public viskores::detail::VISKORES_BASIC_TYPE_VECTOR_is_deprecated \
  {                                                                     \
  };                                                                    \
  }                                                                     \
  }

#endif //viskores_VecTraits_h
