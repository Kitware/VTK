/*=========================================================================

  Program:   Visualization Library
  Module:    StreamPt.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStreamPoints - generate points along streamer separated by constant time increment
// .SECTION Description
// vlStreamPoints is a filter that generates points along a streamer.
// The points are separated by a constant time increment. The resulting visual
// effect (especially when coupled with vlGlyph3D) is an indication of particle
// speed.

#ifndef __vlStreamPoints_h
#define __vlStreamPoints_h

#include "Streamer.hh"

class vlStreamPoints : public vlStreamer
{
public:
  vlStreamPoints();
  ~vlStreamPoints() {};
  char *GetClassName() {return "vlStreamPoints";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify the separation of points in terms of absolute time.
  vlSetClampMacro(TimeIncrement,float,0.000001,LARGE_FLOAT);
  vlGetMacro(TimeIncrement,float);

protected:
  // Convert streamer array into vlPolyData
  void Execute();

  // the separation of points
  float TimeIncrement;
  
};

#endif


