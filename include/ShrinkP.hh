/*=========================================================================

  Program:   Visualization Library
  Module:    ShrinkP.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Class to shrink PolyData
//
#ifndef __vlShrinkPolyData_h
#define __vlShrinkPolyData_h

#include "P2PF.hh"

class vlShrinkPolyData : public vlPolyToPolyFilter 
{
public:
  vlShrinkPolyData() {this->ShrinkFactor = 0.5;};
  ~vlShrinkPolyData() {};
  char *GetClassName() {return "vlShrinkPolyData";};

  vlSetMacro(ShrinkFactor,float);
  vlGetMacro(ShrinkFactor,float);

protected:
  void Execute();
  float ShrinkFactor;
};

#endif


