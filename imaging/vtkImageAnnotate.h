/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAnnotate.h
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
// .NAME vtkImageAnnotate - Test For SIN# placement
// .SECTION Description
// vtkImageAnnotate computes where SIN# should be placed.


#ifndef __vtkImageAnnotate_h
#define __vtkImageAnnotate_h

#include <math.h>
#include "vtkImagePaint.h"


class vtkImageAnnotate : public vtkImagePaint
{
public:
   vtkImageAnnotate();
  ~vtkImageAnnotate();
  char *GetClassName() {return "vtkImageAnnotate";};
  void PrintSelf(ostream& os, vtkIndent indent); 

  // Description:
  // Draw the bounds of the drawing.
  void ComputeBounds();
  
  // Description:
  // Test the automatic placement of annotations.
  // Draw Bounds must be called first.
  void Annotate(int idx);

  vtkSetMacro(Min0, int);
  vtkGetMacro(Min0, int);
  vtkSetMacro(Max0, int);
  vtkGetMacro(Max0, int);

  vtkSetMacro(Min1, int);
  vtkGetMacro(Min1, int);
  vtkSetMacro(Max1, int);
  vtkGetMacro(Max1, int);
  
  vtkSetMacro(Min01, int);
  vtkGetMacro(Min01, int);
  vtkSetMacro(Max01, int);
  vtkGetMacro(Max01, int);
  
  vtkSetMacro(Min10, int);
  vtkGetMacro(Min10, int);
  vtkSetMacro(Max10, int);
  vtkGetMacro(Max10, int);
  
  
  vtkSetMacro(Center0, int);
  vtkGetMacro(Center0, int);
  vtkSetMacro(Center1, int);
  vtkGetMacro(Center1, int);
  
  
protected:

  int Min0;
  int Max0;
  int Min1;
  int Max1;
  int Min01;
  int Max01;
  int Min10;
  int Max10;
  
  int Center0;
  int Center1;
};



#endif


