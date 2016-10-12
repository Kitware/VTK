/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReferenceCount.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkReferenceCount
 * @brief   Obsolete / empty subclass of object.
 *
 * vtkReferenceCount functionality has now been moved into vtkObject
 * @sa
 * vtkObject
*/

#ifndef vtkReferenceCount_h
#define vtkReferenceCount_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONCORE_EXPORT vtkReferenceCount : public vtkObject
{
public:
  static vtkReferenceCount *New();

  vtkTypeMacro(vtkReferenceCount,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkReferenceCount();
  ~vtkReferenceCount() VTK_OVERRIDE;

private:
  vtkReferenceCount(const vtkReferenceCount&) VTK_DELETE_FUNCTION;
  void operator=(const vtkReferenceCount&) VTK_DELETE_FUNCTION;
};


#endif

