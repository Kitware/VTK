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
//
// While the streamline representation is typically a line, it is also possible
// to generate a dashed line where the dash length is proportional to the 
// magnitude of the vector velocity.

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
  // Specify the length of a dash expressed in time. This is the combined 
  // length of both the "on" and "off" parts of the dash.
  vtkSetClampMacro(DashTime,float,0.000001,LARGE_FLOAT);
  vtkGetMacro(DashTime,float);

protected:
  // Convert streamer array into vtkPolyData
  void Execute();

  // the combined length of on/off portion of dash
  float DashTime;

};

#endif


