// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageDataLIC2D
 *
 *
 *  GPU implementation of a Line Integral Convolution, a technique for
 *  imaging vector fields.
 *
 *  The input on port 0 is an vtkImageData with extents of a 2D image. It needs
 *  a vector field on point data. This filter only works on point vectors. One
 *  can use a vtkCellDataToPointData filter to convert cell vectors to point
 *  vectors.
 *
 *  Port 1 is a special port for customized noise input. It is an optional port.
 *  If noise input is not specified, then the filter using vtkImageNoiseSource to
 *  generate a 128x128 noise texture.
 *
 * @sa
 *  vtkSurfaceLICPainter vtkLineIntegralConvolution2D
 */

#ifndef vtkImageDataLIC2D_h
#define vtkImageDataLIC2D_h

#include "vtkImageAlgorithm.h"
#include "vtkRenderingLICOpenGL2Module.h" // For export macro
#include "vtkWeakPointer.h"               // needed for vtkWeakPointer.

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderWindow;
class vtkOpenGLRenderWindow;
class vtkImageNoiseSource;
class vtkImageCast;

class VTKRENDERINGLICOPENGL2_EXPORT vtkImageDataLIC2D : public vtkImageAlgorithm
{
public:
  static vtkImageDataLIC2D* New();
  vtkTypeMacro(vtkImageDataLIC2D, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the context. Context must be a vtkOpenGLRenderWindow.
   * This does not increase the reference count of the
   * context to avoid reference loops.
   * SetContext() may raise an error if the OpenGL context does not support the
   * required OpenGL extensions. Return 0 upon failure and 1 upon success.
   */
  int SetContext(vtkRenderWindow* context);
  vtkRenderWindow* GetContext();
  ///@}

  ///@{
  /**
   * Number of steps. Initial value is 20.
   * class invariant: Steps>0.
   * In term of visual quality, the greater the better.
   */
  vtkSetMacro(Steps, int);
  vtkGetMacro(Steps, int);
  ///@}

  ///@{
  /**
   * Step size.
   * Specify the step size as a unit of the cell length of the input vector
   * field. Cell length is the length of the diagonal of a cell.
   * Initial value is 1.0.
   * class invariant: StepSize>0.0.
   * In term of visual quality, the smaller the better.
   * The type for the interface is double as VTK interface is double
   * but GPU only supports float. This value will be converted to
   * float in the execution of the algorithm.
   */
  vtkSetMacro(StepSize, double);
  vtkGetMacro(StepSize, double);
  ///@}

  ///@{
  /**
   * The magnification factor. Default is 1
   */
  vtkSetMacro(Magnification, int);
  vtkGetMacro(Magnification, int);
  ///@}

  ///@{
  /**
   * Check if the required OpenGL extensions / GPU are supported.
   */
  vtkGetMacro(OpenGLExtensionsSupported, int);
  ///@}

  void TranslateInputExtent(const int* inExt, const int* inWholeExtent, int* outExt);

protected:
  vtkImageDataLIC2D();
  ~vtkImageDataLIC2D() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Fill the input port information objects for this algorithm.  This
   * is invoked by the first call to GetInputPortInformation for each
   * port so subclasses can specify what they can handle.
   * Redefined from the superclass.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestUpdateExtent(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* vtkNotUsed(outputVector)) override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkWeakPointer<vtkOpenGLRenderWindow> Context;
  bool OwnWindow;
  int OpenGLExtensionsSupported;

  vtkImageNoiseSource* NoiseSource;
  vtkImageCast* ImageCast;

  int Steps;
  double StepSize;
  int Magnification;

private:
  vtkImageDataLIC2D(const vtkImageDataLIC2D&) = delete;
  void operator=(const vtkImageDataLIC2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
