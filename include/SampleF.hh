/*=========================================================================

  Program:   Visualization Library
  Module:    SampleF.hh
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
// Sample implicit function on StructuredPointSet
//
#ifndef __vlSampleFunction_h
#define __vlSampleFunction_h

#include "DS2SPtsF.hh"
#include "ImpFunc.hh"

class vlSampleFunction : public vlDataSetToStructuredPointsFilter 
{
public:
  vlSampleFunction();
  ~vlSampleFunction() {};
  char *GetClassName() {return "vlSampleFunction";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetObjectMacro(ImplicitFunction,vlImplicitFunction);
  vlGetObjectMacro(ImplicitFunction,vlImplicitFunction);

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vlGetVectorMacro(SampleDimensions,int);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vlGetVectorMacro(ModelBounds,float);

  vlSetMacro(Capping,int);
  vlGetMacro(Capping,int);
  vlBooleanMacro(Capping,int);
  
  vlSetMacro(CapValue,float);
  vlGetMacro(CapValue,float);

protected:
  void Execute();
  void Cap(vlFloatScalars *s);

  int SampleDimensions[3];
  float ModelBounds[6];
  int Capping;
  float CapValue;
  vlImplicitFunction *ImplicitFunction;
};

#endif


