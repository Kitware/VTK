/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StreamL.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStreamLine - generate streamline in arbitrary dataset
// .SECTION Description
// vtkStreamLine is a filter that generates a streamline for an arbitrary 
// dataset. A streamline is a line that is everywhere tangent to the vector
// field. Scalar values are also calculated along the streamline and can be 
// used to color the line. Streamlines are calculated by integrating from
// a starting point through the vector field. Integration can be performed
// forward in time (see where the line goes), backward in time (see where the
// line came from), or in both directions. It is also possible to compute
// vorticity along the streamline. Vorticity is the projection (i.e., dot
// product) of the flow rotation on the velocity vector, i.e., the rotation
// of flow around the streamline.
//   vtkStreamLine defines the instance variable StepLength. This parameter 
// controls the length of the line segments used to define the streamline.
// The streamline(s) will consist of one (or more) polylines with line
// segment lengths of size StepLength. Smaller values reduce in more line 
// primitives but smoother streamlines. The StepLength instance variable is 
// defined in terms of time (i.e., the distance that the particle travels in
// the specified time period) Thus the line segments will be smaller in areas
// of low velocity and larger in regions of high velocity. (NOTE: This is
// different than the IntegrationStepLength defined by the superclass
// vtkStreamer. IntegrationStepLength is used to control integration step 
// size and is expressed as a fraction of the cell length). The StepLength
// instance variable is important because subclasses of vtkStreamLine (e.g.,
// vtkDashedStreamLine) depend on this value to build their representation.
// .SECTION See Also
// vtkStreamer, vtkDashedStreamLine, vtkStreamPoints;

#ifndef __vtkStreamLine_h
#define __vtkStreamLine_h

#include "Streamer.hh"

class vtkStreamLine : public vtkStreamer
{
public:
  vtkStreamLine();
  ~vtkStreamLine() {};
  char *GetClassName() {return "vtkStreamLine";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the length of a line segment. Smaller values result in smoother
  // appearing streamlines but greater numbers of line primitives.
  vtkSetClampMacro(StepLength,float,0.000001,LARGE_FLOAT);
  vtkGetMacro(StepLength,float);

protected:
  // Convert streamer array into vtkPolyData
  void Execute();

  // the length of line primitives
  float StepLength;

};

#endif


