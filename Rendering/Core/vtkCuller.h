/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCuller.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCuller - a superclass for prop cullers
// .SECTION Description
// A culler has a cull method called by the vtkRenderer. The cull 
// method is called before any rendering is performed,
// and it allows the culler to do some processing on the props and 
// to modify their AllocatedRenderTime and re-order them in the prop list. 

// .SECTION see also
// vtkFrustumCoverageCuller

#ifndef __vtkCuller_h
#define __vtkCuller_h

#include "vtkObject.h"

class vtkProp;
class vtkRenderer;

class VTK_RENDERING_EXPORT vtkCuller : public vtkObject
{
public:
  vtkTypeMacro(vtkCuller,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is called outside the render loop by vtkRenderer
  virtual double Cull( vtkRenderer *ren, vtkProp **propList,
                       int& listLength, int& initialized )=0;

protected:
  vtkCuller();
  ~vtkCuller();
private:
  vtkCuller(const vtkCuller&);  // Not implemented.
  void operator=(const vtkCuller&);    // Not implemented.
};
                                         
#endif
