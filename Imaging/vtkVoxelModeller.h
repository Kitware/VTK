/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelModeller.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVoxelModeller - convert an arbitrary dataset to a voxel representation
// .SECTION Description
// vtkVoxelModeller is a filter that converts an arbitrary data set to a
// structured point (i.e., voxel) representation. It is very similar to 
// vtkImplicitModeller, except that it doesn't record distance; instead it
// records occupancy. As such, it stores its results in the more compact
// form of 0/1 bits.
// .SECTION see also
// vtkImplicitModeller

#ifndef __vtkVoxelModeller_h
#define __vtkVoxelModeller_h

#include "vtkDataSetToImageFilter.h"

class VTK_IMAGING_EXPORT vtkVoxelModeller : public vtkDataSetToImageFilter 
{
public:
  vtkTypeRevisionMacro(vtkVoxelModeller,vtkDataSetToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct an instance of vtkVoxelModeller with its sample dimensions
  // set to (50,50,50), and so that the model bounds are
  // automatically computed from its input. The maximum distance is set to 
  // examine the whole grid. This could be made much faster, and probably
  // will be in the future.
  static vtkVoxelModeller *New();

  // Description:
  // Compute the ModelBounds based on the input geometry.
  float ComputeModelBounds(float origin[3], float ar[3]);

  // Description:
  // Set the i-j-k dimensions on which to sample the distance function.
  void SetSampleDimensions(int i, int j, int k);

  // Description:
  // Set the i-j-k dimensions on which to sample the distance function.
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Specify distance away from surface of input geometry to sample. Smaller
  // values make large increases in performance.
  vtkSetClampMacro(MaximumDistance,float,0.0,1.0);
  vtkGetMacro(MaximumDistance,float);

  // Description:
  // Specify the position in space to perform the voxelization.
  void SetModelBounds(float bounds[6]);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // The the volume out to a specified filename.
  void Write(char *);

protected:
  vtkVoxelModeller();
  ~vtkVoxelModeller() {};

  
  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *);

  int SampleDimensions[3];
  float MaximumDistance;
  float ModelBounds[6];
private:
  vtkVoxelModeller(const vtkVoxelModeller&);  // Not implemented.
  void operator=(const vtkVoxelModeller&);  // Not implemented.
};

#endif
