/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRDuotone.h
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
// .NAME vtkImageRDuotone - Reverse operation of duotone. Combines two images.
// .SECTION Description
// vtkImageRDuotone combines two images to show the result of a duotone
// process.  The input represent the amount of ink
// to be applied to the page (0 is alot of ink).  
// The page is assumed to be white.
// The output is an RGB image (RGB in components).  The pixel
// values range from 0 to OutputMaximum.

#ifndef __vtkImageRDuotone_h
#define __vtkImageRDuotone_h


#include "vtkImageTwoInputFilter.h"

class vtkImageRDuotone : public vtkImageTwoInputFilter
{
public:
  vtkImageRDuotone();
  char *GetClassName() {return "vtkImageRDuotone";};
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
  
  void ComputeOutputImageInformation(vtkImageRegion *in0Region,
				     vtkImageRegion *in1Region,
				     vtkImageRegion *outRegion);
  void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
					vtkImageRegion *in0Region,
					vtkImageRegion *in1Region);   
  void Execute(vtkImageRegion *inRegion1, 
	       vtkImageRegion *inRegion2, 
	       vtkImageRegion *outRegion);
};

#endif



