/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TranSPts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkTransformStructuredPoints - transform (and resample) vtkStructuredPoints
// .SECTION Description
// vtkTransformStructuredPoints is a filter that samples an input structured 
// point set with a "transformed" structured point set. The sampling process
// occurs as follows: each output point (or voxel) is transformed according
// to a user  specified transformation object. The point is used to sample
// the input. If the point does not fall inside the input structured point 
// set, then the point is assigned a fill value (user specified). Otherwise,
// tri-linear interpolation is used to assign the value. (This object is 
// used to support the computation of swept surfaces and volumes).

#ifndef __vtkTransformStructuredPoints_h
#define __vtkTransformStructuredPoints_h

#include "SPt2SPtF.hh"
#include "Trans.hh"

class vtkTransformStructuredPoints : public vtkStructuredPointsToStructuredPointsFilter
{
public:
  vtkTransformStructuredPoints();
  ~vtkTransformStructuredPoints() {};
  char *GetClassName() {return "vtkTransformStructuredPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify i-j-k dimensions to sample input with.
  vtkSetVector3Macro(SampleDimensions,int);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // All voxels not within input structured point set are assigned this value.
  vtkSetMacro(FillValue,float);
  vtkGetMacro(FillValue,float);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Specify object to transform output voxels prior to sampling.
  vtkSetObjectMacro(Transform,vtkTransform);
  vtkGetObjectMacro(Transform,vtkTransform);

  unsigned long int GetMTime();

protected:
  void Execute();

  int SampleDimensions[3];
  float FillValue;
  float ModelBounds[6];

  vtkTransform *Transform;
};

#endif


