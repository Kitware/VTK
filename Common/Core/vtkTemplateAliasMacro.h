/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemplateAliasMacro.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTemplateAliasMacro
 * @brief   Dispatch a scalar processing template.
 *
 * vtkTemplateAliasMacro is used in a switch statement to
 * automatically generate duplicate code for all enabled scalar types.
 * The code can be written to use VTK_TT to refer to the type, and
 * each case generated will define VTK_TT appropriately.  The
 * difference between this and the standard vtkTemplateMacro is that
 * this version will set VTK_TT to an "alias" for each type.  The
 * alias may be the same type or may be a different type that is the
 * same size/signedness.  This is sufficient when only the numerical
 * value associated with instances of the type is needed, and it
 * avoids unnecessary template instantiations.
 *
 * Example usage:
 *
 *   void* p = dataArray->GetVoidPointer(0);
 *   switch(dataArray->GetDataType())
 *     {
 *     vtkTemplateAliasMacro(vtkMyTemplateFunction(static_cast<VTK_TT*>(p)));
 *     }
*/

#ifndef vtkTemplateAliasMacro_h
#define vtkTemplateAliasMacro_h

#include "vtkTypeTraits.h"

// Allow individual switching of support for each scalar size/signedness.
// These could be made advanced user options to be configured by CMake.
#define VTK_USE_INT8 1
#define VTK_USE_UINT8 1
#define VTK_USE_INT16 1
#define VTK_USE_UINT16 1
#define VTK_USE_INT32 1
#define VTK_USE_UINT32 1
#define VTK_USE_INT64 1
#define VTK_USE_UINT64 1
#define VTK_USE_FLOAT32 1
#define VTK_USE_FLOAT64 1

//--------------------------------------------------------------------------

// Define helper macros to switch types on and off.
#define vtkTemplateAliasMacroCase(typeN, call)                                \
  vtkTemplateAliasMacroCase0(typeN, call, VTK_TYPE_SIZED_##typeN)
#define vtkTemplateAliasMacroCase0(typeN, call, sized)                        \
  vtkTemplateAliasMacroCase1(typeN, call, sized)
#define vtkTemplateAliasMacroCase1(typeN, call, sized)                        \
  vtkTemplateAliasMacroCase2(typeN, call, VTK_USE_##sized)
#define vtkTemplateAliasMacroCase2(typeN, call, value)                        \
  vtkTemplateAliasMacroCase3(typeN, call, value)
#define vtkTemplateAliasMacroCase3(typeN, call, value)                        \
  vtkTemplateAliasMacroCase_##value(typeN, call)
#define vtkTemplateAliasMacroCase_0(typeN, call)                              \
  case VTK_##typeN:                                                           \
  {                                                                         \
    vtkGenericWarningMacro("Support for VTK_" #typeN " not compiled.");       \
  }; break
#define vtkTemplateAliasMacroCase_1(typeN, call)                              \
  case VTK_##typeN:                                                           \
  {                                                                         \
    typedef vtkTypeTraits<VTK_TYPE_NAME_##typeN>::SizedType VTK_TT; call;     \
  }; break

// Define a macro to dispatch calls to a template instantiated over
// the aliased scalar types.
#define vtkTemplateAliasMacro(call)                                           \
  vtkTemplateAliasMacroCase(DOUBLE, call);                                    \
  vtkTemplateAliasMacroCase(FLOAT, call);                                     \
  vtkTemplateAliasMacroCase(LONG_LONG, call);                                 \
  vtkTemplateAliasMacroCase(UNSIGNED_LONG_LONG, call);                        \
  vtkTemplateAliasMacroCase(ID_TYPE, call);                                   \
  vtkTemplateAliasMacroCase(LONG, call);                                      \
  vtkTemplateAliasMacroCase(UNSIGNED_LONG, call);                             \
  vtkTemplateAliasMacroCase(INT, call);                                       \
  vtkTemplateAliasMacroCase(UNSIGNED_INT, call);                              \
  vtkTemplateAliasMacroCase(SHORT, call);                                     \
  vtkTemplateAliasMacroCase(UNSIGNED_SHORT, call);                            \
  vtkTemplateAliasMacroCase(CHAR, call);                                      \
  vtkTemplateAliasMacroCase(SIGNED_CHAR, call);                               \
  vtkTemplateAliasMacroCase(UNSIGNED_CHAR, call)

#endif
// VTK-HeaderTest-Exclude: vtkTemplateAliasMacro.h
