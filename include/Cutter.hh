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
// .NAME vlCutter - Cut vlDataSets with user-specified implicit function
// .SECTION Description
// vlCutter is a filter to cut any subclass of vlImplicitFunction to 
// cut arbitrary vlDataSets. That is, a polygonal surface is created
// corresponding to the implicit function F(x,y,z) = 0.

#ifndef __vlCutter_h
#define __vlCutter_h

#include "DS2PolyF.hh"
#include "ImpFunc.hh"

class vlCutter : public vlDataSetToPolyFilter
{
public:
  vlCutter(vlImplicitFunction *cf=NULL);
  ~vlCutter();
  char *GetClassName() {return "vlCutter";};

  unsigned long int GetMTime();

  vlSetObjectMacro(CutFunction,vlImplicitFunction);
  vlGetObjectMacro(CutFunction,vlImplicitFunction);

protected:
  void Execute();
  vlImplicitFunction *CutFunction;
  
};

#endif


