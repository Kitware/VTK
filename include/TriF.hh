/*=========================================================================

  Program:   Visualization Library
  Module:    TriF.hh
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
// Converts PolyData (polygons and strips) strictly to triangles.
//
#ifndef __vlTriangleFilter_h
#define __vlTriangleFilter_h

#include "P2PF.hh"

class vlTriangleFilter : public vlPolyToPolyFilter
{
public:
  vlTriangleFilter() : PassVerts(0), PassLines(0) {};
  ~vlTriangleFilter() {};
  char *GetClassName() {return "vlTriangleFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlBooleanMacro(PassVerts,int);
  vlSetMacro(PassVerts,int);
  vlGetMacro(PassVerts,int);

  vlBooleanMacro(PassLines,int);
  vlSetMacro(PassLines,int);
  vlGetMacro(PassLines,int);

protected:
  // Usual data generation method
  void Execute();

  int PassVerts;
  int PassLines;
};

#endif


