/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamPoints.h
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
// .NAME vtkStreamPoints - generate points along streamer separated by constant time increment
// .SECTION Description
// vtkStreamPoints is a filter that generates points along a streamer.
// The points are separated by a constant time increment. The resulting visual
// effect (especially when coupled with vtkGlyph3D) is an indication of 
// particle speed.

// .SECTION See Also
// vtkStreamer vtkStreamLine vtkDashedStreamLine

#ifndef __vtkStreamPoints_h
#define __vtkStreamPoints_h

#include "vtkStreamer.h"

class VTK_GRAPHICS_EXPORT vtkStreamPoints : public vtkStreamer
{
public:
  vtkTypeRevisionMacro(vtkStreamPoints,vtkStreamer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with time increment set to 1.0.
  static vtkStreamPoints *New();

  // Description:
  // Specify the separation of points in terms of absolute time.
  vtkSetClampMacro(TimeIncrement,float,0.000001,VTK_LARGE_FLOAT);
  vtkGetMacro(TimeIncrement,float);

protected:
  vtkStreamPoints();
  ~vtkStreamPoints() {};

  // Convert streamer array into vtkPolyData
  void Execute();

  // the separation of points
  float TimeIncrement;
  
private:
  vtkStreamPoints(const vtkStreamPoints&);  // Not implemented.
  void operator=(const vtkStreamPoints&);  // Not implemented.
};

#endif


