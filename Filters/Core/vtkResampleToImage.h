// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkResampleToImage
 * @brief   sample dataset on a uniform grid
 *
 * vtkPResampleToImage is a filter that resamples the input dataset on
 * a uniform grid. It internally uses vtkProbeFilter to do the probing.
 * @sa
 * vtkProbeFilter
 */

#ifndef vtkResampleToImage_h
#define vtkResampleToImage_h

#include "vtkAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkNew.h"               // For vtkCompositeDataProbeFilter member variable

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkImageData;

class VTKFILTERSCORE_EXPORT vtkResampleToImage : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkResampleToImage, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkResampleToImage* New();

  ///@{
  /**
   * Set/Get if the filter should use Input bounds to sub-sample the data.
   * By default it is set to 1.
   */
  vtkSetMacro(UseInputBounds, bool);
  vtkGetMacro(UseInputBounds, bool);
  vtkBooleanMacro(UseInputBounds, bool);
  ///@}

  ///@{
  /**
   * Set/Get sampling bounds. If (UseInputBounds == 1) then the sampling
   * bounds won't be used.
   */
  vtkSetVector6Macro(SamplingBounds, double);
  vtkGetVector6Macro(SamplingBounds, double);
  ///@}

  ///@{
  /**
   * Set/Get sampling dimension along each axis. Default will be [10,10,10]
   */
  vtkSetVector3Macro(SamplingDimensions, int);
  vtkGetVector3Macro(SamplingDimensions, int);
  ///@}

  /**
   * Get the output data for this algorithm.
   */
  vtkImageData* GetOutput();

  /**
   * Get the name of the valid-points mask array.
   */
  const char* GetMaskArrayName() const;

protected:
  vtkResampleToImage();
  ~vtkResampleToImage() override;

  // Usual data generation method
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int FillInputPortInformation(int, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Resample input vtkDataObject to a vtkImageData with the specified bounds
   * and extent.
   */
  void PerformResampling(vtkDataObject* input, const double samplingBounds[6],
    bool computeProbingExtent, const double inputBounds[6], vtkImageData* output);

  /**
   * Mark invalid points and cells of vtkImageData as hidden
   */
  void SetBlankPointsAndCells(vtkImageData* data);

  /**
   * Helper function to compute the bounds of the given vtkDataSet or
   * vtkCompositeDataSet
   */
  static void ComputeDataBounds(vtkDataObject* data, double bounds[6]);

  bool UseInputBounds;
  double SamplingBounds[6];
  int SamplingDimensions[3];

private:
  vtkResampleToImage(const vtkResampleToImage&) = delete;
  void operator=(const vtkResampleToImage&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
