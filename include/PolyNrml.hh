/*=========================================================================

  Program:   Visualization Library
  Module:    PolyNrml.hh
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
// Generates normals for PolyData
//
#ifndef __vlPolyNormals_h
#define __vlPolyNormals_h

#include "P2PF.hh"

class vlPolyNormals : public vlPolyToPolyFilter
{
public:
  vlPolyNormals();
  ~vlPolyNormals() {};
  char *GetClassName() {return "vlPolyNormals";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(FeatureAngle,float,0.0,180.0);
  vlGetMacro(FeatureAngle,float);

  vlSetMacro(Splitting,int);
  vlGetMacro(Splitting,int);
  vlBooleanMacro(Splitting,int);

  vlSetMacro(Consistency,int);
  vlGetMacro(Consistency,int);
  vlBooleanMacro(Consistency,int);

  vlSetMacro(FlipNormals,int);
  vlGetMacro(FlipNormals,int);
  vlBooleanMacro(FlipNormals,int);

  vlSetClampMacro(RecursionDepth,int,10,LARGE_INTEGER);
  vlGetMacro(RecursionDepth,int);

protected:
  // Usual data generation method
  void Execute();

  float FeatureAngle;
  int Splitting;
  int Consistency;
  int FlipNormals;
  int RecursionDepth;

  void TraverseAndOrder(int cellId);
  void MarkAndReplace(int cellId, int n, int replacement);
};

#endif


