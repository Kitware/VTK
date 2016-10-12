/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeTextureMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLVolumeTextureMapper2D
 * @brief   Abstract class for a volume mapper
 *
 *
 * vtkOpenGLVolumeTextureMapper2D renders a volume using 2D texture mapping.
 *
 * @sa
 * vtkVolumeMapper
 * @deprecated
*/

#ifndef vtkOpenGLVolumeTextureMapper2D_h
#define vtkOpenGLVolumeTextureMapper2D_h

#include "vtkRenderingVolumeOpenGLModule.h" // For export macro
#include "vtkVolumeTextureMapper2D.h"

#if !defined(VTK_LEGACY_REMOVE)
class VTKRENDERINGVOLUMEOPENGL_EXPORT vtkOpenGLVolumeTextureMapper2D
  : public vtkVolumeTextureMapper2D
{
public:
  vtkTypeMacro(vtkOpenGLVolumeTextureMapper2D,vtkVolumeTextureMapper2D);
  void PrintSelf( ostream& os, vtkIndent indent );

  static vtkOpenGLVolumeTextureMapper2D *New();

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Render the volume
   */
  virtual void Render(vtkRenderer *ren, vtkVolume *vol);

  void RenderQuads( int count, float *v, float *t,
                    unsigned char *texture, int size[2], int reverseFlag);

protected:
  vtkOpenGLVolumeTextureMapper2D();
  ~vtkOpenGLVolumeTextureMapper2D();

private:
  vtkOpenGLVolumeTextureMapper2D(const vtkOpenGLVolumeTextureMapper2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLVolumeTextureMapper2D&) VTK_DELETE_FUNCTION;
};
#endif // VTK_LEGACY_REMOVE
#endif
