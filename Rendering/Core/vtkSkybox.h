/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSkybox.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSkybox
 * @brief Renders a skybox environment
 *
 * You must provide a texture cube map using the SetTexture method.
 */

#ifndef vtkSkybox_h
#define vtkSkybox_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkActor.h"

class VTKRENDERINGCORE_EXPORT vtkSkybox: public vtkActor
{
public:
  static vtkSkybox* New();
  vtkTypeMacro(vtkSkybox, vtkActor)
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax). (The
   * method GetBounds(double bounds[6]) is available from the superclass.)
   */
  using Superclass::GetBounds;
  double *GetBounds() VTK_OVERRIDE;

protected:
  vtkSkybox();
  ~vtkSkybox() VTK_OVERRIDE;

private:
  vtkSkybox(const vtkSkybox&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSkybox&) VTK_DELETE_FUNCTION;
};

#endif // vtkSkybox_h
