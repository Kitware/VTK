/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataArrayMacros.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericDataArrayMacros
// .SECTION Description

#ifndef vtkGenericDataArrayMacros_h
#define vtkGenericDataArrayMacros_h

#include "vtkGenericDataArray.h"
#include "vtkSoADataArrayTemplate.h"
#include "vtkAoSDataArrayTemplate.h"

#include <typeinfo>

// TODO the dynamic_cast is needed instead of:
// if (typeid(*array) == typeid(arrayT<scalarT>)) \
// so that e.g. vtkFloatArray will match vtkAoSDataArrayTemplate<float>
// Might be slower, need to find the more performant way to do this that
// covers all usecases. Then again...this should never be called in a tight
// loop, so dynamic_cast'ing might be fine?
#define vtkGenericDataArrayMacroCase(arrayT, scalarT, array, call) \
  if (dynamic_cast<arrayT<scalarT>*>(array)) \
    { \
    typedef arrayT<scalarT> ARRAY_TYPE; \
    ARRAY_TYPE* ARRAY = reinterpret_cast<ARRAY_TYPE*>(array); \
    call; \
    }

// vtkIdType is at the end just in case it's definied to a non-default type.
#define vtkGenericDataArrayMacroTemplateCases(arrayT, array, call) \
  vtkGenericDataArrayMacroCase(arrayT, char, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, double, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, float, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, int, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, long long, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, long, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, short, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, signed char, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, unsigned char, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, unsigned int, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, unsigned long long, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, unsigned long, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, unsigned short, array, call) \
  else vtkGenericDataArrayMacroCase(arrayT, vtkIdType, array, call)

#define vtkGenericDataArrayMacro(array, call) \
  vtkGenericDataArrayMacroTemplateCases(vtkSoADataArrayTemplate, array, call) \
  else vtkGenericDataArrayMacroTemplateCases(vtkAoSDataArrayTemplate, array, call) \
  else \
    { \
    vtkGenericWarningMacro("Unknown type " << typeid(*array).name()); \
    abort(); \
    }

#define vtkGenericDataArrayMacro2(array1, array2, call) \
  vtkGenericDataArrayMacro(array1, \
    typedef ARRAY_TYPE ARRAY_TYPE1; \
    ARRAY_TYPE1* ARRAY1 = ARRAY; \
    vtkGenericDataArrayMacro(array2, \
      typedef ARRAY_TYPE ARRAY_TYPE2; \
      ARRAY_TYPE2* ARRAY2 = ARRAY; \
      call; \
    )\
  )

#endif
