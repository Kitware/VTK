/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageStencil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageToImageStencil
 * @brief   clip an image with a mask image
 *
 * vtkImageToImageStencil will convert a vtkImageData into an stencil
 * that can be used with vtkImageStecil or other vtk classes that apply
 * a stencil to an image.
 * @sa
 * vtkImageStencil vtkImplicitFunctionToImageStencil vtkPolyDataToImageStencil
*/

#ifndef vtkImageToImageStencil_h
#define vtkImageToImageStencil_h


#include "vtkImagingStencilModule.h" // For export macro
#include "vtkImageStencilAlgorithm.h"

class vtkImageData;

class VTKIMAGINGSTENCIL_EXPORT vtkImageToImageStencil : public vtkImageStencilAlgorithm
{
public:
  static vtkImageToImageStencil *New();
  vtkTypeMacro(vtkImageToImageStencil, vtkImageStencilAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Specify the image data to convert into a stencil.
   */
  void SetInputData(vtkImageData *input);
  vtkImageData *GetInput();
  //@}

  /**
   * The values greater than or equal to the value match.
   */
  void ThresholdByUpper(double thresh);

  /**
   * The values less than or equal to the value match.
   */
  void ThresholdByLower(double thresh);

  /**
   * The values in a range (inclusive) match
   */
  void ThresholdBetween(double lower, double upper);

  //@{
  /**
   * Get the Upper and Lower thresholds.
   */
  vtkSetMacro(UpperThreshold, double);
  vtkGetMacro(UpperThreshold, double);
  vtkSetMacro(LowerThreshold, double);
  vtkGetMacro(LowerThreshold, double);
  //@}

protected:
  vtkImageToImageStencil();
  ~vtkImageToImageStencil();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int, vtkInformation*);

  double UpperThreshold;
  double LowerThreshold;
  double Threshold;
private:
  vtkImageToImageStencil(const vtkImageToImageStencil&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageToImageStencil&) VTK_DELETE_FUNCTION;
};

#endif
