/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageThresholdConnectivity.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageThresholdConnectivity - Flood fill an image region.
// .SECTION Description
// vtkImageThresholdConnectivity will perform a flood fill on an image,
// given upper and lower pixel intensity thresholds. It works similarly
// to vtkImageThreshold, but also allows the user to set seed points
// to limit the threshold operation to contiguous regions of the image.
// The filled region, or the "inside", will be passed through to the
// output by default, while the "outside" will be replaced with zeros.
// This behavior can be changed by using the ReplaceIn() and ReplaceOut()
// methods.  The scalar type of the output is the same as the input.
// .SECTION see also
// vtkImageThreshold
// .SECTION Thanks
// Thanks to David Gobbi for contributing this class to VTK.

#ifndef __vtkImageThresholdConnectivity_h
#define __vtkImageThresholdConnectivity_h

#include "vtkImageAlgorithm.h"

class vtkPoints;
class vtkImageData;
class vtkImageStencilData;

class VTK_IMAGING_EXPORT vtkImageThresholdConnectivity :
  public vtkImageAlgorithm
{
public:
  static vtkImageThresholdConnectivity *New();
  vtkTypeMacro(vtkImageThresholdConnectivity, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the seeds.  The seeds are in real data coordinates, not in
  // voxel index locations.
  void SetSeedPoints(vtkPoints *points);
  vtkGetObjectMacro(SeedPoints, vtkPoints);

  // Description:
  // Values greater than or equal to this threshold will be filled.
  void ThresholdByUpper(double thresh);

  // Description:
  // Values less than or equal to this threshold will be filled.
  void ThresholdByLower(double thresh);

  // Description:
  // Values within this range will be filled, where the range inludes
  // values that are exactly equal to the lower and upper thresholds.
  void ThresholdBetween(double lower, double upper);

  // Description:
  // Replace the filled region by the value set by SetInValue().
  vtkSetMacro(ReplaceIn, int);
  vtkGetMacro(ReplaceIn, int);
  vtkBooleanMacro(ReplaceIn, int);

  // Description:
  // If ReplaceIn is set, the filled region will be replaced by this value.
  void SetInValue(double val);
  vtkGetMacro(InValue, double);

  // Description:
  // Replace the filled region by the value set by SetInValue().
  vtkSetMacro(ReplaceOut, int);
  vtkGetMacro(ReplaceOut, int);
  vtkBooleanMacro(ReplaceOut, int);

  // Description:
  // If ReplaceOut is set, outside the fill will be replaced by this value.
  void SetOutValue(double val);
  vtkGetMacro(OutValue, double);

  // Description:
  // Get the Upper and Lower thresholds.
  vtkGetMacro(UpperThreshold, double);
  vtkGetMacro(LowerThreshold, double);

  // Description:
  // Limit the flood to a range of slices in the specified direction.
  vtkSetVector2Macro(SliceRangeX, int);
  vtkGetVector2Macro(SliceRangeX, int);
  vtkSetVector2Macro(SliceRangeY, int);
  vtkGetVector2Macro(SliceRangeY, int);
  vtkSetVector2Macro(SliceRangeZ, int);
  vtkGetVector2Macro(SliceRangeZ, int);

  // Description:
  // Specify a stencil that will be used to limit the flood fill to
  // an arbitrarily-shaped region of the image.
  virtual void SetStencil(vtkImageStencilData *stencil);
  vtkImageStencilData *GetStencil();

  // Description:
  // For multi-component images, you can set which component will be
  // used for the threshold checks.
  vtkSetMacro(ActiveComponent,int);
  vtkGetMacro(ActiveComponent,int);

  // Description:
  // The radius of the neighborhood that must be within the threshold
  // values in order for the voxel to be included in the mask.  The
  // default radius is zero (one single voxel).  The radius is measured
  // in voxels.
  vtkSetVector3Macro(NeighborhoodRadius, double);
  vtkGetVector3Macro(NeighborhoodRadius, double);

  // Description:
  // The fraction of the neighborhood that must be within the thresholds.
  // The default value is 0.5.
  vtkSetClampMacro(NeighborhoodFraction, double, 0.0, 1.0);
  vtkGetMacro(NeighborhoodFraction, double);

  // Description:
  // Override the MTime to account for the seed points.
  unsigned long GetMTime();

  // Description:
  // After the filter has executed, use GetNumberOfVoxels() to find
  // out how many voxels were filled.
  vtkGetMacro(NumberOfInVoxels, int);

protected:
  vtkImageThresholdConnectivity();
  ~vtkImageThresholdConnectivity();

  double UpperThreshold;
  double LowerThreshold;
  double InValue;
  double OutValue;
  int ReplaceIn;
  int ReplaceOut;

  double NeighborhoodRadius[3];
  double NeighborhoodFraction;

  vtkPoints *SeedPoints;

  int SliceRangeX[2];
  int SliceRangeY[2];
  int SliceRangeZ[2];

  int NumberOfInVoxels;

  int ActiveComponent;

  vtkImageData *ImageMask;

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);

  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

private:
  vtkImageThresholdConnectivity(const vtkImageThresholdConnectivity&);  // Not implemented.
  void operator=(const vtkImageThresholdConnectivity&);  // Not implemented.
};

#endif
