// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSpatioTemporalHarmonicsSource
 * @brief   Creates a vtkImageData with harmonics data.
 *
 * vtkSpatioTemporalHarmonicsSource creates a vtkImageData source
 * that will have harmonics data on its points. It simply applies
 * a vtkSpatioTemporalHarmonicsAttributes on the generated image.
 * It also allows generation of time steps.
 *
 * Note that default harmonics and time step values are set for
 * common usage. Make sure to clear them before adding your own
 * values.
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
   * Default is (-10, 10, -10, 10, -10, 10).
   */
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);
  ///@}

  /**
   * Add a time step value.
   * You may want to remove default values first.
   */
  void AddTimeStepValue(double timeStepValue);

  /**
   * Clear time step values.
   */
  void ClearTimeStepValues();

  /**
   * Reset time step values to default.
   * By default, the source has 20 time steps ranging from 0 to 2*PI.
   * The default values allows an infinite loop of default harmonics.
   */
  void ResetTimeStepValues();

  /**
   * Add an harmonic with all needed parameters.
   * You may want to remove default harmonics first.
   */
  void AddHarmonic(double amplitude, double temporalFrequency, double xWaveVector,
    double yWaveVector, double zWaveVector, double phase);

  /**
   * Clear all harmonics.
   */
  void ClearHarmonics();

  /**
   * Reset harmonics to default.
   * By default, the source has harmonics in each direction, with phase shifts and different
   * frequencies. The default wave vector is scaled to match default extent.
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

  int WholeExtent[6] = { -10, 10, -10, 10, -10, 10 };
};

VTK_ABI_NAMESPACE_END
#endif
