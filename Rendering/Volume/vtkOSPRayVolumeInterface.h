/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayVolumeInterface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayVolumeInterface
 * @brief   Removes link dependence
 * on optional ospray module.
 *
 * Class allows SmartVolume to use OSPRay for rendering when ospray
 * is enabled. When disabled, this class does nothing but return a warning.
*/

#ifndef vtkOSPRayVolumeInterface_h
#define vtkOSPRayVolumeInterface_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkVolumeMapper.h"

class vtkRenderer;
class vtkVolume;

class VTKRENDERINGVOLUME_EXPORT vtkOSPRayVolumeInterface
: public vtkVolumeMapper
{
public:
  static vtkOSPRayVolumeInterface *New();
  vtkTypeMacro(vtkOSPRayVolumeInterface,vtkVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;

  /**
   * Overridden to warn about lack of OSPRay if not overridden.
   */
  void Render(vtkRenderer *, vtkVolume *) VTK_OVERRIDE;

protected:
  vtkOSPRayVolumeInterface();
  ~vtkOSPRayVolumeInterface() VTK_OVERRIDE;

private:
  vtkOSPRayVolumeInterface
    (const vtkOSPRayVolumeInterface&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOSPRayVolumeInterface&) VTK_DELETE_FUNCTION;
};

#endif
