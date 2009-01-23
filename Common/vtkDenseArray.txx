/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDenseArray.txx
  
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

#ifndef __vtkDenseArray_txx
#define __vtkDenseArray_txx

template<typename T>
vtkDenseArray<T>* vtkDenseArray<T>::New()
{
  vtkObject* ret = vtkObjectFactory::CreateInstance(typeid(ThisT).name());
  if(ret)
    {
    return static_cast<ThisT*>(ret);
    }
  return new ThisT();
}

template<typename T>
void vtkDenseArray<T>::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDenseArray<T>::Superclass::PrintSelf(os, indent);
}

template<typename T>
vtkArrayExtents vtkDenseArray<T>::GetExtents()
{
  return this->Extents;
}

template<typename T>
vtkIdType vtkDenseArray<T>::GetNonNullSize()
{
  return this->Extents.GetSize();
}

template<typename T>
void vtkDenseArray<T>::GetCoordinatesN(const vtkIdType n, vtkArrayCoordinates& coordinates)
{
  coordinates.SetDimensions(this->GetDimensions());

  vtkIdType divisor = 1;
  for(vtkIdType i = 0; i < this->GetDimensions(); ++i)
    {
    coordinates[i] = ((n / divisor) % this->Extents[i]);
    divisor *= this->Extents[i];
    }
}

template<typename T>
vtkArray* vtkDenseArray<T>::DeepCopy()
{
  // Provide the strong exception guarantee when allocating memory
  vtkArrayExtents new_extents = this->Extents;
  vtkstd::vector<vtkStdString> new_dimension_labels = this->DimensionLabels;
  T* new_value_begin = new T[new_extents.GetSize()];
  T* new_value_end = new_value_begin + new_extents.GetSize();
  vtkstd::vector<vtkIdType> new_strides = this->Strides;

  vtkstd::copy(this->ValueBegin, this->ValueEnd, new_value_begin);
  
  vtkDenseArray<T>* const copy = vtkDenseArray<T>::New();
  vtkstd::swap(copy->Extents, new_extents);
  vtkstd::swap(copy->DimensionLabels, new_dimension_labels);
  vtkstd::swap(copy->ValueBegin, new_value_begin);
  vtkstd::swap(copy->ValueEnd, new_value_end);
  vtkstd::swap(copy->Strides, new_strides);

  // We don't need to delete[] new_value_begin here, since it is NULL

  return copy;
}

template<typename T>
const T& vtkDenseArray<T>::GetValue(const vtkArrayCoordinates& coordinates)
{
  if(coordinates.GetDimensions() != this->GetDimensions())
    {
    vtkErrorMacro(<< "Index-array dimension mismatch.");
    static T temp;
    return temp;
    }

  return this->ValueBegin[this->MapCoordinates(coordinates)];
}

template<typename T>
const T& vtkDenseArray<T>::GetValueN(const vtkIdType n)
{
  return this->ValueBegin[n];
}

template<typename T>
void vtkDenseArray<T>::SetValue(const vtkArrayCoordinates& coordinates, const T& value)
{
  if(coordinates.GetDimensions() != this->GetDimensions())
    {
    vtkErrorMacro(<< "Index-array dimension mismatch.");
    return;
    }
 
  this->ValueBegin[this->MapCoordinates(coordinates)] = value;
}

template<typename T>
void vtkDenseArray<T>::SetValueN(const vtkIdType n, const T& value)
{
  this->ValueBegin[n] = value;
}

template<typename T>
void vtkDenseArray<T>::Fill(const T& value)
{
  vtkstd::fill(this->ValueBegin, this->ValueEnd, value);
}

template<typename T>
T& vtkDenseArray<T>::operator[](const vtkArrayCoordinates& coordinates)
{
  if(coordinates.GetDimensions() != this->GetDimensions())
    {
    static T temp;
    vtkErrorMacro(<< "Index-array dimension mismatch.");
    return temp;
    }
 
  return this->ValueBegin[this->MapCoordinates(coordinates)];
}

template<typename T>
const T* vtkDenseArray<T>::GetStorage() const
{
  return this->ValueBegin;
}

template<typename T>
T* vtkDenseArray<T>::GetStorage()
{
  return this->ValueBegin;
}

template<typename T>
vtkDenseArray<T>::vtkDenseArray() :
  ValueBegin(0),
  ValueEnd(0)
{
}

template<typename T>
vtkDenseArray<T>::~vtkDenseArray()
{
  delete[] this->ValueBegin;
}

template<typename T>
void vtkDenseArray<T>::InternalResize(const vtkArrayExtents& extents)
{
  // Provide the strong exception guarantee when allocating memory
  vtkArrayExtents new_extents = extents;
  vtkstd::vector<vtkStdString> new_dimension_labels(new_extents.GetDimensions(), vtkStdString());
  T* new_value_begin = new T[new_extents.GetSize()];
  T* new_value_end = new_value_begin + new_extents.GetSize();
  vtkstd::vector<vtkIdType> new_strides(new_extents.GetDimensions());
  for(vtkIdType i = 0; i != new_extents.GetDimensions(); ++i)
    {
    if(i == 0)
      new_strides[i] = 1;
    else
      new_strides[i] = new_strides[i-1] * new_extents[i-1];
    }

  vtkstd::swap(this->Extents, new_extents);
  vtkstd::swap(this->DimensionLabels, new_dimension_labels);
  vtkstd::swap(this->ValueBegin, new_value_begin);
  vtkstd::swap(this->ValueEnd, new_value_end);
  vtkstd::swap(this->Strides, new_strides);

  delete[] new_value_begin;
}

template<typename T>
void vtkDenseArray<T>::InternalSetDimensionLabel(vtkIdType i, const vtkStdString& label)
{
  this->DimensionLabels[i] = label;
}

template<typename T>
vtkStdString vtkDenseArray<T>::InternalGetDimensionLabel(vtkIdType i)
{
  return this->DimensionLabels[i];
}

template<typename T>
vtkIdType vtkDenseArray<T>::MapCoordinates(const vtkArrayCoordinates& coordinates)
{
  vtkIdType index = 0;
  for(vtkIdType i = 0; i != static_cast<vtkIdType>(this->Strides.size()); ++i)
    index += (coordinates[i] * this->Strides[i]);
  return index;
}

#endif

