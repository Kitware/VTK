/*=========================================================================

  Program:   Visualization Library
  Module:    Shepard.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkShepardMethod - sample unstructured points onto structured points using Shepard's method
// .SECTION Description
// vtkShepardMethod is a filter used to visualize unstructured point data using
// Shepard's method. The method works by resampling the unstructured points 
// onto a structured points set. The influence functions are described as 
// "inverse distance weighted". Once the structured points are computed, the 
// usual visualization techniques can be used visualize the structured points.
// .SECTION Caveats
//    The input to this filter is any dataset type. This this filter can be used
// to resample any form of data, i.e., the input data need not be unstructured.
//    The bounds of the data (i.e., the sample space) is automatically computed
// if not set by the user.
//    If you use a maximum distance less than 1.0, some output points will never
// receive a contribution. The final value of these points can be specified with
// the "NullValue" instance variable.

#ifndef __vtkShepardMethod_h
#define __vtkShepardMethod_h

#include "DS2SPtsF.hh"

class vtkShepardMethod : public vtkDataSetToStructuredPointsFilter 
{
public:
  vtkShepardMethod();
  ~vtkShepardMethod() {};
  char *GetClassName() {return "vtkShepardMethod";};
  void PrintSelf(ostream& os, vtkIndent indent);

  float ComputeModelBounds();

  // Description:
  // Specify i-j-k dimensions on which to sample input points.
  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Specify influence distance of each input point. This distance is a 
  // fraction of the length of the diagonal of the sample space. Thus values of 
  // 1.0 will cause each input point to influence all points in the structured 
  // point dataset. Values less than 1.0 can improve performance significantly.
  vtkSetClampMacro(MaximumDistance,float,0.0,1.0);
  vtkGetMacro(MaximumDistance,float);

  // Specify the position in space to perform the sampling.
  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Set the Null value for output points not receiving a contribution from the
  // input points.
  vtkSetMacro(NullValue,float);
  vtkGetMacro(NullValue,float);

protected:
  void Execute();

  int SampleDimensions[3];
  float MaximumDistance;
  float ModelBounds[6];
  float NullValue;
};

#endif


