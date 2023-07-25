// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkRayCastImageDisplayHelper
 * @brief   helper class that draws the image to the screen
 *
 * This is a helper class for drawing images created from ray casting on the screen.
 * This is the abstract device-independent superclass.
 *
 * @sa
 * vtkUnstructuredGridVolumeRayCastMapper
 * vtkOpenGLRayCastImageDisplayHelper
 */

#ifndef vtkRayCastImageDisplayHelper_h
#define vtkRayCastImageDisplayHelper_h

#include "vtkObject.h"
#include "vtkRenderingVolumeModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkFixedPointRayCastImage;
class vtkRenderer;
class vtkVolume;
class vtkWindow;

class VTKRENDERINGVOLUME_EXPORT vtkRayCastImageDisplayHelper : public vtkObject
{
public:
  static vtkRayCastImageDisplayHelper* New();
  vtkTypeMacro(vtkRayCastImageDisplayHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void RenderTexture(vtkVolume* vol, vtkRenderer* ren, int imageMemorySize[2],
    int imageViewportSize[2], int imageInUseSize[2], int imageOrigin[2], float requestedDepth,
    unsigned char* image) = 0;

  virtual void RenderTexture(vtkVolume* vol, vtkRenderer* ren, int imageMemorySize[2],
    int imageViewportSize[2], int imageInUseSize[2], int imageOrigin[2], float requestedDepth,
    unsigned short* image) = 0;

  virtual void RenderTexture(
    vtkVolume* vol, vtkRenderer* ren, vtkFixedPointRayCastImage* image, float requestedDepth) = 0;

  vtkSetClampMacro(PreMultipliedColors, vtkTypeBool, 0, 1);
  vtkGetMacro(PreMultipliedColors, vtkTypeBool);
  vtkBooleanMacro(PreMultipliedColors, vtkTypeBool);

  ///@{
  /**
   * Set / Get the pixel scale to be applied to the image before display.
   * Can be set to scale the incoming pixel values - for example the
   * fixed point mapper uses the unsigned short API but with 15 bit
   * values so needs a scale of 2.0.
   */
  vtkSetMacro(PixelScale, float);
  vtkGetMacro(PixelScale, float);
  ///@}

  /**
   * Derived class should implement this if needed
   */
  virtual void ReleaseGraphicsResources(vtkWindow*) {}

protected:
  vtkRayCastImageDisplayHelper();
  ~vtkRayCastImageDisplayHelper() override;

  /**
   * Have the colors already been multiplied by alpha?
   */
  vtkTypeBool PreMultipliedColors;

  float PixelScale;

private:
  vtkRayCastImageDisplayHelper(const vtkRayCastImageDisplayHelper&) = delete;
  void operator=(const vtkRayCastImageDisplayHelper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
