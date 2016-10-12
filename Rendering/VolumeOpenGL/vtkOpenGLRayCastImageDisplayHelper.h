/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRayCastImageDisplayHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOpenGLRayCastImageDisplayHelper
 * @brief   OpenGL subclass that draws the image to the screen
 *
 * This is the concrete implementation of a ray cast image display helper -
 * a helper class responsible for drawing the image to the screen.
 *
 * @sa
 * vtkRayCastImageDisplayHelper
*/

#ifndef vtkOpenGLRayCastImageDisplayHelper_h
#define vtkOpenGLRayCastImageDisplayHelper_h

#include "vtkRenderingVolumeOpenGLModule.h" // For export macro
#include "vtkRayCastImageDisplayHelper.h"

class vtkVolume;
class vtkRenderer;
class vtkFixedPointRayCastImage;

class VTKRENDERINGVOLUMEOPENGL_EXPORT vtkOpenGLRayCastImageDisplayHelper
  : public vtkRayCastImageDisplayHelper
{
public:
  static vtkOpenGLRayCastImageDisplayHelper *New();
  vtkTypeMacro(vtkOpenGLRayCastImageDisplayHelper,vtkRayCastImageDisplayHelper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                      int imageMemorySize[2],
                      int imageViewportSize[2],
                      int imageInUseSize[2],
                      int imageOrigin[2],
                      float requestedDepth,
                      unsigned char *image );

  void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                      int imageMemorySize[2],
                      int imageViewportSize[2],
                      int imageInUseSize[2],
                      int imageOrigin[2],
                      float requestedDepth,
                      unsigned short *image );

  void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                      vtkFixedPointRayCastImage *image,
                      float requestedDepth );

protected:
  vtkOpenGLRayCastImageDisplayHelper();
  ~vtkOpenGLRayCastImageDisplayHelper();

  void RenderTextureInternal( vtkVolume *vol, vtkRenderer *ren,
                              int imageMemorySize[2],
                              int imageViewportSize[2],
                              int imageInUseSize[2],
                              int imageOrigin[2],
                              float requestedDepth,
                              int imageScalarType,
                              void *image );

private:
  vtkOpenGLRayCastImageDisplayHelper(const vtkOpenGLRayCastImageDisplayHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLRayCastImageDisplayHelper&) VTK_DELETE_FUNCTION;
};

#endif
