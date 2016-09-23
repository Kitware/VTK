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
/**
 * @class   vtkCuller
 * @brief   a superclass for prop cullers
 *
 * A culler has a cull method called by the vtkRenderer. The cull
 * method is called before any rendering is performed,
 * and it allows the culler to do some processing on the props and
 * to modify their AllocatedRenderTime and re-order them in the prop list.
 *
 * @sa
 * vtkFrustumCoverageCuller
*/

#ifndef vtkCuller_h
#define vtkCuller_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkProp;
class vtkRenderer;

class VTKRENDERINGCORE_EXPORT vtkCuller : public vtkObject
{
public:
  vtkTypeMacro(vtkCuller, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * This is called outside the render loop by vtkRenderer
   */
  virtual double Cull( vtkRenderer *ren, vtkProp **propList,
                       int& listLength, int& initialized ) = 0;

protected:
  vtkCuller();
  ~vtkCuller();

private:
  vtkCuller(const vtkCuller&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCuller&) VTK_DELETE_FUNCTION;
};

#endif
