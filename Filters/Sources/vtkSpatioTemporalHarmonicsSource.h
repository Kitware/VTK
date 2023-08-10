// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSpatioTemporalHarmonicsSource
 * @brief   Creates a vtkImageData with harmonics data.
 *
 * vtkSpatioTemporalHarmonicsSource creates a vtkImageData source
 * that will have harmonics data on its points. It simply applies
 * a vtkSpatioTemporalHarmonicsAttributes on the generated image.
 *
 * Note that if no harmonics is set, the source will apply the
 * filter with default harmonics.
 *
 * @sa vtkImageData vtkSpatioTemporalHarmonicsAttribute
 */

#ifndef vtkSpatioTemporalHarmonicsSource_h
#define vtkSpatioTemporalHarmonicsSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkImageAlgorithm.h"

#include <memory>

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSSOURCES_EXPORT vtkSpatioTemporalHarmonicsSource : public vtkImageAlgorithm
{
public:
  static vtkSpatioTemporalHarmonicsSource* New();

  vtkTypeMacro(vtkSpatioTemporalHarmonicsSource, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the image extent.
   * Default is (-1, 1, -1, 1, -1, 1).
   */
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);
  ///@}

  /**
   * Add an harmonic with all needed parameters.
   */
  void AddHarmonic(double amplitude, double temporalFrequency, double xWaveVector,
    double yWaveVector, double zWaveVector, double phase);

  /**
   * Clear all harmonics.
   */
  void ClearHarmonics();

  /**
   * Reset harmonics to default.
   */
  void ResetHarmonics();

protected:
  vtkSpatioTemporalHarmonicsSource();
  ~vtkSpatioTemporalHarmonicsSource() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkSpatioTemporalHarmonicsSource(const vtkSpatioTemporalHarmonicsSource&) = delete;
  void operator=(const vtkSpatioTemporalHarmonicsSource&) = delete;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  int WholeExtent[6] = { -1, 1, -1, 1, -1, 1 };
};

VTK_ABI_NAMESPACE_END
#endif
