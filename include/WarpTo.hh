/*=========================================================================

  Program:   Visualization Toolkit
  Module:    WarpTo.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkWarpTo - deform geometry by warping towards a point
// .SECTION Description
// vtkWarpTo is a filter that modifies point coordinates by moving
// points towards a user specified point times the scale factor. 

#ifndef __vtkWarpTo_h
#define __vtkWarpTo_h

#include "PtS2PtSF.hh"

class vtkWarpTo : public vtkPointSetToPointSetFilter
{
public:
  vtkWarpTo() : ScaleFactor(1.0) {};
  ~vtkWarpTo() {};
  char *GetClassName() {return "vtkWarpTo";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify value to scale displacement.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Get the warp to position.
  vtkGetVectorMacro(Position,float,3);
  // Description:
  // Sets the position to warp towards.
  vtkSetVector3Macro(Position,float);

protected:
  void Execute();
  float ScaleFactor;
  float Position[3];
};

#endif


