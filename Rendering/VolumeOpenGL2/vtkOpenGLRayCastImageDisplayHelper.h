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

#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "vtkRayCastImageDisplayHelper.h"

class vtkFixedPointRayCastImage;
class vtkOpenGLHelper;
class vtkRenderer;
class vtkTextureObject;
class vtkVolume;
class vtkWindow;

class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkOpenGLRayCastImageDisplayHelper
  : public vtkRayCastImageDisplayHelper
{
public:
  static vtkOpenGLRayCastImageDisplayHelper *New();
  vtkTypeMacro(vtkOpenGLRayCastImageDisplayHelper,vtkRayCastImageDisplayHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                      int imageMemorySize[2],
                      int imageViewportSize[2],
                      int imageInUseSize[2],
                      int imageOrigin[2],
                      float requestedDepth,
                      unsigned char *image ) override;

  void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                      int imageMemorySize[2],
                      int imageViewportSize[2],
                      int imageInUseSize[2],
                      int imageOrigin[2],
                      float requestedDepth,
                      unsigned short *image ) override;

  void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                      vtkFixedPointRayCastImage *image,
                      float requestedDepth ) override;

  void ReleaseGraphicsResources(vtkWindow *win) override;

protected:
  vtkOpenGLRayCastImageDisplayHelper();
  ~vtkOpenGLRayCastImageDisplayHelper() override;

  void RenderTextureInternal( vtkVolume *vol, vtkRenderer *ren,
                              int imageMemorySize[2],
                              int imageViewportSize[2],
                              int imageInUseSize[2],
                              int imageOrigin[2],
                              float requestedDepth,
                              int imageScalarType,
                              void *image );

  // used for copying to framebuffer
  vtkTextureObject *TextureObject;
  vtkOpenGLHelper *ShaderProgram;



private:
  vtkOpenGLRayCastImageDisplayHelper(const vtkOpenGLRayCastImageDisplayHelper&) = delete;
  void operator=(const vtkOpenGLRayCastImageDisplayHelper&) = delete;
};

#endif
