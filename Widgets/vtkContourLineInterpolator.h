/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME 
// .SECTION Description
// 
//
// .SECTION See Also



#ifndef __vtkContourLineInterpolator_h
#define __vtkContourLineInterpolator_h

#include "vtkObject.h"

class vtkRenderer;
class vtkContourRepresentation;

class VTK_WIDGETS_EXPORT vtkContourLineInterpolator : public vtkObject
{
public:
  // Description:
  // Standard methods for instances of this class.
  vtkTypeRevisionMacro(vtkContourLineInterpolator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual int InterpolateLine( vtkRenderer *ren, 
                               vtkContourRepresentation *rep,
                               int idx1, int idx2 ) = 0;
  
protected:
  vtkContourLineInterpolator();
  ~vtkContourLineInterpolator();

private:
  vtkContourLineInterpolator(const vtkContourLineInterpolator&);  //Not implemented
  void operator=(const vtkContourLineInterpolator&);  //Not implemented
};

#endif
