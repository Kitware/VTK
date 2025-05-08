//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_FieldAccessor_h
#define lcl_FieldAccessor_h

#include <lcl/internal/Config.h>

#include <utility>

namespace lcl
{
namespace internal
{
//----------------------------------------------------------------------------
/// Does T support the indexing operator?
/// Examples:
///   std::array
///   std::vector
///   T*
///   custom classes with operator[](integer_type)
///

struct NotVecTag {};

template <typename T> inline decltype(std::declval<T>()[0]) VecCheck(T);
inline NotVecTag VecCheck(...);

template <typename T>
using IsVecType =
  std::integral_constant<bool, !std::is_same<decltype(VecCheck(std::declval<T>())),
                                             internal::NotVecTag>::value>;

//----------------------------------------------------------------------------
/// Does T support operator()(integer_type)?

struct NotFuncTag {};

template <typename T> inline decltype(std::declval<T>()(0)) FuncCheck(T);
inline NotFuncTag FuncCheck(...);

template <typename T>
using IsFuncType =
  std::integral_constant<bool, !std::is_same<decltype(FuncCheck(std::declval<T>())),
                                             internal::NotFuncTag>::value>;

//----------------------------------------------------------------------------
template <typename T> typename T::size_type IndexTypeImpl(T);
IdComponent IndexTypeImpl(...);

template <typename VecT>
using IndexType = decltype(IndexTypeImpl(std::declval<VecT>()));

//----------------------------------------------------------------------------
struct VecTypeIndexer {};
struct VecTypeFunctor {};
struct VecTypeScalar  {};

template <typename T>
using VecTypeTag =
  typename std::conditional<IsVecType<T>::value,
                            VecTypeIndexer,
                            typename std::conditional<IsFuncType<T>::value,
                                                      VecTypeFunctor,
                                                      VecTypeScalar>::type>::type;

//----------------------------------------------------------------------------
template <typename T>
LCL_EXEC auto componentImpl(VecTypeIndexer, T&& vec, int idx) -> decltype(vec[0])
{
  return vec[static_cast<IndexType<T>>(idx)];
}

template <typename T>
LCL_EXEC auto componentImpl(VecTypeFunctor, T&& vec, int idx) -> decltype(vec(0))
{
  return vec(idx);
}

template <typename T>
LCL_EXEC T& componentImpl(VecTypeScalar, T&& vec, int)
{
  return vec;
}

} // namespace internal

//----------------------------------------------------------------------------
template <typename T>
LCL_EXEC auto component(T&& vec, int idx)
  -> decltype(internal::componentImpl<T>(internal::VecTypeTag<T>{}, std::forward<T>(vec), 0))
{
  return internal::componentImpl<T>(internal::VecTypeTag<T>{}, std::forward<T>(vec), idx);
}

//----------------------------------------------------------------------------
template <typename T>
using ComponentType = typename std::decay<decltype(component(std::declval<T>(), 0))>::type;

///============================================================================
/// Since there are different ways fields maybe represented in the clients of
/// this libarary, LCL relies on helper classes that implement the
/// \c FieldAccessor "concept" to access the elements of a field.
///
/// These classes should wrap the field and provide the follolwing interface:
///
/// template <typename FieldType>
/// class FieldAccessor
/// {
/// public:
///   /// An alias for the component type of the field
///   using ValueType = ...;
///
///   /// Return the number of components
///   int getNumberOfComponents() const;
///
///   /// Set the value to `tuple` and `comp` to `value`.
///   void setValue(int tuple, int comp, const ValueType& value) const;
///
///   /// Get the value at `tuple` and `comp`.
///   ValueType getValue(int tuple, int comp) const;
///
///   /// Set the tuple at index `tuple`. It is recomended to make this a
///   /// template function and use `lcl::component` to access
///   /// the components of `value`.
///   template <typename VecType>
///   void setTuple(int tuple, const VecType& value) const;
///
///   /// Get the tuple at index `tuple`. It is recomended to make this a
///   /// template function and use `lcl::component` to access
///   /// the components of `value`.
///   template <typename VecType>
///   void getTuple(int tuple, VecType& value) const;
/// }
///
/// The set functions are optional and such a class would act as a const variant
/// of \c FieldAccessor.
///

///----------------------------------------------------------------------------
template <typename FieldType>
class FieldAccessorNestedSOA
{
public:
  using ValueType = typename std::decay<decltype(component(std::declval<FieldType>()[0], 0))>::type;

  LCL_EXEC FieldAccessorNestedSOA(FieldType& field, int numberOfComponents = 1)
    : Field(&field), NumberOfComponents(numberOfComponents)
  {
  }

  LCL_EXEC int getNumberOfComponents() const
  {
    return this->NumberOfComponents;
  }

  LCL_EXEC void setValue(int tuple, int comp, const ValueType& value) const
  {
    component((*this->Field)[static_cast<IdxType>(tuple)], comp) = value;
  }

  LCL_EXEC ValueType getValue(int tuple, int comp) const
  {
    return component((*this->Field)[static_cast<IdxType>(tuple)], comp);
  }

  template <typename VecType>
  LCL_EXEC void setTuple(int tuple, const VecType& value) const
  {
    for (int i = 0; i < this->NumberOfComponents; ++i)
    {
      component((*this->Field)[static_cast<IdxType>(tuple)], i) =
        static_cast<ValueType>(component(value, i));
    }
  }

  template <typename VecType>
  LCL_EXEC void getTuple(int tuple, VecType& value) const
  {
    for (int i = 0; i < this->NumberOfComponents; ++i)
    {
      component(value, i) = static_cast<ComponentType<VecType>>(
        component((*this->Field)[static_cast<IdxType>(tuple)], i));
    }
  }

private:
  using IdxType = internal::IndexType<FieldType>;

