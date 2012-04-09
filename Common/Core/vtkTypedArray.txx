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

#include "vtkVariantCast.h"
#include "vtkVariantCreate.h"

template<typename T>
void vtkTypedArray<T>::PrintSelf(ostream &os, vtkIndent indent)
{
  this->vtkTypedArray<T>::Superclass::PrintSelf(os, indent);
}

template<typename T>
vtkVariant vtkTypedArray<T>::GetVariantValue(const vtkArrayCoordinates& coordinates)
{
  return vtkVariantCreate<T>(this->GetValue(coordinates));
}

template<typename T>
vtkVariant vtkTypedArray<T>::GetVariantValueN(const SizeT n)
{
  return vtkVariantCreate<T>(this->GetValueN(n));
}

template<typename T>
void vtkTypedArray<T>::SetVariantValue(const vtkArrayCoordinates& coordinates, const vtkVariant& value)
{
  this->SetValue(coordinates, vtkVariantCast<T>(value));
}

template<typename T>
void vtkTypedArray<T>::SetVariantValueN(const SizeT n, const vtkVariant& value)
{
  this->SetValueN(n, vtkVariantCast<T>(value));
}

template<typename T>
void vtkTypedArray<T>::CopyValue(vtkArray* source, const vtkArrayCoordinates& source_coordinates, const vtkArrayCoordinates& target_coordinates)
{
  if(!source->IsA(this->GetClassName()))
    {
    vtkWarningMacro("source and target array data types do not match");
    return;
    }

  this->SetValue(target_coordinates, static_cast<vtkTypedArray<T>*>(source)->GetValue(source_coordinates));
}

template<typename T>
void vtkTypedArray<T>::CopyValue(vtkArray* source, const SizeT source_index, const vtkArrayCoordinates& target_coordinates)
{
  if(!source->IsA(this->GetClassName()))
    {
    vtkWarningMacro("source and target array data types do not match");
    return;
    }

  this->SetValue(target_coordinates, static_cast<vtkTypedArray<T>*>(source)->GetValueN(source_index));
}

template<typename T>
void vtkTypedArray<T>::CopyValue(vtkArray* source, const vtkArrayCoordinates& source_coordinates, const SizeT target_index)
{
  if(!source->IsA(this->GetClassName()))
    {
    vtkWarningMacro("source and target array data types do not match");
    return;
    }

  this->SetValueN(target_index, static_cast<vtkTypedArray<T>*>(source)->GetValue(source_coordinates));
}
