/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayVolumeMapper.h
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen

  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayVolumeMapper
 * @brief   Standalone OSPRayVolumeMapper.
 *
 * This is a standalone interface for ospray volume rendering to be used
 * within otherwise OpenGL rendering contexts such as within the
 * SmartVolumeMapper.
*/

#ifndef vtkOSPRayVolumeMapper_h
#define vtkOSPRayVolumeMapper_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkOSPRayVolumeInterface.h"

class vtkOSPRayPass;
class vtkRenderer;
class vtkWindow;

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayVolumeMapper
  : public vtkOSPRayVolumeInterface
{
public:
  static vtkOSPRayVolumeMapper *New();
  vtkTypeMacro(vtkOSPRayVolumeMapper,vtkOSPRayVolumeInterface);
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

  // Initialize internal constructs
  virtual void Init();

  /**
   * Render the volume onto the screen.
   * Overridden to use OSPRay to do the work.
   */
  virtual void Render(vtkRenderer *, vtkVolume *) VTK_OVERRIDE;

protected:
  vtkOSPRayVolumeMapper();
  ~vtkOSPRayVolumeMapper();

  vtkOSPRayPass *InternalOSPRayPass;
  vtkRenderer *InternalRenderer;
  bool Initialized;

private:
  vtkOSPRayVolumeMapper(const vtkOSPRayVolumeMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOSPRayVolumeMapper&) VTK_DELETE_FUNCTION;
};

#endif
