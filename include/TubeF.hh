/*=========================================================================

  Program:   Visualization Library
  Module:    TubeF.hh
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
// Generates a tube from input lines.  If scalar data is present, will vary
// radius of tube (if turned on).
//
#ifndef __vlTubeFilter_h
#define __vlTubeFilter_h

#include "P2PF.hh"

class vlTubeFilter : public vlPolyToPolyFilter
{
public:
  vlTubeFilter();
  ~vlTubeFilter() {};
  char *GetClassName() {return "vlTubeFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(Radius,float,0.0,LARGE_FLOAT);
  vlGetMacro(Radius,float);

  vlSetMacro(VaryRadius,int);
  vlGetMacro(VaryRadius,int);
  vlBooleanMacro(VaryRadius,int);

  vlSetClampMacro(NumberOfSides,int,0,LARGE_INTEGER);
  vlGetMacro(NumberOfSides,int);

  vlSetMacro(Rotation,float);
  vlGetMacro(Rotation,float);

protected:
  // Usual data generation method
  void Execute();

  float Radius; //minimum radius of tube
  int VaryRadius; //controls whether radius varies with scalar data
  int NumberOfSides; //number of sides to create tube
  float Rotation; //rotation of initial side of tube
};

#endif


