/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCheckerboard.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageCheckerboard
 * @brief   show two images at once using a checkboard pattern
 *
 *  vtkImageCheckerboard displays two images as one using a checkerboard
 *  pattern. This filter can be used to compare two images. The
 *  checkerboard pattern is controlled by the NumberOfDivisions
 *  ivar. This controls the number of checkerboard divisions in the whole
 *  extent of the image.
*/

#ifndef vtkImageCheckerboard_h
#define vtkImageCheckerboard_h

#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"


class VTKIMAGINGGENERAL_EXPORT vtkImageCheckerboard : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageCheckerboard *New();
  vtkTypeMacro(vtkImageCheckerboard,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the number of divisions along each axis.
   */
  vtkSetVector3Macro(NumberOfDivisions,int);
  vtkGetVectorMacro(NumberOfDivisions,int,3);
  //@}

  /**
   * Set the two inputs to this filter
   */
  virtual void SetInput1Data(vtkDataObject *in) { this->SetInputData(0,in); }
  virtual void SetInput2Data(vtkDataObject *in) { this->SetInputData(1,in); }

protected:
  vtkImageCheckerboard();
  ~vtkImageCheckerboard()VTK_OVERRIDE {}

  void ThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData,
                                   int extent[6], int threadId) VTK_OVERRIDE;
  int NumberOfDivisions[3];
private:
  vtkImageCheckerboard(const vtkImageCheckerboard&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageCheckerboard&) VTK_DELETE_FUNCTION;
};

#endif













