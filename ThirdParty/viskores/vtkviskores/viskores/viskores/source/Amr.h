//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_source_Amr_h
#define viskores_source_Amr_h

#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/source/viskores_source_export.h>

namespace viskores
{
namespace source
{
/**
 * @brief The Amr source creates a dataset similar to VTK's
 * vtkRTAnalyticSource.
 *
 * This class generates a predictable structured dataset with a smooth yet
 * interesting set of scalars, which is useful for testing and benchmarking.
 *
 * The Execute method creates a complete structured dataset that have a
 * point field names 'scalars'
 *
 * The scalars are computed as:
 *
 * ```
 * MaxVal * Gauss + MagX * sin(FrqX*x) + MagY * sin(FrqY*y) + MagZ * cos(FrqZ*z)
 * ```
 *
 * The dataset properties are determined by:
 * - `Minimum/MaximumExtent`: The logical point extents of the dataset.
 * - `Spacing`: The distance between points of the dataset.
 * - `Center`: The center of the dataset.
 *
 * The scalar functions is control via:
 * - `Center`: The center of a Gaussian contribution to the scalars.
 * - `StandardDeviation`: The unscaled width of a Gaussian contribution.
 * - `MaximumValue`: Upper limit of the scalar range.
 * - `Frequency`: The Frq[XYZ] parameters of the periodic contributions.
 * - `Magnitude`: The Mag[XYZ] parameters of the periodic contributions.
 *
 * By default, the following parameters are used:
 * - `Extents`: { -10, -10, -10 } `-->` { 10, 10, 10 }
 * - `Spacing`: { 1, 1, 1 }
 * - `Center`: { 0, 0, 0 }
 * - `StandardDeviation`: 0.5
 * - `MaximumValue`: 255
 * - `Frequency`: { 60, 30, 40 }
 * - `Magnitude`: { 10, 18, 5 }
 */
class VISKORES_SOURCE_EXPORT Amr
{
public:
  VISKORES_CONT Amr() = default;

  VISKORES_CONT VISKORES_DEPRECATED(2.0, "Use Set* methods to set parameters.")
    Amr(viskores::IdComponent dimension,
        viskores::IdComponent cellsPerDimension = 6,
        viskores::IdComponent numberOfLevels = 4);

  VISKORES_CONT ~Amr() = default;

  VISKORES_CONT void SetDimension(viskores::IdComponent dimension) { this->Dimension = dimension; }
  VISKORES_CONT viskores::IdComponent GetDimension() const { return this->Dimension; }

  VISKORES_CONT void SetCellsPerDimension(viskores::IdComponent cellsPerDimension)
  {
    this->CellsPerDimension = cellsPerDimension;
  }
  VISKORES_CONT viskores::IdComponent GetCellsPerDimension() const
  {
    return this->CellsPerDimension;
  }

  VISKORES_CONT void SetNumberOfLevels(viskores::IdComponent numberOfLevels)
  {
    this->NumberOfLevels = numberOfLevels;
  }
  VISKORES_CONT viskores::IdComponent GetNumberOfLevels() const { return this->NumberOfLevels; }

  VISKORES_CONT viskores::cont::PartitionedDataSet Execute() const;

private:
  template <viskores::IdComponent Dim>
  viskores::cont::DataSet GenerateDataSet(unsigned int level, unsigned int amrIndex) const;

  viskores::IdComponent Dimension = 2;
  viskores::IdComponent CellsPerDimension = 6;
  viskores::IdComponent NumberOfLevels = 4;
};
} //namespace source
} //namespace viskores

#endif //viskores_source_Amr_h
