//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef vtkToDax_vtkPointsContainer_h
#define vtkToDax_vtkPointsContainer_h

#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkPoints.h"

#include <dax/Types.h>
#include <dax/VectorTraits.h>
#include <dax/cont/ArrayPortal.h>
#include <dax/cont/internal/IteratorFromArrayPortal.h>
#include <iterator>

//this is needed so that we can properly deduce if we are a const
//portal. and if we are a const portal properly move the const
//to be each vaule of the dax tuple, instead of the tuple itself
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/mpl/if.hpp>

namespace
{
template<int N>
struct fillComponents
{
  template<typename T, typename Tuple>
  void operator()(T* t, const Tuple& tuple) const
  {
    fillComponents<N-1>()(t,tuple);
    t[N-1]=dax::VectorTraits<Tuple>::GetComponent(tuple,N-1);
  }
};

template<>
struct fillComponents<1>
  {
  template<typename T, typename Tuple>
  void operator()(T* t, const Tuple& tuple) const
  {
    t[0]=dax::VectorTraits<Tuple>::GetComponent(tuple,0);
  }
};

template<int N>
struct readComponents
{
  template<typename T, typename Tuple>
  void operator()(const T* t, Tuple& tuple) const
  {
    readComponents<N-1>()(t,tuple);
    dax::VectorTraits<Tuple>::SetComponent(tuple,N-1,t[N-1]);
  }
};

template<>
struct readComponents<1>
{
  template<typename T, typename Tuple>
  void operator()(const T* t, Tuple& tuple) const
  {
    dax::VectorTraits<Tuple>::SetComponent(tuple,0,t[0]);
  }
};

template<typename ValueType, int N>
struct readVector
{
  template<typename T>
  ValueType operator()(const T* rawArray)
  {
  ValueType temp;
  readComponents<N>()(rawArray,temp);
  return temp;
  }
};

template<typename ValueType>
struct readVector<ValueType,1>
{
  template<typename T>
  ValueType operator()(const T* rawArray)
  {
  return ValueType(*rawArray);
  }
};


template<typename T>
struct ConstCorrectedType
{
  //get the number of components in T
  static const int NUM_COMPONENTS = dax::VectorTraits<T>::NUM_COMPONENTS;
  typedef typename dax::VectorTraits<T>::ComponentType ComponentType;

  //on arrayPortal and PointsPortal we are generating
  //T on the fly, so it be const T since
  typedef typename boost::remove_const<T>::type correctConstT;
  typedef typename boost::is_const<T>::type isConst;
  typedef typename boost::mpl::if_<isConst,correctConstT,T>::type Type;
};

}

namespace vtkToDax
{


template <typename Type,
          int NUM_COMPONENTS = dax::VectorTraits<Type>::NUM_COMPONENTS>
class vtkArrayPortal
{
public:
  typedef typename ConstCorrectedType<Type>::Type ValueType;
  typedef typename ConstCorrectedType<Type>::ComponentType ComponentType;

  DAX_CONT_EXPORT vtkArrayPortal():
    Data(NULL),
    Array(NULL),
    Size(0)
    {
    }

  DAX_CONT_EXPORT vtkArrayPortal(vtkDataArray* array, dax::Id size):
    Data(array),
    Array(static_cast<ComponentType*>(array->GetVoidPointer(0))),
    Size(size)
    {
    DAX_ASSERT_CONT(this->GetNumberOfValues() >= 0);
    }


  /// Copy constructor for any other vtkArrayPortal with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<typename OtherType>
  DAX_CONT_EXPORT
  vtkArrayPortal(const vtkArrayPortal<OtherType> &src):
    Data(src.GetVtkData()),
    Array(static_cast<ComponentType*>(src.GetVtkData()->GetVoidPointer(0))),
    Size(src.GetNumberOfValues())
    {
    }

  DAX_CONT_EXPORT
  dax::Id GetNumberOfValues() const
    {
    return this->Size;
    }

  DAX_CONT_EXPORT
  ValueType Get(dax::Id index) const
    {
    const ComponentType *rawArray = this->Array + (index * NUM_COMPONENTS);
    return readVector<ValueType,NUM_COMPONENTS>()(rawArray);
    }

  DAX_CONT_EXPORT
  void Set(dax::Id index, const ValueType& value) const
    {
    ComponentType *rawArray = this->Array + (index * NUM_COMPONENTS);
    //use template magic to auto unroll insertion
    fillComponents<NUM_COMPONENTS>()(rawArray,value);
    }

  typedef dax::cont::internal::IteratorFromArrayPortal<vtkArrayPortal>
                                                                  IteratorType;
  DAX_CONT_EXPORT IteratorType GetIteratorBegin() const
    {
    return IteratorType(*this, 0);
    }

  DAX_CONT_EXPORT IteratorType GetIteratorEnd() const
    {
    return IteratorType(*this, this->Size);
    }

