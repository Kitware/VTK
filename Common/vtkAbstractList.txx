/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractList.txx
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
#ifndef __vtkAbstractList_txx
#define __vtkAbstractList_txx

#include "vtkAbstractList.h"
#include "vtkDebugLeaks.h"

template<class DType>
vtkAbstractList<DType>* vtkAbstractList<DType>::New()
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkAbstractList");
#endif
  return new vtkAbstractList<DType>;
}

template<class DataType>
vtkAbstractList<DataType>::vtkAbstractList() {}

#if defined ( _MSC_VER )
template <class DType>
vtkAbstractList<DType>::vtkAbstractList(const vtkAbstractList<DType>&){}
template <class DType>
void vtkAbstractList<DType>::operator=(const vtkAbstractList<DType>&){}
#endif

#endif
