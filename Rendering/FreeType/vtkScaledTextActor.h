/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScaledTextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkScaledTextActor - create text that will scale as needed
// .SECTION Description
// vtkScaledTextActor is deprecated. New code should use vtkTextActor with
// the Scaled = true option.
//
// .SECTION See Also
// vtkTextActor vtkActor2D vtkTextMapper

#ifndef __vtkScaledTextActor_h
#define __vtkScaledTextActor_h

#include "vtkRenderingFreeTypeModule.h" // For export macro
#include "vtkTextActor.h"

class VTKRENDERINGFREETYPE_EXPORT vtkScaledTextActor : public vtkTextActor
{
public:
  vtkTypeMacro(vtkScaledTextActor,vtkTextActor);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with a rectangle in normaled view coordinates
  // of (0.2,0.85, 0.8, 0.95).
  static vtkScaledTextActor *New();

protected:
   vtkScaledTextActor();
private:
  vtkScaledTextActor(const vtkScaledTextActor&);  // Not implemented.
  void operator=(const vtkScaledTextActor&);  // Not implemented.
};


#endif

