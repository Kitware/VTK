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
  vtkDenseArray<T>* const copy = vtkDenseArray<T>::New();

  copy->Resize(this->Extents);
  copy->DimensionLabels = this->DimensionLabels;
  vtkstd::copy(this->Storage.begin(), this->Storage.end(), copy->Storage.begin());    

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

  return this->Storage[this->MapCoordinates(coordinates)];
}

template<typename T>
const T& vtkDenseArray<T>::GetValueN(const vtkIdType n)
{
  return this->Storage[n];
}

template<typename T>
void vtkDenseArray<T>::SetValue(const vtkArrayCoordinates& coordinates, const T& value)
{
  if(coordinates.GetDimensions() != this->GetDimensions())
    {
    vtkErrorMacro(<< "Index-array dimension mismatch.");
    return;
    }
 
  this->Storage[this->MapCoordinates(coordinates)] = value;
}

template<typename T>
void vtkDenseArray<T>::SetValueN(const vtkIdType n, const T& value)
{
  this->Storage[n] = value;
}

template<typename T>
void vtkDenseArray<T>::Fill(const T& value)
{
  vtkstd::fill(this->Storage.begin(), this->Storage.end(), value);
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
 
  return this->Storage[this->MapCoordinates(coordinates)];
}

template<typename T>
const T* vtkDenseArray<T>::GetStorage() const
{
  return &this->Storage[0];
}

template<typename T>
T* vtkDenseArray<T>::GetStorage()
{
  return &this->Storage[0];
}

template<typename T>
vtkDenseArray<T>::vtkDenseArray()
{
}

template<typename T>
vtkDenseArray<T>::~vtkDenseArray()
{
}

template<typename T>
void vtkDenseArray<T>::InternalResize(const vtkArrayExtents& extents)
{
  this->Extents = extents;
  this->DimensionLabels.resize(extents.GetDimensions(), vtkStdString());
  this->Storage.resize(this->Extents.GetSize());

  this->Strides.resize(this->GetDimensions());
  for(vtkIdType i = 0; i != this->GetDimensions(); ++i)
    {
    if(i == 0)
      this->Strides[i] = 1;
    else
      this->Strides[i] = this->Strides[i-1] * this->Extents[i-1];
    }
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

