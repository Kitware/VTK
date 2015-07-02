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
#define vtkGenericDataArrayMacroCase(arrayT, scalarT, array, call) \
  if (typeid(*array) == typeid(arrayT<scalarT>)) \
    { \
    typedef arrayT<scalarT> ARRAY_TYPE; \
    ARRAY_TYPE* ARRAY = reinterpret_cast<ARRAY_TYPE*>(array); \
    call; \
    }

#define vtkWriteableGenericDataArrayMacro(array, call) \
  vtkGenericDataArrayMacroCase(vtkSoADataArrayTemplate, float, array, call) \
  else vtkGenericDataArrayMacroCase(vtkSoADataArrayTemplate, double, array, call) \
  else vtkGenericDataArrayMacroCase(vtkAoSDataArrayTemplate, float, array, call) \
  else vtkGenericDataArrayMacroCase(vtkAoSDataArrayTemplate, double, array, call) \
  else \
    { \
    vtkGenericWarningMacro("Unknown type " << typeid(*array).name()); \
    abort(); \
    }

#define vtkConstGenericDataArrayMacro(array, call) \
  vtkGenericDataArrayMacroCase(vtkSoADataArrayTemplate, const float, array, call) \
  else vtkGenericDataArrayMacroCase(vtkSoADataArrayTemplate, const double, array, call) \
  else vtkGenericDataArrayMacroCase(vtkAoSDataArrayTemplate, const float, array, call) \
  else vtkGenericDataArrayMacroCase(vtkAoSDataArrayTemplate, const double, array, call) \
  else vtkWriteableGenericDataArrayMacro(array, call)

#define vtkWriteableGenericDataArrayMacro2(array1, array2, call) \
  vtkWriteableGenericDataArrayMacro(array1, \
    typedef ARRAY_TYPE ARRAY_TYPE1; \
    ARRAY_TYPE1* ARRAY1 = ARRAY; \
    vtkWriteableGenericDataArrayMacro(array2, \
      typedef ARRAY_TYPE2 ARRAY_TYPE; \
      ARRAY_TYPE2* ARRAY2 = ARRAY; \
      call; \
    )\
  )

#define vtkGenericDataArrayMacro2(inarray, outarray, call) \
  vtkConstGenericDataArrayMacro(inarray, \
    typedef ARRAY_TYPE IN_ARRAY_TYPE; \
    IN_ARRAY_TYPE* IN_ARRAY = ARRAY; \
    vtkWriteableGenericDataArrayMacro(outarray, \
      typedef ARRAY_TYPE OUT_ARRAY_TYPE; \
      OUT_ARRAY_TYPE* OUT_ARRAY = ARRAY; \
      call; \
    ) \
  )
#endif
