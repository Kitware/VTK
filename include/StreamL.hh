/*=========================================================================

  Program:   Visualization Library
  Module:    StreamL.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStreamLine - generate streamline in arbitrary dataset
// .SECTION Description
// vlStreamLine is a filter that generates a streamline for an arbitrary 
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

#ifndef __vlStreamLine_h
#define __vlStreamLine_h

#include "Streamer.hh"

class vlStreamLine : public vlStreamer
{
public:
  vlStreamLine();
  ~vlStreamLine() {};
  char *GetClassName() {return "vlStreamLine";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify the length of a dash expressed in time. This is the combined 
  // length of both the "on" and "off" parts of the dash.
  vlSetClampMacro(DashTime,float,0.000001,LARGE_FLOAT);
  vlGetMacro(DashTime,float);

protected:
  // Convert streamer array into vlPolyData
  void Execute();

  // the combined length of on/off portion of dash
  float DashTime;

};

#endif


