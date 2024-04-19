// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSpatioTemporalHarmonicsAttribute
 * @brief   Computes spatio-temporal harmonics on each point.
 *
 * vtkSpatioTemporalHarmonicsAttribute is a filter that adds a
 * data array storing spatio-temporal harmonics defined by the
 * filter. Those harmonics are defined by their amplitude,
 * temporal frequency, wave vector, and phase, with only a
 * sinus function (no cosinus).
 *
 * Note that the data array generated is a vtkDoubleArray that
 * is set as the output SCALARS attribute.
 *
 * @sa vtkSpatioTemporalHarmonicsSource
 */

#ifndef vtkSpatioTemporalHarmonicsAttribute_h
#define vtkSpatioTemporalHarmonicsAttribute_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

#include <array>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkSpatioTemporalHarmonicsAttribute : public vtkDataSetAlgorithm
{
public:
  static vtkSpatioTemporalHarmonicsAttribute* New();
  vtkTypeMacro(vtkSpatioTemporalHarmonicsAttribute, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * Whether the filter has harmonics set, or is empty.
   */
  bool HasHarmonics();

  /**
   * Compute spatio-temporal harmonic using filter-defined harmonics.
   */
  double ComputeValue(double coords[3], double time);

protected:
  vtkSpatioTemporalHarmonicsAttribute() = default;
  ~vtkSpatioTemporalHarmonicsAttribute() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkSpatioTemporalHarmonicsAttribute(const vtkSpatioTemporalHarmonicsAttribute&) = delete;
  void operator=(const vtkSpatioTemporalHarmonicsAttribute&) = delete;

  using Vector = std::array<double, 3>;

  std::vector<double> Amplitudes;
  std::vector<double> TemporalFrequencies;
  std::vector<Vector> WaveVectors;
  std::vector<double> Phases;
};

VTK_ABI_NAMESPACE_END
#endif
