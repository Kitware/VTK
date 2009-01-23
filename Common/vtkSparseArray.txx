/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSparseArray.txx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkSparseArray_txx
#define __vtkSparseArray_txx

template<typename T>
vtkSparseArray<T>* vtkSparseArray<T>::New()
{
  vtkObject* ret = vtkObjectFactory::CreateInstance(typeid(ThisT).name());
  if(ret)
    {
    return static_cast<ThisT*>(ret);
    }
  return new ThisT();
}

template<typename T>
void vtkSparseArray<T>::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSparseArray<T>::Superclass::PrintSelf(os, indent);
}

template<typename T>
vtkArrayExtents vtkSparseArray<T>::GetExtents()
{
  return this->Extents;
}

template<typename T>
vtkIdType vtkSparseArray<T>::GetNonNullSize()
{
  return this->ValueEnd - this->ValueBegin;
}

template<typename T>
void vtkSparseArray<T>::GetCoordinatesN(const vtkIdType n, vtkArrayCoordinates& coordinates)
{
  coordinates.SetDimensions(this->GetDimensions());
  for(vtkIdType i = 0; i != this->GetDimensions(); ++i)
    coordinates[i] = this->Coordinates[(n * this->GetDimensions()) + i];
}

template<typename T>
vtkArray* vtkSparseArray<T>::DeepCopy()
{
  // Provide the strong exception guarantee when allocating memory
  vtkArrayExtents new_extents = this->Extents;
  vtkstd::vector<vtkStdString> new_dimension_labels = this->DimensionLabels;
  vtkstd::vector<vtkIdType> new_coordinates = this->Coordinates;
  T* new_value_begin = new T[this->GetNonNullSize()];
  T* new_value_end = new_value_begin + this->GetNonNullSize();
  T* new_value_reserve = new_value_end;
  T new_null_value = this->NullValue;

  vtkstd::copy(this->ValueBegin, this->ValueEnd, new_value_begin);

  ThisT* const copy = ThisT::New();
  vtkstd::swap(copy->Extents, new_extents);
  vtkstd::swap(copy->DimensionLabels, new_dimension_labels);
  vtkstd::swap(copy->Coordinates, new_coordinates);
  vtkstd::swap(copy->ValueBegin, new_value_begin);
  vtkstd::swap(copy->ValueEnd, new_value_end);
  vtkstd::swap(copy->ValueReserve, new_value_reserve);
  vtkstd::swap(copy->NullValue, new_null_value);

  return copy;
}

template<typename T>
const T& vtkSparseArray<T>::GetValue(const vtkArrayCoordinates& coordinates)
{
  if(coordinates.GetDimensions() != this->GetDimensions())
    {
    vtkErrorMacro(<< "Index-array dimension mismatch.");
    return this->NullValue;
    }

  // Do a naive linear-search for the time-being ... 
  for(vtkIdType row = 0; row != static_cast<vtkIdType>(this->GetNonNullSize()); ++row)
    {
    for(vtkIdType column = 0; column != this->GetDimensions(); ++column)
      {
      if(coordinates[column] != this->Coordinates[(row * this->GetDimensions()) + column])
        break;

      if(column + 1 == this->GetDimensions())
        return this->ValueBegin[row];
      }
    }
  
  return this->NullValue;
}

template<typename T>
const T& vtkSparseArray<T>::GetValueN(const vtkIdType n)
{
  return this->ValueBegin[n];
}

template<typename T>
void vtkSparseArray<T>::SetValue(const vtkArrayCoordinates& coordinates, const T& value)
{
  if(coordinates.GetDimensions() != this->GetDimensions())
    {
    vtkErrorMacro(<< "Index-array dimension mismatch.");
    return;
    }

  // Do a naive linear-search for the time-being ... 
  for(vtkIdType row = 0; row != static_cast<vtkIdType>(this->GetNonNullSize()); ++row)
    {
    for(vtkIdType column = 0; column != this->GetDimensions(); ++column)
      {
      if(coordinates[column] != this->Coordinates[(row * this->GetDimensions()) + column])
        break;

      if(column + 1 == this->GetDimensions())
        {
        this->ValueBegin[row] = value;
        return;
        }
      }
    }

  // Element doesn't already exist, so add it to the end of the list ...
  this->AddValue(coordinates, value);
}

template<typename T>
void vtkSparseArray<T>::SetValueN(const vtkIdType n, const T& value)
{
  this->ValueBegin[n] = value;
}

