/*=========================================================================

  Program:   Visualization Library
  Module:    VoxelMod.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlVoxelModeller - convert arbitrary dataset to voxel (structured point) representation
// .SECTION Description
// vlVoxelModeller is a filter that converts an arbitrary data set to a
// structured point (i.e., voxel) representation.

#ifndef __vlVoxelModeller_h
#define __vlVoxelModeller_h

#include "DS2SPtsF.hh"

class vlVoxelModeller : public vlDataSetToStructuredPointsFilter 
{
public:
  vlVoxelModeller();
  ~vlVoxelModeller() {};
  char *GetClassName() {return "vlVoxelModeller";};
  void PrintSelf(ostream& os, vlIndent indent);

  float ComputeModelBounds();

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vlGetVectorMacro(SampleDimensions,int);

  // Description:
  // Specify distance away from surface of input geometry to sample. Smaller
  // values make large increases in performance.
  vlSetClampMacro(MaximumDistance,float,0.0,1.0);
  vlGetMacro(MaximumDistance,float);

  // Specify the position in space to perform the sampling.
  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vlGetVectorMacro(ModelBounds,float);

  void Write(char *);

protected:
  void Execute();
  int SampleDimensions[3];
  float MaximumDistance;
  float ModelBounds[6];
};

#endif