  FieldType* Field;
  int NumberOfComponents;
};

template <typename FieldType>
class FieldAccessorNestedSOAConst
{
public:
  using ValueType = typename std::decay<decltype(component(std::declval<FieldType>()[0], 0))>::type;

  LCL_EXEC FieldAccessorNestedSOAConst(const FieldType& field, int numberOfComponents = 1)
    : Field(&field), NumberOfComponents(numberOfComponents)
  {
  }

  LCL_EXEC int getNumberOfComponents() const
  {
    return this->NumberOfComponents;
  }

  LCL_EXEC ValueType getValue(int tuple, int comp) const
  {
    return component((*this->Field)[static_cast<IdxType>(tuple)], comp);
  }

  template <typename VecType>
  LCL_EXEC void getTuple(int tuple, VecType& value) const
  {
    for (int i = 0; i < this->NumberOfComponents; ++i)
    {
      component(value, i) = static_cast<ComponentType<VecType>>(
        component((*this->Field)[static_cast<IdxType>(tuple)], i));
    }
  }

private:
  using IdxType = internal::IndexType<FieldType>;

  const FieldType* Field;
  int NumberOfComponents;
};

template <typename FieldType>
LCL_EXEC
FieldAccessorNestedSOA<FieldType> makeFieldAccessorNestedSOA(FieldType& field,
                                                             int numberOfComponents = 1)
{
  return FieldAccessorNestedSOA<FieldType>(field, numberOfComponents);
}

template <typename FieldType>
LCL_EXEC
FieldAccessorNestedSOAConst<FieldType> makeFieldAccessorNestedSOAConst(const FieldType& field,
                                                                       int numberOfComponents = 1)
{
  return FieldAccessorNestedSOAConst<FieldType>(field, numberOfComponents);
}

///----------------------------------------------------------------------------
template <typename FieldType>
class FieldAccessorFlatSOA
{
public:
  using ValueType = typename std::decay<decltype(std::declval<FieldType>()[0])>::type;

  LCL_EXEC FieldAccessorFlatSOA(FieldType& field, int numberOfComponents = 1)
    : Field(&field), NumberOfComponents(numberOfComponents)
  {
  }

  LCL_EXEC int getNumberOfComponents() const
  {
    return this->NumberOfComponents;
  }

  LCL_EXEC void setValue(int tuple, int comp, const ValueType& value) const
  {
    auto FlatIdx = static_cast<IdxType>(tuple * this->NumberOfComponents + comp);
    (*this->Field)[FlatIdx] = value;
  }

  LCL_EXEC ValueType getValue(int tuple, int comp) const
  {
    auto FlatIdx = static_cast<IdxType>(tuple * this->NumberOfComponents + comp);
    return (*this->Field)[FlatIdx];
  }

  template <typename VecType>
  LCL_EXEC void setTuple(int tuple, const VecType& value) const
  {
    auto start = static_cast<IdxType>(tuple * this->NumberOfComponents);
    for (int i = 0; i < this->NumberOfComponents; ++i)
    {
      (*this->Field)[start++] = static_cast<ValueType>(component(value, i));
    }
  }

  template <typename VecType>
  LCL_EXEC void getTuple(int tuple, VecType& value) const
  {
    auto start = static_cast<IdxType>(tuple * this->NumberOfComponents);
    for (int i = 0; i < this->NumberOfComponents; ++i)
    {
      component(value, i) = static_cast<ComponentType<VecType>>((*this->Field)[start++]);
    }
  }

private:
  using IdxType = internal::IndexType<FieldType>;

  FieldType* Field;
  int NumberOfComponents;
};

template <typename FieldType>
class FieldAccessorFlatSOAConst
{
public:
  using ValueType = typename std::decay<decltype(std::declval<FieldType>()[0])>::type;

  LCL_EXEC FieldAccessorFlatSOAConst(const FieldType& field, int numberOfComponents = 1)
    : Field(&field), NumberOfComponents(numberOfComponents)
  {
  }

  LCL_EXEC int getNumberOfComponents() const
  {
    return this->NumberOfComponents;
  }

  LCL_EXEC ValueType getValue(int tuple, int comp) const
  {
    auto FlatIdx = static_cast<IdxType>(tuple * this->NumberOfComponents + comp);
    return (*this->Field)[FlatIdx];
  }

  template <typename VecType>
  LCL_EXEC void getTuple(int tuple, VecType& value) const
  {
    auto start = static_cast<IdxType>(tuple * this->NumberOfComponents);
    for (int i = 0; i < this->NumberOfComponents; ++i)
    {
      component(value, i) = static_cast<ComponentType<VecType>>((*this->Field)[start++]);
    }
  }

private:
  using IdxType = internal::IndexType<FieldType>;

  const FieldType* Field;
  int NumberOfComponents;
};

template <typename FieldType>
LCL_EXEC
FieldAccessorFlatSOA<FieldType> makeFieldAccessorFlatSOA(FieldType& field,
                                                         int numberOfComponents)
{
  return FieldAccessorFlatSOA<FieldType>(field, numberOfComponents);
}

template <typename FieldType>
LCL_EXEC
FieldAccessorFlatSOAConst<FieldType> makeFieldAccessorFlatSOAConst(const FieldType& field,
                                                                   int numberOfComponents)
{
  return FieldAccessorFlatSOAConst<FieldType>(field, numberOfComponents);
}

} // namespace lcl

#endif // lcl_FieldAccessor_h
