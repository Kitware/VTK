/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelModeller.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#include "vtkImageAlgorithm.h"

class VTK_IMAGING_EXPORT vtkVoxelModeller : public vtkImageAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkVoxelModeller,vtkImageAlgorithm);
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
  double ComputeModelBounds(double origin[3], double ar[3]);

  // Description:
  // Set the i-j-k dimensions on which to sample the distance function.
  // Default is (50, 50, 50)
  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Specify distance away from surface of input geometry to sample. Smaller
  // values make large increases in performance. Default is 1.0.
  vtkSetClampMacro(MaximumDistance,double,0.0,1.0);
  vtkGetMacro(MaximumDistance,double);

  // Description:
  // Specify the position in space to perform the voxelization.
  // Default is (0, 0, 0, 0, 0, 0)
  void SetModelBounds(double bounds[6]);
  void SetModelBounds(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  vtkGetVectorMacro(ModelBounds,double,6);

protected:
  vtkVoxelModeller();
  ~vtkVoxelModeller() {};

  
  virtual int RequestInformation (vtkInformation *, 
                                  vtkInformationVector **, 
                                  vtkInformationVector *);

  // see vtkAlgorithm for details
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  int SampleDimensions[3];
  double MaximumDistance;
  double ModelBounds[6];
private:
  vtkVoxelModeller(const vtkVoxelModeller&);  // Not implemented.
  void operator=(const vtkVoxelModeller&);  // Not implemented.
};

#endif
