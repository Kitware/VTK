/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnify.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkImageMagnify - magnify an image by an integer value
// .SECTION Description
// vtkImageMagnify maps each pixel of the input onto a nxmx... region
// of the output.  Location (0,0,...) remains in the same place. The
// magnification occurs via pixel replication, or if Interpolate is on,
// by bilinear interpolation.

#ifndef __vtkImageMagnify_h
#define __vtkImageMagnify_h

#include "vtkImageToImageFilter.h"

class VTK_EXPORT vtkImageMagnify : public vtkImageToImageFilter
{
public:
  static vtkImageMagnify *New();
  vtkTypeMacro(vtkImageMagnify,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the integer magnification factors in the i-j-k directions.
  vtkSetVector3Macro(MagnificationFactors,int);
  vtkGetVector3Macro(MagnificationFactors,int);
  
  // Description:
  // Turn interpolation on and off (pixel replication is used when off).
  vtkSetMacro(Interpolate,int);
  vtkGetMacro(Interpolate,int);
  vtkBooleanMacro(Interpolate,int);
  

protected:
  vtkImageMagnify();
  ~vtkImageMagnify() {};
  vtkImageMagnify(const vtkImageMagnify&) {};
  void operator=(const vtkImageMagnify&) {};

  int MagnificationFactors[3];
  int Interpolate;
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
		       int extent[6], int id);
};

#endif




