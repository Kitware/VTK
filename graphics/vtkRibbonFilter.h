/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRibbonFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkRibbonFilter - create oriented ribbons from lines defined in polygonal dataset
// .SECTION Description
// vtkRibbonFilter is a filter to create oriented ribbons from lines defined
// in polygonal dataset. The orientation of the ribbon is along the line 
// segments and perpendicular to "projected" line normals. Projected line 
// normals are the original line normals projected to be perpendicular to 
// the local line segment. An offset angle can be specified to rotate the 
// ribbon with respect to the normal.
//
// The input line must not have duplicate points, or normals at points that
// are parallel to the incoming/outgoing line segments. (Duplicate points
// can be removed with vtkCleanPolyData.)
// .SECTION See Also
// vtkTubeFilter

#ifndef __vtkRibbonFilter_h
#define __vtkRibbonFilter_h

#include "vtkPolyToPolyFilter.h"

class VTK_EXPORT vtkRibbonFilter : public vtkPolyToPolyFilter 
{
public:
  vtkRibbonFilter();
  vtkRibbonFilter *New() {return new vtkRibbonFilter;};
  char *GetClassName() {return "vtkRibbonFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the "half" width of the ribbon. If the width is allowed to vary, 
  // this is the minimum width.
  vtkSetClampMacro(Width,float,0,VTK_LARGE_FLOAT);
  vtkGetMacro(Width,float);

  // Description:
  // Set the offset angle of the ribbon from the line normal.
  vtkSetClampMacro(Angle,float,0,360);
  vtkGetMacro(Angle,float);

  // Description:
  // Turn on/off the variation of ribbon width with scalar value.
  vtkSetMacro(VaryWidth,int);
  vtkGetMacro(VaryWidth,int);
  vtkBooleanMacro(VaryWidth,int);

  // Description:
  // Set the maximum ribbon width in terms of a multiple of the minimum width.
  vtkSetMacro(WidthFactor,float);
  vtkGetMacro(WidthFactor,float);

protected:
  void Execute();
  float Width;
  float Angle;
  int VaryWidth; //controls whether width varies with scalar data
  float WidthFactor;
};

#endif


