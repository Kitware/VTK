/*=========================================================================

  Program:   Visualization Toolkit
  Module:    VoxelMod.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkVoxelModeller - convert arbitrary dataset to voxel (structured point) representation
// .SECTION Description
// vtkVoxelModeller is a filter that converts an arbitrary data set to a
// structured point (i.e., voxel) representation.

#ifndef __vtkVoxelModeller_h
#define __vtkVoxelModeller_h

#include "DS2SPtsF.hh"

class vtkVoxelModeller : public vtkDataSetToStructuredPointsFilter 
{
public:
  vtkVoxelModeller();
  ~vtkVoxelModeller() {};
  char *GetClassName() {return "vtkVoxelModeller";};
  void PrintSelf(ostream& os, vtkIndent indent);

  float ComputeModelBounds();

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

  void Write(char *);

protected:
  void Execute();
  int SampleDimensions[3];
  float MaximumDistance;
  float ModelBounds[6];
};

#endif


