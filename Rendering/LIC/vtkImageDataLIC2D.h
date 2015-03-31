/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataLIC2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageDataLIC2D
//
// .SECTION Description
//  GPU implementation of a Line Integral Convolution, a technique for
//  imaging  vector fields.
//
//  The input on port 0 is an vtkImageData with extents of a 2D image. It needs
//  a vector field on point data. This filter only works on point vectors. One
//  can use a vtkCellDataToPointData filter to convert cell vectors to point
//  vectors.
//
//  Port 1 is a special port for customized noise input. It is an optional port.
//  If noise input is not specified, then the filter using vtkImageNoiseSource to
//  generate a 128x128 noise texture.
//
// .SECTION see also
//  vtkSurfaceLICPainter vtkLineIntegralConvolution2D

#ifndef vtkImageDataLIC2D_h
#define vtkImageDataLIC2D_h

#include "vtkRenderingLICModule.h" // For export macro
#include "vtkImageAlgorithm.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkRenderWindow;
class vtkImageNoiseSource;
class vtkImageCast;
class vtkShaderProgram2;

class VTKRENDERINGLIC_EXPORT vtkImageDataLIC2D : public vtkImageAlgorithm
{
public:
  static vtkImageDataLIC2D* New();
  vtkTypeMacro(vtkImageDataLIC2D, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the context. Context must be a vtkOpenGLRenderWindow.
  // This does not increase the reference count of the
  // context to avoid reference loops.
  // SetContext() may raise an error is the OpenGL context does not support the
  // required OpenGL extensions. Return 0 upon failure and 1 upon success.
  int SetContext( vtkRenderWindow * context );
  vtkRenderWindow * GetContext();

  // Description:
  // Number of steps. Initial value is 20.
  // class invariant: Steps>0.
  // In term of visual quality, the greater the better.
  vtkSetMacro(Steps,int);
  vtkGetMacro(Steps,int);

  // Description:
  // Step size.
  // Specify the step size as a unit of the cell length of the input vector
  // field. Cell lengthh is the length of the diagonal of a cell.
  // Initial value is 1.0.
  // class invariant: StepSize>0.0.
  // In term of visual quality, the smaller the better.
  // The type for the interface is double as VTK interface is double
  // but GPU only supports float. This value will be converted to
  // float in the execution of the algorithm.
  vtkSetMacro(StepSize, double);
  vtkGetMacro(StepSize, double);

  // Description:
  // The the magnification factor. Default is 1
  vtkSetMacro(Magnification, int);
  vtkGetMacro(Magnification, int);

  // Description:
  // Check if the required OpenGL extensions / GPU are supported.
  vtkGetMacro(OpenGLExtensionsSupported, int);

  void TranslateInputExtent(
        const int* inExt,
        const int* inWholeExtent,
        int *outExt);

//BTX
protected:
  vtkImageDataLIC2D();
  ~vtkImageDataLIC2D();

  virtual int RequestInformation(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Fill the input port information objects for this algorithm.  This
  // is invoked by the first call to GetInputPortInformation for each
  // port so subclasses can specify what they can handle.
  // Redefined from the superclass.
  virtual int FillInputPortInformation(int port,
                                       vtkInformation *info);

  int RequestUpdateExtent (vtkInformation * vtkNotUsed(request),
                           vtkInformationVector **inputVector,
                           vtkInformationVector *vtkNotUsed( outputVector ));

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  vtkWeakPointer<vtkRenderWindow> Context;
  bool OwnWindow;
  int OpenGLExtensionsSupported;

  vtkShaderProgram2 *MagShader;

  vtkImageNoiseSource* NoiseSource;
  vtkImageCast *ImageCast;

  int Steps;
  double StepSize;
  int Magnification;



private:
  vtkImageDataLIC2D(const vtkImageDataLIC2D&); // Not implemented.
  void operator=(const vtkImageDataLIC2D&); // Not implemented.
//ETX
};

#endif
