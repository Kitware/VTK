/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMatte4d.h
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
// .NAME vtkImageMatte4d - Adds a border to an image.
// .SECTION Description
// vtkImageMatte4d adds a border to an image.  The border
// can have different widths for each axis.  Notice that this
// filter is not cached.  The input is used directly with no
// copying of data (unless absolutely necessary).


#ifndef __vtkImageMatte4d_h
#define __vtkImageMatte4d_h

#include "vtkImageSource.h"
#include "vtkImageRegion.h"

class vtkImageMatte4d : public vtkImageSource
{
public:
  vtkImageMatte4d();
  char *GetClassName() {return "vtkImageMatte4d";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void UpdateRegion(vtkImageRegion *region); 
  void UpdateImageInformation(vtkImageRegion *region); 
  unsigned long GetPipelineMTime();
  int GetDataType();

  void SetBorderWidths(int w0);
  void SetBorderWidths(int w0, int w1);
  void SetBorderWidths(int w0, int w1, int w2);

  // Description:
  // Set/Get the input to this filter
  vtkSetObjectMacro(Input,vtkImageSource);
  vtkGetObjectMacro(Input,vtkImageSource);

  // Description:
  // Set/Get the value to use as a border.
  vtkSetMacro(BorderValue,float);
  vtkGetMacro(BorderValue,float);

  // Description:
  // Set/Get the Border of the mat.
  vtkSetVector4Macro(BorderWidths,int);
  vtkGetVector4Macro(BorderWidths,int);
  
  // Description:
  // Set/Get the local coordinate system.
  vtkSetVector4Macro(Axes,int);
  vtkGetVector4Macro(Axes,int);

  
protected:
  int Axes[4];
  int BorderWidths[4];
  float BorderValue;
  vtkImageSource *Input;
  
};



#endif



