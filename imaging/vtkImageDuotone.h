/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDuotone.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkImageDuotone - For printing Duotone color images.
// .SECTION Description
// Given two ink colors in RGB (cyan =(0,1,1), Yellow = (1,1,0), ...),
// this filter computes two black and white images to overlay to get 
// a resonable approximation to the input color image.  In the outputs,
// high values (OutputMax) imply no ink, where 
// 0 implies alot of ink. Combination of colors
// from the two images is assumed to be subtractive.
// The filter uses a simple minded approach.  It minimizes the squared
// error (input - result) for each pixel.  The resulting images are
// clamped to remove negative values).
// The filter has two outputs:  Output0 for ink0, and output1 for ink1.
// InputMaximum refers to input and inks. (max, max, max) => white.



#ifndef __vtkImageDuotone_h
#define __vtkImageDuotone_h


#include "vtkImageTwoOutputFilter.h"

class VTK_EXPORT vtkImageDuotone : public vtkImageTwoOutputFilter
{
public:
  vtkImageDuotone();
  static vtkImageDuotone *New() {return new vtkImageDuotone;};
  char *GetClassName() {return "vtkImageDuotone";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the color of ink #1;
  vtkSetVector3Macro(Ink0,float);
  vtkGetVector3Macro(Ink0,float);

  // Description:
  // Set/Get the color of ink #2;
  vtkSetVector3Macro(Ink1,float);
  vtkGetVector3Macro(Ink1,float);

  // Description:
  // Set/Get the maximum of the output.  The maximum amount of ink.
  vtkSetMacro(OutputMaximum,float);
  vtkGetMacro(OutputMaximum,float);

  // Description:
  // Set/Get the maximum of the input.  (max, max, max) is white.
  vtkSetMacro(InputMaximum,float);
  vtkGetMacro(InputMaximum,float);

  
protected:
  float Ink0[3];
  float Ink1[3];
  float OutputMaximum;
  float InputMaximum;
  
  void ComputeOutputImageInformation(vtkImageRegion *inRegion,
				     vtkImageRegion *outRegion);
  void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
					vtkImageRegion *inRegion);
  void Execute(vtkImageRegion *inRegion, 
	       vtkImageRegion *outRegion0, vtkImageRegion *outRegion1);
};

#endif



