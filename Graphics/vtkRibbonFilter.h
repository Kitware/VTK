/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRibbonFilter.h
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

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkRibbonFilter : public vtkPolyDataToPolyDataFilter 
{
public:
  vtkTypeRevisionMacro(vtkRibbonFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct ribbon so that width is 0.1, the width does 
  // not vary with scalar values, and the width factor is 2.0.
  static vtkRibbonFilter *New();

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

  // Description:
  // Set the default normal to use if no normals are supplied, and the
  // DefaultNormalOn is set.
  vtkSetVector3Macro(DefaultNormal,float);
  vtkGetVectorMacro(DefaultNormal,float,3);

  // Description:
  // Set a boolean to control whether to use default normals.
  // DefaultNormalOn is set.
  vtkSetMacro(UseDefaultNormal,int);
  vtkGetMacro(UseDefaultNormal,int);
  vtkBooleanMacro(UseDefaultNormal,int);

protected:
  vtkRibbonFilter();
  ~vtkRibbonFilter() {};

  void Execute();
  float Width;
  float Angle;
  int VaryWidth; //controls whether width varies with scalar data
  float WidthFactor;
  float DefaultNormal[3];
  int UseDefaultNormal;
private:
  vtkRibbonFilter(const vtkRibbonFilter&);  // Not implemented.
  void operator=(const vtkRibbonFilter&);  // Not implemented.
};

#endif


