/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StreamPt.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStreamPoints - generate points along streamer separated by constant time increment
// .SECTION Description
// vtkStreamPoints is a filter that generates points along a streamer.
// The points are separated by a constant time increment. The resulting visual
// effect (especially when coupled with vtkGlyph3D) is an indication of particle
// speed.

#ifndef __vtkStreamPoints_h
#define __vtkStreamPoints_h

#include "Streamer.hh"

class vtkStreamPoints : public vtkStreamer
{
public:
  vtkStreamPoints();
  ~vtkStreamPoints() {};
  char *GetClassName() {return "vtkStreamPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the separation of points in terms of absolute time.
  vtkSetClampMacro(TimeIncrement,float,0.000001,LARGE_FLOAT);
  vtkGetMacro(TimeIncrement,float);

protected:
  // Convert streamer array into vtkPolyData
  void Execute();

  // the separation of points
  float TimeIncrement;
  
};

#endif