  vtkDataArray* GetVtkData() const { return Data; }

private:
  vtkDataArray* Data;
  ComponentType *Array;
  dax::Id Size;

};


template <typename Type,
          int NUM_COMPONENTS = dax::VectorTraits<Type>::NUM_COMPONENTS>
class vtkPointsPortal
{
public:
  typedef typename ConstCorrectedType<Type>::Type ValueType;
  typedef typename ConstCorrectedType<Type>::ComponentType ComponentType;

  DAX_CONT_EXPORT vtkPointsPortal():
    Points(NULL),
    Array(NULL),
    Size(0)
    {
    }

  DAX_CONT_EXPORT vtkPointsPortal(vtkPoints* points, dax::Id size):
    Points(points),
    Array(static_cast<ComponentType*>(points->GetVoidPointer(0))),
    Size(size)
    {
    DAX_ASSERT_CONT(this->GetNumberOfValues() >= 0);
    }

  /// Copy constructor for any other vtkArrayPortal with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<typename OtherType>
  DAX_CONT_EXPORT
  vtkPointsPortal(const vtkPointsPortal<OtherType> &src):
    Points(src.GetVtkData()),
    Array(static_cast<ComponentType*>(src.GetVtkData()->GetVoidPointer(0))),
    Size(src.GetNumberOfValues())
    {
    }

  DAX_CONT_EXPORT
  dax::Id GetNumberOfValues() const
    {
    return this->Size;
    }

  DAX_CONT_EXPORT
  ValueType Get(dax::Id index) const
    {
    return ValueType(this->Array+(index*NUM_COMPONENTS));
    }

  DAX_CONT_EXPORT
  void Set(dax::Id index, const ValueType& value) const
    {
    ComponentType *rawArray = this->Array + (index * NUM_COMPONENTS);
    //use template magic to auto unroll insertion
    fillComponents<NUM_COMPONENTS>()(rawArray,value);
    }

  typedef dax::cont::internal::IteratorFromArrayPortal<vtkPointsPortal>
                                                                  IteratorType;
  DAX_CONT_EXPORT IteratorType GetIteratorBegin() const
    {
    return IteratorType(*this, 0);
    }

  DAX_CONT_EXPORT IteratorType GetIteratorEnd() const
    {
    return IteratorType(*this, this->Size);
    }

  vtkPoints* GetVtkData() const { return Points; }

private:
  vtkPoints* Points;
  ComponentType *Array;
  dax::Id Size;

};

//A topology portal goal is to make the vtkCellArray for a continous cell type
//look like a dax topology layout. This means that we skip over the elements
//in the vtkCellArray that state how many number of points are in each cell
//so for example a vtkCellArray of triangles is stored like:
// 3, 0, 2, 1, 3, 0, 3, 1,
//and we want it to be in Dax:
// 0, 2, 1, 0, 3, 1

template<typename T, int PointsPerCell>
class vtkTopologyPortal
{
public:
  typedef T ValueType;
  DAX_CONT_EXPORT vtkTopologyPortal():
    CellArray(NULL),
    RawCells(NULL),
    Size(0)
    {
    }

  //numOfEntries should be the length of the cell topology array as far
  //as dax is concerned.
  DAX_CONT_EXPORT vtkTopologyPortal(vtkCellArray* cells, dax::Id daxTopoLen):
    CellArray(cells),
    RawCells(cells->GetPointer()),
    Size(daxTopoLen)
    {
    DAX_ASSERT_CONT(this->GetNumberOfValues() >= 0);
    DAX_ASSERT_CONT(this->CellArray->GetNumberOfConnectivityEntries() >=
                    daxTopoLen + (daxTopoLen/PointsPerCell));
    }

  /// Copy constructor for any other vtkArrayPortal with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<typename OtherType>
  DAX_CONT_EXPORT
  vtkTopologyPortal(const vtkTopologyPortal<OtherType,PointsPerCell> &src):
    CellArray(src.GetVtkData()),
    RawCells(src.GetVtkData()->GetPointer()),
    Size(src.GetNumberOfValues())
  {
  }

  DAX_CONT_EXPORT
  dax::Id GetNumberOfValues() const{
    return this->Size;
    }


  DAX_CONT_EXPORT
  ValueType Get(dax::Id index) const{
    return this->RawCells[1 + index + (index/PointsPerCell) ];
  }

  DAX_CONT_EXPORT
  void Set(dax::Id index, const ValueType& value) const{
    this->RawCells[1 + index + index/PointsPerCell]=value;
  }

  typedef dax::cont::internal::IteratorFromArrayPortal<vtkTopologyPortal>
                                                                  IteratorType;
  DAX_CONT_EXPORT IteratorType GetIteratorBegin() const
    {
    return IteratorType(*this, 0);
    }

  DAX_CONT_EXPORT IteratorType GetIteratorEnd() const
    {
    return IteratorType(*this, this->Size);
    }

  vtkCellArray* GetVtkData() const { return CellArray; }

private:
  vtkCellArray *CellArray;
  vtkIdType *RawCells;
  dax::Id Size;

};

}



#endif // vtkToDax_vtkPointsContainer_h
