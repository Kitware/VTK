/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImpMod.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkImplicitModeller - compute distance from input geometry on structured point set
// .SECTION Description
// vtkImplicitModeller is a filter that computes the distance from the input
// geometry on a structured point set. This distance function can then be
// "contoured" to generate new, offset surfaces from the original geometry.

#ifndef __vtkImplicitModeller_h
#define __vtkImplicitModeller_h

#include "DS2SPtsF.hh"

class vtkImplicitModeller : public vtkDataSetToStructuredPointsFilter 
{
public:
  vtkImplicitModeller();
  ~vtkImplicitModeller() {};
  char *GetClassName() {return "vtkImplicitModeller";};
  void PrintSelf(ostream& os, vtkIndent indent);

  float ComputeModelBounds();

  // Description:
  // Specify i-j-k dimensions on which to sample distance function.
  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Specify distance away from surface of input geometry to sample. Smaller
  // values make large increases in performance.
  vtkSetClampMacro(MaximumDistance,float,0.0,1.0);
  vtkGetMacro(MaximumDistance,float);

  // Specify the position in space to perform the sampling.
  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // The outer boundary of the structured point set can be assigned a 
  // particular value. This can be used to close or "cap" all surfaces.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);
  
  // Description:
  // Specify the capping value to use.
  vtkSetMacro(CapValue,float);
  vtkGetMacro(CapValue,float);

protected:
  void Execute();
  void Cap(vtkFloatScalars *s);

  int SampleDimensions[3];
  float MaximumDistance;
  float ModelBounds[6];
  int Capping;
  float CapValue;
};

#endif


