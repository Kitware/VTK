/*=========================================================================

  Program:   Visualization Library
  Module:    MaskPoly.hh
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
#ifndef __vlMaskPolyData_h
#define __vlMaskPolyData_h

#include "P2PF.hh"

class vlMaskPolyData : public vlPolyToPolyFilter
{
public:
  vlMaskPolyData();
  ~vlMaskPolyData();
  char *GetClassName() {return "vlMaskPolyData";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Turn on every nth entity
  vlSetClampMacro(OnRatio,int,1,LARGE_INTEGER);
  vlGetMacro(OnRatio,int);

  // Start with this point
  vlSetClampMacro(Offset,int,0,LARGE_INTEGER);
  vlGetMacro(Offset,int);

protected:
  void Execute();
  // every OnRatio entity is on; all others are off.
  int OnRatio;
  // offset (or starting point id)
  int Offset;
};

#endif


