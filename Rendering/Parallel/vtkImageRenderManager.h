/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRenderManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageRenderManager
 * @brief   An object to control sort-first parallel rendering.
 *
 *
 * vtkImageRenderManager is a subclass of vtkParallelRenderManager that
 * uses RGBA compositing (blending) to do parallel rendering.
 * This is the exact opposite of vtkCompositeRenderManager.
 * It actually does nothing special. It relies on the rendering pipeline to be
 * initialized with a vtkCompositeRGBAPass.
 * Compositing makes sense only for renderers in layer 0.
 * @sa
 * vtkCompositeRGBAPass
*/

#ifndef vtkImageRenderManager_h
#define vtkImageRenderManager_h

#include "vtkRenderingParallelModule.h" // For export macro
#include "vtkParallelRenderManager.h"

class VTKRENDERINGPARALLEL_EXPORT vtkImageRenderManager : public vtkParallelRenderManager
{
public:
  vtkTypeMacro(vtkImageRenderManager, vtkParallelRenderManager);
  static vtkImageRenderManager *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkImageRenderManager();
  ~vtkImageRenderManager();

  virtual void PreRenderProcessing() VTK_OVERRIDE;
  virtual void PostRenderProcessing() VTK_OVERRIDE;

private:
  vtkImageRenderManager(const vtkImageRenderManager &) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageRenderManager &) VTK_DELETE_FUNCTION;
};

#endif //vtkImageRenderManager_h
