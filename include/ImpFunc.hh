/*=========================================================================

  Program:   Visualization Library
  Module:    ImpFunc.hh
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
// Abstract interface for implicit functions
//
#ifndef __vlImplicitFunction_h
#define __vlImplicitFunction_h

#include "Object.hh"

class vlImplicitFunction : public vlObject
{
public:
  char *GetClassName() {return "vlImplicitFunction";};

  virtual float Evaluate(float x, float y, float z) = 0;
};

#endif