template<typename T>
void vtkSparseArray<T>::SetNullValue(const T& value)
{
  this->NullValue = value;
}

template<typename T>
const T& vtkSparseArray<T>::GetNullValue()
{
  return this->NullValue;
}

template<typename T>
void vtkSparseArray<T>::Clear()
{
  this->Coordinates.clear();
  this->ValueEnd = this->ValueBegin;
}

template<typename T>
const vtkIdType* vtkSparseArray<T>::GetCoordinateStorage() const
{
  return &this->Coordinates[0];
}

template<typename T>
vtkIdType* vtkSparseArray<T>::GetCoordinateStorage()
{
  return &this->Coordinates[0];
}

template<typename T>
const T* vtkSparseArray<T>::GetValueStorage() const
{
  return this->ValueBegin;
}

template<typename T>
T* vtkSparseArray<T>::GetValueStorage()
{
  return this->ValueBegin;
}

template<typename T>
void vtkSparseArray<T>::ResizeToContents()
{
  vtkArrayExtents new_extents = this->Extents;

  vtkIdType row_begin = 0;
  vtkIdType row_end = row_begin + this->GetNonNullSize();
  for(vtkIdType row = row_begin; row != row_end; ++row)
    {
    for(vtkIdType column = 0; column != this->GetDimensions(); ++column)
      {
      new_extents[column] = vtkstd::max(new_extents[column], this->Coordinates[(row * this->GetDimensions()) + column] + 1);
      }
    }

  this->Extents = new_extents;
}

template<typename T>
void vtkSparseArray<T>::AddValue(vtkIdType i, const T& value)
{
  this->AddValue(vtkArrayCoordinates(i), value);
}

template<typename T>
void vtkSparseArray<T>::AddValue(vtkIdType i, vtkIdType j, const T& value)
{
  this->AddValue(vtkArrayCoordinates(i, j), value);
}

template<typename T>
void vtkSparseArray<T>::AddValue(vtkIdType i, vtkIdType j, vtkIdType k, const T& value)
{
  this->AddValue(vtkArrayCoordinates(i, j, k), value);
}

template<typename T>
void vtkSparseArray<T>::AddValue(const vtkArrayCoordinates& coordinates, const T& value)
{
  if(coordinates.GetDimensions() != this->GetDimensions())
    {
    vtkErrorMacro(<< "Index-array dimension mismatch.");
    return;
    }

  // Provide the strong exception guarantee when allocating memory
  this->Coordinates.reserve(this->Coordinates.size() + this->GetDimensions());

  if(this->ValueEnd == this->ValueReserve)
    {
    const vtkIdType current_size = this->ValueEnd - this->ValueBegin;
    const vtkIdType new_size = vtkstd::max(static_cast<vtkIdType>(16), current_size * 2);
    T* new_value_begin = new T[new_size];
    T* new_value_end = new_value_begin + current_size;
    T* new_value_reserve = new_value_begin + new_size;
    
    vtkstd::copy(this->ValueBegin, this->ValueEnd, new_value_begin);

    vtkstd::swap(this->ValueBegin, new_value_begin);
    vtkstd::swap(this->ValueEnd, new_value_end);
    vtkstd::swap(this->ValueReserve, new_value_reserve);

    delete[] new_value_begin;
    }

  *this->ValueEnd++ = value;

  for(vtkIdType i = 0; i != coordinates.GetDimensions(); ++i)
    this->Coordinates.push_back(coordinates[i]);
}

template<typename T>
vtkSparseArray<T>::vtkSparseArray() :
  ValueBegin(0),
  ValueEnd(0),
  ValueReserve(0),
  NullValue(T())
{
}

template<typename T>
vtkSparseArray<T>::~vtkSparseArray()
{
  delete[] this->ValueBegin;
}

template<typename T>
void vtkSparseArray<T>::InternalResize(const vtkArrayExtents& extents)
{
  this->Extents = extents;
  this->DimensionLabels.resize(extents.GetDimensions(), vtkStdString());
  this->ValueEnd = this->ValueBegin;
  this->Coordinates.resize(0);
}

template<typename T>
void vtkSparseArray<T>::InternalSetDimensionLabel(vtkIdType i, const vtkStdString& label)
{
  this->DimensionLabels[i] = label;
}

template<typename T>
vtkStdString vtkSparseArray<T>::InternalGetDimensionLabel(vtkIdType i)
{
  return this->DimensionLabels[i];
}

#endif

