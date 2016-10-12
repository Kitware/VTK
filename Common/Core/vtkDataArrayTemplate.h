/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataArrayTemplate
 *
 * vtkDataArrayTemplate is deprecated, use vtkAOSDataArrayTemplate instead.
*/

#ifndef vtkDataArrayTemplate_h
#define vtkDataArrayTemplate_h

#include "vtkAOSDataArrayTemplate.h"

#ifndef VTK_LEGACY_REMOVE

template <typename ValueType>
class vtkDataArrayTemplate : public vtkAOSDataArrayTemplate<ValueType>
{
public:
  vtkTemplateTypeMacro(vtkDataArrayTemplate<ValueType>,
                       vtkAOSDataArrayTemplate<ValueType>)

  static vtkDataArrayTemplate<ValueType>* New()
  {
    VTK_STANDARD_NEW_BODY(vtkDataArrayTemplate<ValueType>);
  }

protected:
  vtkDataArrayTemplate() {}
  ~vtkDataArrayTemplate() {}

private:
  vtkDataArrayTemplate(const vtkDataArrayTemplate&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataArrayTemplate&) VTK_DELETE_FUNCTION;
};

#endif // VTK_LEGACY_REMOVE

#endif // vtkDataArrayTemplate_h

// VTK-HeaderTest-Exclude: vtkDataArrayTemplate.h
