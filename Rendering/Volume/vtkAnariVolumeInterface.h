/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnariVolumeInterface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAnariVolumeInterface
 * @brief   Removes link dependence on optional ANARI module.
 *
 * Class allows SmartVolume to use ANARI for rendering when ANARI
 * is enabled. When disabled, this class just returns a warning.
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariVolumeInterface_h
#define vtkAnariVolumeInterface_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkVolumeMapper.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkRenderer;
class vtkVolume;

class VTKRENDERINGVOLUME_EXPORT vtkAnariVolumeInterface : public vtkVolumeMapper
{
public:
  static vtkAnariVolumeInterface* New();
  vtkTypeMacro(vtkAnariVolumeInterface, vtkVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to warn about lack of ANARI if not overridden.
   */
  void Render(vtkRenderer*, vtkVolume*) override;

protected:
  vtkAnariVolumeInterface() = default;
  ~vtkAnariVolumeInterface() override = default;

private:
  vtkAnariVolumeInterface(const vtkAnariVolumeInterface&) = delete;
  void operator=(const vtkAnariVolumeInterface&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
