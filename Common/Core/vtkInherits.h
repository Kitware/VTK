// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkInherits_h
#define vtkInherits_h

#include "vtkTypeName.h"

#include <string>

#define vtkInheritanceHierarchyBodyMacro(thisClass)                                                \
  {                                                                                                \
    std::vector<vtkStringToken> result;                                                            \
    vtk::Inherits<thisClass>(result);                                                              \
    return result;                                                                                 \
  }

/// Use these macros to add an "InheritanceHierarchy()" method to a VTK class.
///
/// Note: To use this macro, you must `#include "vtkStringToken.h"` and
/// `#include <vector>` as the method returns a vector of string tokens.
//@{
#define vtkInheritanceHierarchyBaseMacro(thisClass)                                                \
  virtual std::vector<vtkStringToken> InheritanceHierarchy()                                       \
    const vtkInheritanceHierarchyBodyMacro(thisClass)

#define vtkInheritanceHierarchyOverrideMacro(thisClass)                                            \
  std::vector<vtkStringToken> InheritanceHierarchy()                                               \
    const override vtkInheritanceHierarchyBodyMacro(thisClass)
//@}

namespace vtk
{
namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

// Used by Inherits with ParentClasses to produce a list of inherited type names.
template <typename Container, typename StopAtType = void>
struct AddNames
{
  AddNames(Container& c)
    : m_container(c)
  {
  }

  template <typename T>
  bool operator()()
  {
    if (!std::is_same<StopAtType, T>::value)
    {
      std::string typeName = vtk::TypeName<T>();
      m_container.insert(m_container.end(), typeName);
      return true;
    }
    return false;
  }
  Container& m_container;
};

VTK_ABI_NAMESPACE_END
} // namespace detail

VTK_ABI_NAMESPACE_BEGIN
///@{
/**
 * Determine whether the provided class (\a VTKObjectType ) has a parent class.
 *
 * The \a VTKObjectType template-parameter should be a subclass of vtkObjectBase
 * that uses the `vtkTypeMacro()` to define a `Superclass` type-alias.
 * The `value` in this class is true when a Superclass type-alias exists and
 * false otherwise.
 */
template <typename VTKObjectType>
class HasSuperclass
{
  class No
  {
  };
  class Yes
  {
    No no[2];
  };

  template <typename C>
  static Yes Test(typename C::Superclass*);
  template <typename C>
  static No Test(...);

public:
  enum
  {
    value = sizeof(Test<VTKObjectType>(nullptr)) == sizeof(Yes)
  };
};
///@}

///@{
/**
 * Invoke a functor on the named type and each of its parent types.
 *
 * The \a VTKObjectType template-parameter should be a subclass of vtkObjectBase
 * that uses the `vtkTypeMacro()` to define a `Superclass` type-alias, as this
 * is how the inheritance hierarchy is traversed.
 *
 * Call the static `enumerate()` method of ParentClasses with a functor
 * that accepts no arguments and a single template parameter.
 * If the return value of your functor is void, then `enumerate()` will invoke
 * your functor once on every type in your object's hierarchy.
 * If the return value of your functor is a boolean, then `enumerate()` will
 * invoke your functor on every type in your object's hierarchy until the
 * functor returns false (indicating early termination is requested).
 * See detail::AddNames() above for an example of the latter.
 */
template <typename VTKObjectType, bool IsDerived = HasSuperclass<VTKObjectType>::value>
struct ParentClasses;

template <typename VTKObjectType>
struct ParentClasses<VTKObjectType, false>
{
  template <typename Functor>
  static void enumerate(Functor& ff)
  {
    ff.template operator()<VTKObjectType>();
  }
};

template <typename VTKObjectType>
struct ParentClasses<VTKObjectType, true>
{
  // This variant handles Functors with a void return type.
  template <typename Functor>
  static typename std::enable_if<
    std::is_same<decltype(std::declval<Functor>().template operator()<vtkObject>()), void>::value,
    void>::type enumerate(Functor& ff)
  {
    ff.template operator()<VTKObjectType>();
    ParentClasses<typename VTKObjectType::Superclass>::enumerate(ff);
  }

  // This variant handles Functors with a bool return type.
  template <typename Functor>
  static typename std::enable_if<
    std::is_same<decltype(std::declval<Functor>().template operator()<vtkObject>()), bool>::value,
    void>::type enumerate(Functor& ff)
  {
    if (ff.template operator()<VTKObjectType>())
    {
      ParentClasses<typename VTKObjectType::Superclass>::enumerate(ff);
    }
  }
};
///@}

///@{
/**
 * Populate the \a container with the name of this class and its ancestors.
 *
 * The \a VTKObjectType template-parameter should be a subclass of vtkObjectBase
 * that uses the `vtkTypeMacro()` to define a `Superclass` type-alias, as this
 * is how the inheritance hierarchy is traversed.
 *
 * The version of this function that accepts 2 template parameters
 * uses the second parameter to iterate over a partial hierarchy
 * truncated at (not including) the \a StopAtType.
 */
template <typename VTKObjectType, typename Container>
void Inherits(Container& container)
{
  detail::AddNames<Container> addNames(container);
  ParentClasses<VTKObjectType>::enumerate(addNames);
}

template <typename VTKObjectType, typename StopAtType, typename Container>
void Inherits(Container& container)
{
  detail::AddNames<Container, StopAtType> addNames(container);
  ParentClasses<VTKObjectType>::enumerate(addNames);
}
///@}

VTK_ABI_NAMESPACE_END
} // namespace vtk

#endif // vtkInherits_h
