/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAgnosticArrayHelpers.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAgnosticArrayHelpers
// .SECTION Description

#ifndef vtkAgnosticArrayHelpers_h
#define vtkAgnosticArrayHelpers_h

#include "vtkObject.h"

class vtkAbstractArray;
class VTKCOMMONCORE_EXPORT vtkAgnosticArrayHelpers : public vtkObject
{
public:
  vtkTypeMacro(vtkAgnosticArrayHelpers, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static void SetTuple(vtkAbstractArray* dest, vtkIdType destTuple,
    vtkAbstractArray* source, vtkIdType sourceTuple);

  static void GetTuple(vtkAbstractArray* source, vtkIdType tuple, double* buffer);
//BTX
protected:
  vtkAgnosticArrayHelpers();
  ~vtkAgnosticArrayHelpers();

private:
  vtkAgnosticArrayHelpers(const vtkAgnosticArrayHelpers&); // Not implemented.
  void operator=(const vtkAgnosticArrayHelpers&); // Not implemented.
//ETX
};

#endif
