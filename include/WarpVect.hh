/*=========================================================================

  Program:   Visualization Library
  Module:    WarpVect.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Warp geometry in direction of vectors
//
#ifndef __vlWarpVector_h
#define __vlWarpVector_h

#include "PtS2PtSF.hh"

class vlWarpVector : public vlPointSetToPointSetFilter
{
public:
  vlWarpVector() : ScaleFactor(1.0) {};
  ~vlWarpVector() {};
  char *GetClassName() {return "vlWarpVector";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetMacro(ScaleFactor,float);
  vlGetMacro(ScaleFactor,float);

protected:
  void Execute();
  float ScaleFactor;
};

#endif


