/*=========================================================================

  Program:   Visualization Library
  Module:    MaskPts.hh
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
// Sample subset of points and associated data
//
#ifndef __vlMaskPoints_h
#define __vlMaskPoints_h

#include "DS2PolyF.hh"

class vlMaskPoints : public vlDataSetToPolyFilter
{
public:
  vlMaskPoints():OnRatio(2),Offset(0) {};
  char *GetClassName() {return "vlMaskPoints";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Turn on every nth point
  vlSetClampMacro(OnRatio,int,1,LARGE_INTEGER);
  vlGetMacro(OnRatio,int);

  // Start with this point
  vlSetClampMacro(Offset,int,0,LARGE_INTEGER);
  vlGetMacro(Offset,int);

protected:
  void Execute();

  // every OnRatio point is on; all others are off.
  int OnRatio;
  // offset (or starting point id)
  int Offset;
};

#endif


