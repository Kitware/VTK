/*=========================================================================

  Program:   Visualization Toolkit
  Module:    RibbonF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkRibbonFilter - create oriented ribbons from lines defined in polygonal dataset
// .SECTION Description
// vtkRibbonFilter is a filter to create oriented ribbons from lines defined
// in polygonal dataset. The orientation of the ribbon is along the line 
// segments and perpendicular to "projected" line normals. Projected line 
// normals are the original line normals projected to be perpendicular to 
// the local line segment. An offset angle can be specified to rotate the 
// ribbon with respect to the normal.
//    The input line must not have duplicate points, or normals at points that
// are parallel to the incoming/outgoing line segments. (Duplicate points
// can be removed with vtkCleanPolyData).

#ifndef __vtkRibbonFilter_h
#define __vtkRibbonFilter_h

#include "P2PF.hh"

class vtkRibbonFilter : public vtkPolyToPolyFilter 
{
public:
  vtkRibbonFilter();
  ~vtkRibbonFilter() {};
  char *GetClassName() {return "vtkRibbonFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the "half" width of the ribbon. If the width is allowed to vary, 
  // this is the minimum width.
  vtkSetClampMacro(Width,float,0,LARGE_FLOAT);
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


