/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageBlend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageBlend - blend images together using alpha or opacity
// .SECTION Description
// vtkImageBlend takes L, LA, RGB, or RGBA images as input and blends them
// according to the alpha values and/or the opacity setting for each input.
//
// The spacing, origin, extent, and number of components of the output are
// the same as those for the first input.  If the input has an alpha
// component, then this component is copied unchanged into the output.
// In addition, if the first input has either one component or two
// components i.e. if it is either L (greyscale) or LA (greyscale + alpha)
// then all other inputs must also be L or LA.
//
// Different blending modes are available:
//
// \em Normal (default) :
// This is the standard blending mode used by OpenGL and other graphics
// packages.  The output always has the same number of components
// and the same extent as the first input.  The alpha value of the first
// input is not used in the blending computation, instead it is copied
// directly to the output.
//
// \code
// output <- input[0]
// foreach input i {
//   foreach pixel px {
//     r <- input[i](px)(alpha) * opacity[i]
//     f <- (255 - r)
//     output(px) <- output(px) * f + input(px) * r
//   }
// }
// \endcode
//
// \em Compound :
// Images are compounded together and each component is scaled by the sum of
// the alpha/opacity values. Use the CompoundThreshold method to set
// specify a threshold in compound mode. Pixels with opacity*alpha less
// or equal than this threshold are ignored.
// The alpha value of the first input, if present, is NOT copied to the alpha
// value of the output.  The output always has the same number of components
// and the same extent as the first input.
//
// \code
// output <- 0
// foreach pixel px {
//   sum <- 0
//   foreach input i {
//     r <- input[i](px)(alpha) * opacity(i)
//     sum <- sum + r
//     if r > threshold {
//       output(px) <- output(px) + input(px) * r
//     }
//   }
//   output(px) <- output(px) / sum
// }
// \endcode

#ifndef vtkImageBlend_h
#define vtkImageBlend_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class vtkImageStencilData;

#define VTK_IMAGE_BLEND_MODE_NORMAL   0
#define VTK_IMAGE_BLEND_MODE_COMPOUND 1

class VTKIMAGINGCORE_EXPORT vtkImageBlend : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageBlend *New();
  vtkTypeMacro(vtkImageBlend,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Replace one of the input connections with a new input.  You can
  // only replace input connections that you previously created with
  // AddInputConnection() or, in the case of the first input,
  // with SetInputConnection().
  virtual void ReplaceNthInputConnection(int idx, vtkAlgorithmOutput* input);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void SetInputData(int num, vtkDataObject *input);
  void SetInputData(vtkDataObject *input) { this->SetInputData(0, input); };

  // Description:
  // Get one input to this filter. This method is only for support of
  // old-style pipeline connections.  When writing new code you should
  // use vtkAlgorithm::GetInputConnection(0, num).
  vtkDataObject *GetInput(int num);
  vtkDataObject *GetInput() { return this->GetInput(0); };

  // Description:
  // Get the number of inputs to this filter. This method is only for
  // support of old-style pipeline connections.  When writing new code
  // you should use vtkAlgorithm::GetNumberOfInputConnections(0).
  int GetNumberOfInputs() { return this->GetNumberOfInputConnections(0); };

  // Description:
  // Set the opacity of an input image: the alpha values of the image are
  // multiplied by the opacity.  The opacity of image idx=0 is ignored.
  void SetOpacity(int idx, double opacity);
  double GetOpacity(int idx);

  // Description:
  // Set a stencil to apply when blending the data.
  // Create a pipeline connection.
  void SetStencilConnection(vtkAlgorithmOutput *algOutput);

  // Description:
  // Set a stencil to apply when blending the data.
  void SetStencilData(vtkImageStencilData *stencil);
  vtkImageStencilData *GetStencil();

  // Description:
  // Set the blend mode
  vtkSetClampMacro(BlendMode,int,
                   VTK_IMAGE_BLEND_MODE_NORMAL,
                   VTK_IMAGE_BLEND_MODE_COMPOUND );
  vtkGetMacro(BlendMode,int);
  void SetBlendModeToNormal()
        {this->SetBlendMode(VTK_IMAGE_BLEND_MODE_NORMAL);};
  void SetBlendModeToCompound()
        {this->SetBlendMode(VTK_IMAGE_BLEND_MODE_COMPOUND);};
  const char *GetBlendModeAsString(void);

  // Description:
  // Specify a threshold in compound mode. Pixels with opacity*alpha less
  // or equal the threshold are ignored.
  vtkSetMacro(CompoundThreshold,double);
  vtkGetMacro(CompoundThreshold,double);

protected:
  vtkImageBlend();
  ~vtkImageBlend();

  virtual int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  void InternalComputeInputUpdateExtent(int inExt[6], int outExt[6],
                                        int inWExtent[6]);

  void ThreadedRequestData (vtkInformation* request,
                            vtkInformationVector** inputVector,
                            vtkInformationVector* outputVector,
                            vtkImageData ***inData, vtkImageData **outData,
                            int ext[6], int id);

  // see vtkAlgorithm for docs.
  virtual int FillInputPortInformation(int, vtkInformation*);

  // see vtkAlgorithm for docs.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  double *Opacity;
  int OpacityArrayLength;
  int BlendMode;
  double CompoundThreshold;
  int DataWasPassed;

private:
  vtkImageBlend(const vtkImageBlend&);  // Not implemented.
  void operator=(const vtkImageBlend&);  // Not implemented.
};

// Description:
// Get the blending mode as a descriptive string
inline const char *vtkImageBlend::GetBlendModeAsString()
{
  switch (this->BlendMode)
    {
    case VTK_IMAGE_BLEND_MODE_NORMAL:
      return "Normal";
    case VTK_IMAGE_BLEND_MODE_COMPOUND:
      return "Compound";
    default:
      return "Unknown Blend Mode";
    }
}


#endif




