/*=========================================================================

  Program:   Visualization Library
  Module:    WarpTo.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlWarpTo - deform geometry by warping towards a point
// .SECTION Description
// vlWarpTo is a filter that modifies point coordinates by moving
// points towards a user specified point times the scale factor. 

#ifndef __vlWarpTo_h
#define __vlWarpTo_h

#include "PtS2PtSF.hh"

class vlWarpTo : public vlPointSetToPointSetFilter
{
public:
  vlWarpTo() : ScaleFactor(1.0) {};
  ~vlWarpTo() {};
  char *GetClassName() {return "vlWarpTo";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify value to scale displacement.
  vlSetMacro(ScaleFactor,float);
  vlGetMacro(ScaleFactor,float);

  // Description:
  // Get the warp to position.
  vlGetVectorMacro(Position,float,3);
  // Description:
  // Sets the position to warp towards.
  vlSetVector3Macro(Position,float);

protected:
  void Execute();
  float ScaleFactor;
  float Position[3];
};

#endif


