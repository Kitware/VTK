/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDifference.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageDifference - Compares images for regression tests.
// .SECTION Description
// vtkImageDifference takes two rgb unsigned char images and compares them.
// It allows the images to be slightly different.  If AllowShift is on,
// then each pixel can be shifted by one pixel. Threshold is the allowable
// error for each pixel.

#ifndef __vtkImageDifference_h
#define __vtkImageDifference_h

#include "vtkImageTwoInputFilter.h"

class VTK_IMAGING_EXPORT vtkImageDifference : public vtkImageTwoInputFilter
{
public:
  static vtkImageDifference *New();
  vtkTypeRevisionMacro(vtkImageDifference,vtkImageTwoInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the Image to compare the input to.
  void SetImage(vtkImageData *image) {this->SetInput2(image);}
  vtkImageData *GetImage() {return this->GetInput2();}

  // Description:
  // Specify the Input for comparison.
  void SetInput(vtkImageData *input) {this->SetInput1(input);}
  void SetInput(int num, vtkImageData *input)
    { this->vtkImageMultipleInputFilter::SetInput(num, input); };
  
  // Description:
  // Return the total error in comparing the two images.
  float GetError(void);
  void GetError(float *e) { *e = this->GetError(); };
  
  // Description:
  // Return the total thresholded error in comparing the two images.
  // The thresholded error is the error for a given pixel minus the
  // threshold and clamped at a minimum of zero. 
  float GetThresholdedError(void);
  void GetThresholdedError(float *e) { *e = this->GetThresholdedError(); };


  // Description:
  // Specify a threshold tolerance for pixel differences.
  vtkSetMacro(Threshold,int);
  vtkGetMacro(Threshold,int);

  // Description:
  // Specify whether the comparison will allow a shift of one
  // pixel between the images.  If set, then the minimum difference
  // between input images will be used to determine the difference.
  // Otherwise, the difference is computed directly between pixels
  // of identical row/column values.
  vtkSetMacro(AllowShift,int);
  vtkGetMacro(AllowShift,int);
  vtkBooleanMacro(AllowShift,int);

  // Description:
  // Specify whether the comparison will include comparison of
  // averaged 3x3 data between the images. For graphics renderings
  // you normally would leave this on. For imaging operations it
  // should be off.
  vtkSetMacro(Averaging,int);
  vtkGetMacro(Averaging,int);
  vtkBooleanMacro(Averaging,int);

protected:
  vtkImageDifference();
  ~vtkImageDifference() {};

  float ErrorPerThread[VTK_MAX_THREADS];
  float ThresholdedErrorPerThread[VTK_MAX_THREADS];
  int AllowShift;
  int Threshold;
  int Averaging;
  
  void ExecuteInformation(vtkImageData **inputs, vtkImageData *output); 
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6],
                                int whichInput);
  void ExecuteInformation(){this->vtkImageTwoInputFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
                       int extent[6], int id);  
  
private:
  vtkImageDifference(const vtkImageDifference&);  // Not implemented.
  void operator=(const vtkImageDifference&);  // Not implemented.
};

#endif


