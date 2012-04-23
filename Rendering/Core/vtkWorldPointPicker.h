/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWorldPointPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWorldPointPicker - find world x,y,z corresponding to display x,y,z
// .SECTION Description
// vtkWorldPointPicker is used to find the x,y,z world coordinate of a
// screen x,y,z. This picker cannot pick actors and/or mappers, it
// simply determines an x-y-z coordinate in world space. (It will always
// return a x-y-z, even if the selection point is not over a prop/actor.)

// .SECTION Caveats
// The PickMethod() is not invoked, but StartPickMethod() and EndPickMethod()
// are.

// .SECTION See Also
// vtkPropPicker vtkPicker vtkCellPicker vtkPointPicker

#ifndef __vtkWorldPointPicker_h
#define __vtkWorldPointPicker_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkAbstractPicker.h"

class VTKRENDERINGCORE_EXPORT vtkWorldPointPicker : public vtkAbstractPicker
{
public:
  static vtkWorldPointPicker *New();
  vtkTypeMacro(vtkWorldPointPicker,vtkAbstractPicker);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform the pick. (This method overload's the superclass.)
  int Pick(double selectionX, double selectionY, double selectionZ,
           vtkRenderer *renderer);
  int Pick(double selectionPt[3], vtkRenderer *renderer)
    { return this->vtkAbstractPicker::Pick( selectionPt, renderer); };

protected:
  vtkWorldPointPicker ();
  ~vtkWorldPointPicker() {};

private:
  vtkWorldPointPicker(const vtkWorldPointPicker&);  // Not implemented.
  void operator=(const vtkWorldPointPicker&);  // Not implemented.
};

#endif


