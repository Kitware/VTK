/*=========================================================================

  Program:   Visualization Library
  Module:    ImpMod.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlImplicitModeller - compute distance from input geometry on structured point set
// .SECTION Description
// vlImplicitModeller is a filter that computes the distance from the input
// geometry on a structured point set. This distance function can then be
// "contoured" to generate new, offset surfaces from the original geometry.

#ifndef __vlImplicitModeller_h
#define __vlImplicitModeller_h

#include "DS2SPtsF.hh"

class vlImplicitModeller : public vlDataSetToStructuredPointsFilter 
{
public:
  vlImplicitModeller();
  ~vlImplicitModeller() {};
  char *GetClassName() {return "vlImplicitModeller";};
  void PrintSelf(ostream& os, vlIndent indent);

  float ComputeModelBounds();

  // Description:
  // Specify i-j-k dimensions on which to sample distance function.
  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vlGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Specify distance away from surface of input geometry to sample. Smaller
  // values make large increases in performance.
  vlSetClampMacro(MaximumDistance,float,0.0,1.0);
  vlGetMacro(MaximumDistance,float);

  // Specify the position in space to perform the sampling.
  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vlGetVectorMacro(ModelBounds,float,6);

  // Description:
  // The outer boundary of the structured point set can be assigned a 
  // particular value. This can be used to close or "cap" all surfaces.
  vlSetMacro(Capping,int);
  vlGetMacro(Capping,int);
  vlBooleanMacro(Capping,int);
  
  // Description:
  // Specify the capping value to use.
  vlSetMacro(CapValue,float);
  vlGetMacro(CapValue,float);

protected:
  void Execute();
  void Cap(vlFloatScalars *s);

  int SampleDimensions[3];
  float MaximumDistance;
  float ModelBounds[6];
  int Capping;
  float CapValue;
};

#endif


