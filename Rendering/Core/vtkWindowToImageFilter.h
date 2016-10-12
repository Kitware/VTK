/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowToImageFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWindowToImageFilter
 * @brief   Use a vtkWindow as input to image pipeline
 *
 * vtkWindowToImageFilter provides methods needed to read the data in
 * a vtkWindow and use it as input to the imaging pipeline. This is
 * useful for saving an image to a file for example. The window can
 * be read as either RGB or RGBA pixels;  in addition, the depth buffer
 * can also be read.   RGB and RGBA pixels are of type unsigned char,
 * while Z-Buffer data is returned as floats.  Use this filter
 * to convert RenderWindows or ImageWindows to an image format.
 *
 * @warning
 * A vtkWindow doesn't behave like other parts of the VTK pipeline: its
 * modification time doesn't get updated when an image is rendered.  As a
 * result, naive use of vtkWindowToImageFilter will produce an image of
 * the first image that the window rendered, but which is never updated
 * on subsequent window updates.  This behavior is unexpected and in
 * general undesirable.
 *
 * @warning
 * To force an update of the output image, call vtkWindowToImageFilter's
 * Modified method after rendering to the window.
 *
 * @warning
 * In VTK versions 4 and later, this filter is part of the canonical
 * way to output an image of a window to a file (replacing the
 * obsolete SaveImageAsPPM method for vtkRenderWindows that existed in
 * 3.2 and earlier).  Connect this filter to the output of the window,
 * and filter's output to a writer such as vtkPNGWriter.
 *
 * @warning
 * Reading back alpha planes is dependent on the correct operation of
 * the render window's GetRGBACharPixelData method, which in turn is
 * dependent on the configuration of the window's alpha planes.  As of
 * VTK 4.4+, machine-independent behavior is not automatically
 * assured because of these dependencies.
 *
 * @sa
 * vtkRendererSource vtkRendererPointCloudSource vtkWindow
 * vtkRenderLargeImage
*/

#ifndef vtkWindowToImageFilter_h
#define vtkWindowToImageFilter_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkImageData.h" // makes things a bit easier

// VTK_RGB and VTK_RGBA are defined in system includes
#define VTK_ZBUFFER 5

class vtkWindow;

class vtkWTI2DHelperClass;
class VTKRENDERINGCORE_EXPORT vtkWindowToImageFilter : public vtkAlgorithm
{
public:
  static vtkWindowToImageFilter *New();

  vtkTypeMacro(vtkWindowToImageFilter,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Indicates what renderer to get the pixel data from. Initial value is 0.
   */
  void SetInput(vtkWindow *input);

  //@{
  /**
   * Returns which renderer is being used as the source for the pixel data.
   * Initial value is 0.
   */
  vtkGetObjectMacro(Input,vtkWindow);
  //@}

  //@{
  /**
   * The magnification of the current render window. Initial value is 1.
   */
  vtkSetClampMacro(Magnification,int,1,2048);
  vtkGetMacro(Magnification,int);
  //@}

  //@{
  /**
   * When this->Magnification > 1, this class render the full image in tiles.
   * Sometimes that results in artificial artifacts at internal tile seams.
   * To overcome this issue, set this flag to true.
   */
  vtkSetMacro(FixBoundary, bool);
  vtkGetMacro(FixBoundary, bool);
  vtkBooleanMacro(FixBoundary, bool);
  //@}

  //@{
  /**
   * Set/Get the flag that determines which buffer to read from.
   * The default is to read from the front buffer.
   */
  vtkBooleanMacro(ReadFrontBuffer, int);
  vtkGetMacro(ReadFrontBuffer, int);
  vtkSetMacro(ReadFrontBuffer, int);
  //@}

  //@{
  /**
   * Set/get whether to re-render the input window. Initial value is true.
   * (This option makes no difference if Magnification > 1.)
   */
  vtkBooleanMacro(ShouldRerender, int);
  vtkSetMacro(ShouldRerender, int);
  vtkGetMacro(ShouldRerender, int);
  //@}

  //@{
  /**
   * Set/get the extents to be used to generate the image. Initial value is
   * {0,0,1,1} (This option does not work if Magnification > 1.)
   */
  void SetViewport(double, double, double, double);
  void SetViewport(double*);
  vtkGetVectorMacro(Viewport,double,4);
  //@}

  //@{
  /**
   * Set/get the window buffer from which data will be read.  Choices
   * include VTK_RGB (read the color image from the window), VTK_RGBA
   * (same, but include the alpha channel), and VTK_ZBUFFER (depth
   * buffer, returned as a float array). Initial value is VTK_RGB.
   */
  vtkSetMacro(InputBufferType, int);
  vtkGetMacro(InputBufferType, int);
  void SetInputBufferTypeToRGB() {this->SetInputBufferType(VTK_RGB);};
  void SetInputBufferTypeToRGBA() {this->SetInputBufferType(VTK_RGBA);};
  void SetInputBufferTypeToZBuffer() {this->SetInputBufferType(VTK_ZBUFFER);};
  //@}


  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkImageData* GetOutput();

  /**
   * see vtkAlgorithm for details
   */
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

protected:
  vtkWindowToImageFilter();
  ~vtkWindowToImageFilter();

  // vtkWindow is not a vtkDataObject, so we need our own ivar.
  vtkWindow *Input;
  int Magnification;
  int ReadFrontBuffer;
  int ShouldRerender;
  double Viewport[4];
  int InputBufferType;
  bool FixBoundary;

  void RequestData(vtkInformation *,
                   vtkInformationVector **, vtkInformationVector *);

  virtual void RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  // The following was extracted from vtkRenderLargeImage, and patch to handle viewports
  void Rescale2DActors();
  void Shift2DActors(int x, int y);
  void Restore2DActors();
  vtkWTI2DHelperClass *StoredData;

private:
  vtkWindowToImageFilter(const vtkWindowToImageFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWindowToImageFilter&) VTK_DELETE_FUNCTION;
};

#endif
