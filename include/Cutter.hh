/*=========================================================================

  Program:   Visualization Library
  Module:    Cutter.hh
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
// Uses user-specified implicit function f(x,y,z)=0 to cut datasets.
//
#ifndef __vlCutter_h
#define __vlCutter_h

#include "DS2PolyF.hh"
#include "ImpFunc.hh"

class vlCutter : public vlDataSetToPolyFilter
{
public:
  vlCutter(vlImplicitFunction *cf=0);
  ~vlCutter();
  char *GetClassName() {return "vlCutter";};

  vlSetObjectMacro(CutFunction,vlImplicitFunction);
  vlGetObjectMacro(CutFunction,vlImplicitFunction);

protected:
  void Execute();
  vlImplicitFunction *CutFunction;
  
};

#endif


