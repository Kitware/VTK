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

#include "vtkParallelRenderManager.h"
#include "vtkRenderingParallelModule.h" // For export macro

class VTKRENDERINGPARALLEL_EXPORT vtkImageRenderManager : public vtkParallelRenderManager
{
public:
  vtkTypeMacro(vtkImageRenderManager, vtkParallelRenderManager);
  static vtkImageRenderManager* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageRenderManager();
  ~vtkImageRenderManager() override;

  void PreRenderProcessing() override;
  void PostRenderProcessing() override;

private:
  vtkImageRenderManager(const vtkImageRenderManager&) = delete;
  void operator=(const vtkImageRenderManager&) = delete;
};

#endif // vtkImageRenderManager_h
