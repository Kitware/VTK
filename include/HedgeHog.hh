/*=========================================================================

  Program:   Visualization Library
  Module:    HedgeHog.hh
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
// Creates oriented lines directed along vector
//
#ifndef __vlHedgeHog_h
#define __vlHedgeHog_h

#include "DS2PolyF.hh"

class vlHedgeHog : public vlDataSetToPolyFilter
{
public:
  vlHedgeHog() : ScaleFactor(1.0) {};
  ~vlHedgeHog() {};
  char *GetClassName() {return "vlHedgeHog";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetMacro(ScaleFactor,float);
  vlGetMacro(ScaleFactor,float);

protected:
  void Execute();
  float ScaleFactor;

};

#endif


