/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRayCastImageDisplayHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkRayCastImageDisplayHelper
 * @brief   helper class that draws the image to the screen
 *
 * This is a helper class for drawing images created from ray casting on the screen.
 * This is the abstract device-independent superclass.
 *
 * @sa
 * vtkVolumeRayCastMapper vtkUnstructuredGridVolumeRayCastMapper
 * vtkOpenGLRayCastImageDisplayHelper
*/

#ifndef vtkRayCastImageDisplayHelper_h
#define vtkRayCastImageDisplayHelper_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkObject.h"

class vtkFixedPointRayCastImage;
class vtkRenderer;
class vtkVolume;
class vtkWindow;

class VTKRENDERINGVOLUME_EXPORT vtkRayCastImageDisplayHelper : public vtkObject
{
public:
  static vtkRayCastImageDisplayHelper *New();
  vtkTypeMacro(vtkRayCastImageDisplayHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                              int imageMemorySize[2],
                              int imageViewportSize[2],
                              int imageInUseSize[2],
                              int imageOrigin[2],
                              float requestedDepth,
                              unsigned char *image ) = 0;

  virtual void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                              int imageMemorySize[2],
                              int imageViewportSize[2],
                              int imageInUseSize[2],
                              int imageOrigin[2],
                              float requestedDepth,
                              unsigned short *image ) = 0;

  virtual void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                              vtkFixedPointRayCastImage *image,
                              float requestedDepth ) = 0;

  vtkSetClampMacro( PreMultipliedColors, int, 0, 1 );
  vtkGetMacro( PreMultipliedColors, int );
  vtkBooleanMacro( PreMultipliedColors, int );


  //@{
  /**
   * Set / Get the pixel scale to be applied to the image before display.
   * Can be set to scale the incoming pixel values - for example the
   * fixed point mapper uses the unsigned short API but with 15 bit
   * values so needs a scale of 2.0.
   */
  vtkSetMacro( PixelScale, float );
  vtkGetMacro( PixelScale, float );
  //@}

  /**
   * Derived class should implemen this if needed
   */
  virtual void ReleaseGraphicsResources(vtkWindow *) { }

protected:
  vtkRayCastImageDisplayHelper();
  ~vtkRayCastImageDisplayHelper();

  /**
   * Have the colors already been multiplied by alpha?
   */
  int PreMultipliedColors;

  float PixelScale;

private:
  vtkRayCastImageDisplayHelper(const vtkRayCastImageDisplayHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRayCastImageDisplayHelper&) VTK_DELETE_FUNCTION;
};

#endif

