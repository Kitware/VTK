/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DStreamL.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkDashedStreamLine - generate constant-time dashed streamline in arbitrary dataset
// .SECTION Description
// vtkDashedStreamLine is a filter that generates a "dashed" streamline for 
// an arbitrary dataset. The streamline consists of a series of dashes, each 
// of which represents (approximately) a constant time increment. Thus in the
// resulting visual representation, relatively long dashes represent areas of 
// high velocity, and small dashes represent areas of low velocity.
//   vtkDashedStreamLine introduces the instance variable DashFactor. 
// DashFactor interacts with its superclass' instance variable StepLength to
// create the dashes. DashFactor is the percentage of the StepLength line 
// segment that is visible. Thus if DashFactor=0.75, the dashes will be 
// "three-quarters on" and "one-quarter off".
// .SECTION See Also
// vtkStreamer, vtkStreamLine, vtkStreamPoints

#ifndef __vtkDashedStreamLine_h
#define __vtkDashedStreamLine_h

#include "StreamL.hh"

class vtkDashedStreamLine : public vtkStreamLine
{
public:
  vtkDashedStreamLine();
  ~vtkDashedStreamLine() {};
  char *GetClassName() {return "vtkDashedStreamLine";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // For each dash, specify the fraction of the dash that is "on". A factor
  // of 1.0 will result in a continuous line, a factor of 0.5 will result in 
  // dashed that are half on and half off.
  vtkSetClampMacro(DashFactor,float,0.01,1.0);
  vtkGetMacro(DashFactor,float);

protected:
  // Convert streamer array into vtkPolyData
  void Execute();

  // the fraction of on versus off in dash
  float DashFactor;
  
};

#endif


