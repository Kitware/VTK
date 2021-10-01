/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVariantCast.cxx

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

// Hide VTK_DEPRECATED_IN_9_1_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkVariantCast.h"
#include "vtkUnicodeString.h"
#include "vtkVariantCreate.h"

template <>
vtkVariant vtkVariantCreate<vtkUnicodeString>(const vtkUnicodeString& value)
{
  return value;
}

template <>
vtkUnicodeString vtkVariantCast<vtkUnicodeString>(const vtkVariant& value, bool* valid)
{
  if (valid)
    *valid = true;

  return value.ToUnicodeString();
}
