/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnify.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageMagnify
 * @brief   magnify an image by an integer value
 *
 * vtkImageMagnify maps each pixel of the input onto a nxmx... region
 * of the output.  Location (0,0,...) remains in the same place. The
 * magnification occurs via pixel replication, or if Interpolate is on,
 * by bilinear interpolation. Initially, interpolation is off and magnification
 * factors are set to 1 in all directions.
*/

#ifndef vtkImageMagnify_h
#define vtkImageMagnify_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCORE_EXPORT vtkImageMagnify : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageMagnify *New();
  vtkTypeMacro(vtkImageMagnify,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the integer magnification factors in the i-j-k directions.
   * Initially, factors are set to 1 in all directions.
   */
  vtkSetVector3Macro(MagnificationFactors,int);
  vtkGetVector3Macro(MagnificationFactors,int);
  //@}

  //@{
  /**
   * Turn interpolation on and off (pixel replication is used when off).
   * Initially, interpolation is off.
   */
  vtkSetMacro(Interpolate,int);
  vtkGetMacro(Interpolate,int);
  vtkBooleanMacro(Interpolate,int);
  //@}

protected:
  vtkImageMagnify();
  ~vtkImageMagnify()VTK_OVERRIDE {}

  int MagnificationFactors[3];
  int Interpolate;
  int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *) VTK_OVERRIDE;
  int RequestInformation(vtkInformation *,
                                 vtkInformationVector **,
                                 vtkInformationVector *) VTK_OVERRIDE;

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData,
                           vtkImageData **outData,
                           int outExt[6],
                           int id) VTK_OVERRIDE;

  void InternalRequestUpdateExtent(int *inExt, int *outExt);

private:
  vtkImageMagnify(const vtkImageMagnify&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageMagnify&) VTK_DELETE_FUNCTION;
};

#endif




