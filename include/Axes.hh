/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Axes.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkAxes - create an x-y-z axes
// .SECTION Description
// vtkAxes creates three lines that form an x-y-z axes. The origin of the 
// axes is user specified (0,0,0 is default), and the size is specified 
// with a scale factor. Three scalar values are generated for the three 
// lines and can be used (via color map) to indicate a particular 
// coordinate axis.

#ifndef __vtkAxes_h
#define __vtkAxes_h

#include "PolySrc.hh"

class vtkAxes : public vtkPolySource 
{
public:
  vtkAxes();
  char *GetClassName() {return "vtkAxes";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the origin of the axes.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Set the scale factor of the axes. Used to control size.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

protected:
  void Execute();

  float Origin[3];
  float ScaleFactor;
};

#endif


