// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkResizingWindowToImageFilter
 * @brief   Use a vtkWindow as input to image pipeline
 *
 * vtkResizingWindowToImageFilter provides methods needed to read the data in
 * a vtkWindow and use it as input to the imaging pipeline. This is
 * useful for saving an image to a file for example. The window can
 * be read as either RGB or RGBA pixels;  in addition, the depth buffer
 * can also be read.   RGB and RGBA pixels are of type unsigned char,
 * while Z-Buffer data is returned as floats.  Use this filter
 * to convert RenderWindows or ImageWindows to an image format.
 *
 * @note
 * In contrast to the vtkWindowToImageFilter, this allows also for non-integral
 * values to be used as scaling factors for the generated image. Also, it
 * provides a SizeLimit parameter which enables to control when the algorithm
 * will switch to using tiling for generating a large image instead of
 * rendering the entire result at once.
 *
 * @warning
 * A vtkWindow doesn't behave like other parts of the VTK pipeline: its
 * modification time doesn't get updated when an image is rendered.  As a
 * result, naive use of vtkResizingWindowToImageFilter will produce an image of
 * the first image that the window rendered, but which is never updated
 * on subsequent window updates.  This behavior is unexpected and in
 * general undesirable.
 *
 * @warning
 * To force an update of the output image, call vtkResizingWindowToImageFilter's
 * Modified method after rendering to the window.
 *
 * @sa
 * vtkRendererSource vtkRendererPointCloudSource vtkWindow
 * vtkRenderLargeImage vtkWindowToImageFilter
 */

#ifndef vtkResizingWindowToImageFilter_h
#define vtkResizingWindowToImageFilter_h

#include "vtkAlgorithm.h"
#include "vtkImageData.h"           // makes things a bit easier
#include "vtkRenderingCoreModule.h" // For export macro

// VTK_RGB and VTK_RGBA are defined in system includes
#define VTK_ZBUFFER 5

VTK_ABI_NAMESPACE_BEGIN
class vtkWindow;

class vtkWTI2DHelperClass;
class VTKRENDERINGCORE_EXPORT vtkResizingWindowToImageFilter : public vtkAlgorithm
{
public:
  static vtkResizingWindowToImageFilter* New();

  vtkTypeMacro(vtkResizingWindowToImageFilter, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Indicates what renderer to get the pixel data from. Initial value is 0.
   */
  void SetInput(vtkWindow* input);

  ///@{
  /**
   * Returns which renderer is being used as the source for the pixel data.
   * Initial value is 0.
   */
  vtkGetObjectMacro(Input, vtkWindow);
  ///@}

  ///@{
  /**
   * Get/Set the size of the target image in pixels.
   */
  vtkGetVector2Macro(Size, int);
  vtkSetVector2Macro(Size, int);
  ///@}

  ///@{
  /**
   * Get/Set the size limit of the image (in pixels per axis) for switching
   * from rendering the entire image in memory to using tiling which uses less
   * memory but may fail to produce the exact size in pixels as requested.
   */
  vtkGetMacro(SizeLimit, int);
  vtkSetMacro(SizeLimit, int);
  ///@}

  ///@{
  /**
   * Set/get the window buffer from which data will be read.  Choices
   * include VTK_RGB (read the color image from the window), VTK_RGBA
   * (same, but include the alpha channel), and VTK_ZBUFFER (depth
   * buffer, returned as a float array). Initial value is VTK_RGB.
   */
  vtkSetMacro(InputBufferType, int);
  vtkGetMacro(InputBufferType, int);
  void SetInputBufferTypeToRGB() { this->SetInputBufferType(VTK_RGB); }
  void SetInputBufferTypeToRGBA() { this->SetInputBufferType(VTK_RGBA); }
  void SetInputBufferTypeToZBuffer() { this->SetInputBufferType(VTK_ZBUFFER); }
  ///@}

  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkImageData* GetOutput();

  /**
   * see vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkResizingWindowToImageFilter();
  ~vtkResizingWindowToImageFilter() override;

  // vtkWindow is not a vtkDataObject, so we need our own ivar.
  vtkWindow* Input;
  // requested size of the screenshot in pixels.
  int Size[2];
  // window size limit for using this filter. If the target resolution is higher we switch to
  // vtkWindowToImageFilter with tiling
  int SizeLimit;

  int InputBufferType;

  void RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  virtual void RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  // see algorithm for more info
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  /**
   * Allows subclasses to customize how a request for render is handled.
   * Default implementation checks if the render window has an interactor, if
   * so, call interactor->Render(). If not, then renderWindow->Render() is
   * called. Note, this may be called even when this->ShouldRerender is false,
   * e.g. when saving images Scale > 1.
   */
  virtual void Render();

  /**
   * Compute scale factors and new size for target resolution. This determines
   * integral scale factors (in X and Y) to get a box of size of \c targetSize from a
   * box of maximum size specified by \c size. If \c approximate is non-null,
   * then it is set to true when there no way to do that (e.g. one of the
   * components of the \c targetSize is prime and doesn't match \c size).
   *
   * On success, returns the scale factors and modifies \c size such that size *
   * scaleFactors == targetSize is possible. If not, size * scaleFactors <
   * targetSize and approximate if non-null, is set to true.
   *
   * The implementation comes from vtkSMSaveScreenshotProxy.cxx of ParaView
   *
   */
  void GetScaleFactorsAndSize(
    const int requestedSize[2], int actualSize[2], int scale[2], bool* approximate);

private:
  vtkResizingWindowToImageFilter(const vtkResizingWindowToImageFilter&) = delete;
  void operator=(const vtkResizingWindowToImageFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
