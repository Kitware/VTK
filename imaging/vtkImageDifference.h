/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDifference.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

class VTK_EXPORT vtkImageDifference : public vtkImageTwoInputFilter
{
public:

  vtkImageDifference();
  static vtkImageDifference *New() {return new vtkImageDifference;};
  const char *GetClassName() {return "vtkImageDifference";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the Image to compare the input to.
  void SetImage(vtkImageCache *image) {this->SetInput2(image);}
  void SetImage(vtkStructuredPoints *image) {this->SetInput2(image);}
  vtkImageCache *GetImage() {return this->GetInput2();}

  // Description:
  // Specify the Input for comparison.
  void SetInput(vtkImageCache *input) {this->SetInput1(input);}
  void SetInput(vtkStructuredPoints *input) {this->SetInput1(input);}
  vtkImageCache *GetInput() {return this->GetInput1();}

  // Description:
  // Return the total error in comparing the two images.
  vtkGetMacro(Error,float);
  
  // Description:
  // Return the total thresholded error in comparing the two images.
  // The thresholded error is the error for a given pixel minus the
  // threshold and clamped at a minimum of zero. 
  vtkGetMacro(ThresholdedError,float);

  // Description:
  // Specify a threshold tolorance for pixel differences.
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
  float Error;
  float ThresholdedError;
  int AllowShift;
  int Threshold;
  int Averaging;
  
  void ExecuteImageInformation(); 
  void ComputeRequiredInputUpdateExtent(int inExt[6], int outExt[6],
					int whichInput);
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
		       int extent[6], int id);  
  
};

#endif


