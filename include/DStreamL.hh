/*=========================================================================

  Program:   Visualization Library
  Module:    DStreamL.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDashedStreamLine - generate constant-time dashed streamline in arbitrary dataset
// .SECTION Description
// vlStreamLine is a filter that generates a streamline for an arbitrary 
// dataset. The streamline consists of a series of dashes, each of which 
// represents a constant time increment. (A streamline is a line that is 
// everywhere tangent to the vector field (see vlStreamLine).

#ifndef __vlDashedStreamLine_h
#define __vlDashedStreamLine_h

#include "StreamL.hh"

class vlDashedStreamLine : public vlStreamLine
{
public:
  vlDashedStreamLine();
  ~vlDashedStreamLine() {};
  char *GetClassName() {return "vlDashedStreamLine";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // For each dash, specify the fraction of the dash that is "on". A factor
  // of 1.0 will result in a continuous line, a factor of 0.5 will result in 
  // dashed that are half on and half off.
  vlSetClampMacro(DashFactor,float,0.01,1.0);
  vlGetMacro(DashFactor,float);

protected:
  // Convert streamer array into vlPolyData
  void Execute();

  // the fraction of on versus off in dash
  float DashFactor;
  
};

#endif


