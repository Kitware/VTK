/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDraw.h
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
// .NAME vtkImageDraw - A region that can be drawn into.
// .SECTION Description
// vtkImageDraw is a region object with methods to draw boxs, lines ...
// over the data.  


#ifndef __vtkImageDraw_h
#define __vtkImageDraw_h

#include <math.h>
#include "vtkImageRegion.h"


class vtkImageDraw : public vtkImageRegion
{
public:
  vtkImageDraw();
  char *GetClassName() {return "vtkImageDraw";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get DrawValue.  This is the value that is used when filling regions
  // or drawing lines.
  vtkSetMacro(DrawValue,float);
  vtkGetMacro(DrawValue,float);
  
  void FillBox(int min0, int max0, int min1, int max1);
  void FillTube(int x0, int y0, int x1, int y1, float radius);
  void DrawSegment(int x0, int y0, int x1, int y1);
  void DrawSegment3D(float *p0, float *p1);

protected:
  float DrawValue;
  
  int ClipSegment(int &a0, int &a1, int &b0, int &b1);
};



#endif


