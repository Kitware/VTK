/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataArrayHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericDataArrayHelper
// .SECTION Description

#ifndef vtkGenericDataArrayHelper_h
#define vtkGenericDataArrayHelper_h

#include "vtkObject.h"

class vtkAbstractArray;
class VTKCOMMONCORE_EXPORT vtkGenericDataArrayHelper : public vtkObject
{
public:
  vtkTypeMacro(vtkGenericDataArrayHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static void SetTuple(vtkAbstractArray* dest, vtkIdType destTuple,
    vtkAbstractArray* source, vtkIdType sourceTuple);

  static void GetTuple(vtkAbstractArray* source, vtkIdType tuple,
                       double* buffer);
//BTX
protected:
  vtkGenericDataArrayHelper();
  ~vtkGenericDataArrayHelper();

private:
  vtkGenericDataArrayHelper(const vtkGenericDataArrayHelper&); // Not implemented.
  void operator=(const vtkGenericDataArrayHelper&); // Not implemented.
//ETX
};

#endif
