/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTypedArray.txx
  
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

template<typename T>
void vtkTypedArray<T>::PrintSelf(ostream &os, vtkIndent indent)
{
  this->vtkTypedArray<T>::Superclass::PrintSelf(os, indent);
}

template<typename T>
const T& vtkTypedArray<T>::GetValue(vtkIdType i)
{
  return this->GetValue(vtkArrayCoordinates(i));
}

template<typename T>
const T& vtkTypedArray<T>::GetValue(vtkIdType i, vtkIdType j)
{
  return this->GetValue(vtkArrayCoordinates(i, j));
}

template<typename T>
const T& vtkTypedArray<T>::GetValue(vtkIdType i, vtkIdType j, vtkIdType k)
{
  return this->GetValue(vtkArrayCoordinates(i, j, k));
}

template<typename T>
void vtkTypedArray<T>::SetValue(vtkIdType i, const T& value)
{
  this->SetValue(vtkArrayCoordinates(i), value);
}

template<typename T>
void vtkTypedArray<T>::SetValue(vtkIdType i, vtkIdType j, const T& value)
{
  this->SetValue(vtkArrayCoordinates(i, j), value);
}

template<typename T>
void vtkTypedArray<T>::SetValue(vtkIdType i, vtkIdType j, vtkIdType k, const T& value)
{
  this->SetValue(vtkArrayCoordinates(i, j, k), value);
}

