/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageBlend.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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
// .NAME vtkImageBlend - blend images together using alpha or opacity
// .SECTION Description
// vtkImageBlend takes L, LA, RGB, or RGBA images as input and blends them 
// according to the alpha values and/or the opacity setting for each input.
// The blending rules are very similar to those for VTK texture maps.   
// The alpha value of the first input, if present, is copied to the alpha 
// value of the output.  The output always has the same number of components
// and the same extent as the first input.  

#ifndef __vtkImageBlend_h
#define __vtkImageBlend_h


#include "vtkImageMultipleInputFilter.h"

class VTK_EXPORT vtkImageBlend : public vtkImageMultipleInputFilter
{
public:
  static vtkImageBlend *New();
  const char *GetClassName() {return "vtkImageBlend";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the opacity of an input image: the alpha values of the image are
  // multiplied by the opacity.  The opacity of image idx=0 is ignored.
  void SetOpacity(int idx, double opacity);
  double GetOpacity(int idx);

  virtual void UpdateData(vtkDataObject *output);
  
protected:
  vtkImageBlend();
  ~vtkImageBlend();
  vtkImageBlend(const vtkImageBlend&) {};
  void operator=(const vtkImageBlend&) {};

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6],
				int whichInput);
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
		       int extent[6], int id);


  double *Opacity;
  int OpacityArrayLength;
};

#endif




