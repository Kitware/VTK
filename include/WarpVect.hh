/*=========================================================================

  Program:   Visualization Toolkit
  Module:    WarpVect.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkWarpVector - deform geometry with vector data
// .SECTION Description
// vtkWarpVector is a filter that modifies point coordinates by moving
// points along vector times the scale factor. Useful for showing flow
// profiles or mechanical deformation.

#ifndef __vtkWarpVector_h
#define __vtkWarpVector_h

#include "PtS2PtSF.hh"

class vtkWarpVector : public vtkPointSetToPointSetFilter
{
public:
  vtkWarpVector() : ScaleFactor(1.0) {};
  ~vtkWarpVector() {};
  char *GetClassName() {return "vtkWarpVector";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify value to scale displacement.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

protected:
  void Execute();
  float ScaleFactor;
};

#endif


