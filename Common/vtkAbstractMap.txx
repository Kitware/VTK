/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractMap.txx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Include blockers needed since vtkVector.h includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.
#ifndef __vtkAbstractMap_txx
#define __vtkAbstractMap_txx

#include "vtkAbstractMap.h"

template<class KeyType, class DataType>
vtkAbstractMap<KeyType,DataType>::vtkAbstractMap() {}


#endif
