// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageThresholdConnectivity
 * @brief   Flood fill an image region.
 *
 * vtkImageThresholdConnectivity will perform a flood fill on an image,
 * given upper and lower pixel intensity thresholds. It works similarly
 * to vtkImageThreshold, but also allows the user to set seed points
 * to limit the threshold operation to contiguous regions of the image.
 * The filled region, or the "inside", will be passed through to the
 * output by default, while the "outside" will be replaced with zeros.
 * This behavior can be changed by using the ReplaceIn() and ReplaceOut()
 * methods.  The scalar type of the output is the same as the input.
 * @sa
 * vtkImageThreshold
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to VTK.
 */

#ifndef vtkImageThresholdConnectivity_h
#define vtkImageThresholdConnectivity_h

#include "vtkImageAlgorithm.h"
#include "vtkImagingMorphologicalModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints;
class vtkImageData;
class vtkImageStencilData;

class VTKIMAGINGMORPHOLOGICAL_EXPORT vtkImageThresholdConnectivity : public vtkImageAlgorithm
{
public:
  static vtkImageThresholdConnectivity* New();
  vtkTypeMacro(vtkImageThresholdConnectivity, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the seeds.  The seeds are in real data coordinates, not in
   * voxel index locations.
   */
  void SetSeedPoints(vtkPoints* points);
  vtkGetObjectMacro(SeedPoints, vtkPoints);
  ///@}

  /**
   * Values greater than or equal to this threshold will be filled.
   */
  void ThresholdByUpper(double thresh);

  /**
   * Values less than or equal to this threshold will be filled.
   */
  void ThresholdByLower(double thresh);

  /**
   * Values within this range will be filled, where the range includes
   * values that are exactly equal to the lower and upper thresholds.
   */
  void ThresholdBetween(double lower, double upper);

  ///@{
  /**
   * Replace the filled region by the value set by SetInValue().
   */
  vtkSetMacro(ReplaceIn, vtkTypeBool);
  vtkGetMacro(ReplaceIn, vtkTypeBool);
  vtkBooleanMacro(ReplaceIn, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If ReplaceIn is set, the filled region will be replaced by this value.
   */
  void SetInValue(double val);
  vtkGetMacro(InValue, double);
  ///@}

  ///@{
  /**
   * Replace the filled region by the value set by SetInValue().
   */
  vtkSetMacro(ReplaceOut, vtkTypeBool);
  vtkGetMacro(ReplaceOut, vtkTypeBool);
  vtkBooleanMacro(ReplaceOut, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If ReplaceOut is set, outside the fill will be replaced by this value.
   */
  void SetOutValue(double val);
  vtkGetMacro(OutValue, double);
  ///@}

  ///@{
  /**
   * Get the Upper and Lower thresholds.
   */
  vtkGetMacro(UpperThreshold, double);
  vtkGetMacro(LowerThreshold, double);
  ///@}

  ///@{
  /**
   * Limit the flood to a range of slices in the specified direction.
   */
  vtkSetVector2Macro(SliceRangeX, int);
  vtkGetVector2Macro(SliceRangeX, int);
  vtkSetVector2Macro(SliceRangeY, int);
  vtkGetVector2Macro(SliceRangeY, int);
  vtkSetVector2Macro(SliceRangeZ, int);
  vtkGetVector2Macro(SliceRangeZ, int);
  ///@}

  ///@{
  /**
   * Specify a stencil that will be used to limit the flood fill to
   * an arbitrarily-shaped region of the image.
   */
  virtual void SetStencilData(vtkImageStencilData* stencil);
  vtkImageStencilData* GetStencil();
  ///@}

  ///@{
  /**
   * For multi-component images, you can set which component will be
   * used for the threshold checks.
   */
  vtkSetMacro(ActiveComponent, int);
  vtkGetMacro(ActiveComponent, int);
  ///@}

  ///@{
  /**
   * The radius of the neighborhood that must be within the threshold
   * values in order for the voxel to be included in the mask.  The
   * default radius is zero (one single voxel).  The radius is measured
   * in voxels.
   */
  vtkSetVector3Macro(NeighborhoodRadius, double);
  vtkGetVector3Macro(NeighborhoodRadius, double);
  ///@}

  ///@{
  /**
   * The fraction of the neighborhood that must be within the thresholds.
   * The default value is 0.5.
   */
  vtkSetClampMacro(NeighborhoodFraction, double, 0.0, 1.0);
  vtkGetMacro(NeighborhoodFraction, double);
  ///@}

  /**
   * Override the MTime to account for the seed points.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * After the filter has executed, use GetNumberOfVoxels() to find
   * out how many voxels were filled.
   */
  vtkGetMacro(NumberOfInVoxels, int);
  ///@}

protected:
  vtkImageThresholdConnectivity();
  ~vtkImageThresholdConnectivity() override;

  double UpperThreshold;
  double LowerThreshold;
  double InValue;
  double OutValue;
  vtkTypeBool ReplaceIn;
  vtkTypeBool ReplaceOut;

  double NeighborhoodRadius[3];
  double NeighborhoodFraction;

  vtkPoints* SeedPoints;

  int SliceRangeX[2];
  int SliceRangeY[2];
  int SliceRangeZ[2];

  int NumberOfInVoxels;

  int ActiveComponent;

  vtkImageData* ImageMask;

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkImageThresholdConnectivity(const vtkImageThresholdConnectivity&) = delete;
  void operator=(const vtkImageThresholdConnectivity&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
