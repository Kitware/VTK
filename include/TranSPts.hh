/*=========================================================================

  Program:   Visualization Library
  Module:    TranSPts.hh
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
// Transform (and resample) StructuredPoints using transform object.
//
#ifndef __vlTransformStructuredPoints_h
#define __vlTransformStructuredPoints_h

#include "SPt2SPtF.hh"
#include "Trans.hh"

class vlTransformStructuredPoints : public vlStructuredPointsToStructuredPointsFilter
{
public:
  vlTransformStructuredPoints();
  ~vlTransformStructuredPoints() {};
  char *GetClassName() {return "vlTransformStructuredPoints";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetVector3Macro(SampleDimensions,int);
  vlGetVectorMacro(SampleDimensions,int);

  vlSetMacro(FillValue,float);
  vlGetMacro(FillValue,float);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vlGetVectorMacro(ModelBounds,float);

  vlSetObjectMacro(Transform,vlTransform);
  vlGetObjectMacro(Transform,vlTransform);

  unsigned long int GetMTime();

protected:
  void Execute();

  int SampleDimensions[3];
  float FillValue;
  float ModelBounds[6];

  vlTransform *Transform;
};

#endif


