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

#ifndef viskores_filter_vector_analysis_Gradient_h
#define viskores_filter_vector_analysis_Gradient_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/vector_analysis/viskores_filter_vector_analysis_export.h>

namespace viskores
{
namespace filter
{
namespace vector_analysis
{

/// @brief A general filter for gradient estimation.
///
/// Estimates the gradient of a point field in a data set. The created gradient array
/// can be determined at either each point location or at the center of each cell.
///
/// The default for the filter is output as cell centered gradients.
/// To enable point based gradient computation enable `SetComputePointGradient()`
///
/// If no explicit name for the output field is provided the filter will
/// default to "Gradients"
class VISKORES_FILTER_VECTOR_ANALYSIS_EXPORT Gradient : public viskores::filter::Filter
{
public:
  /// @brief Specify whether to compute gradients
  ///
  /// When this flag is on (default is off), the gradient filter will provide a
  /// point based gradients, which are significantly more costly since for each
  /// point we need to compute the gradient of each cell that uses it.
  void SetComputePointGradient(bool enable) { ComputePointGradient = enable; }
  /// @copydoc SetComputePointGradient
  bool GetComputePointGradient() const { return ComputePointGradient; }

  /// Add divergence field to the output data. The input array must have 3 components
  /// to compute this. The default is off.
  void SetComputeDivergence(bool enable) { ComputeDivergence = enable; }
  /// @copydoc SetComputeDivergence
  bool GetComputeDivergence() const { return ComputeDivergence; }

  /// When `SetComputeDivergence()` is enabled, the result is stored in a field
  /// of this name. If not specified, the name of the field will be `Divergence`.
  void SetDivergenceName(const std::string& name) { this->DivergenceName = name; }
  /// @copydoc SetDivergenceName
  const std::string& GetDivergenceName() const { return this->DivergenceName; }

  /// Add voriticity/curl field to the output data. The input array must have 3 components
  /// to compute this. The default is off.
  void SetComputeVorticity(bool enable) { ComputeVorticity = enable; }
  /// @copydoc SetComputeVorticity
  bool GetComputeVorticity() const { return ComputeVorticity; }

  /// When `SetComputeVorticity()` is enabled, the result is stored in a field
  /// of this name. If not specified, the name of the field will be `Vorticity`.
  void SetVorticityName(const std::string& name) { this->VorticityName = name; }
  /// @copydoc SetVorticityName
  const std::string& GetVorticityName() const { return this->VorticityName; }

  /// Add Q-criterion field to the output data. The input array must have 3 components
  /// to compute this. The default is off.
  void SetComputeQCriterion(bool enable) { ComputeQCriterion = enable; }
  /// @copydoc SetComputeQCriterion
  bool GetComputeQCriterion() const { return ComputeQCriterion; }

  /// When `SetComputeQCriterion()` is enabled, the result is stored in a field
  /// of this name. If not specified, the name of the field will be `QCriterion`.
  void SetQCriterionName(const std::string& name) { this->QCriterionName = name; }
  /// @copydoc SetQCriterionName
  const std::string& GetQCriterionName() const { return this->QCriterionName; }

  /// Add gradient field to the output data.  The name of the array
  /// will be `Gradients` unless otherwise specified with `SetOutputFieldName`
  /// and will be a cell field unless `ComputePointGradient()`
  /// is enabled. It is useful to turn this off when you are only interested
  /// in the results of Divergence, Vorticity, or QCriterion. The default is on.
  void SetComputeGradient(bool enable) { StoreGradient = enable; }
  /// @copydoc SetComputeGradient
  bool GetComputeGradient() const { return StoreGradient; }

  /// Make the vector gradient output format be in FORTRAN Column-major order.
  /// This is only used when the input field is a vector field.
  /// Enabling column-major is important if integrating with other projects
  /// such as VTK.
  /// Default: Row Order.
  void SetColumnMajorOrdering() { RowOrdering = false; }

  /// Make the vector gradient output format be in C Row-major order.
  /// This is only used when the input field is a vector field.
  /// Default: Row Order.
  void SetRowMajorOrdering() { RowOrdering = true; }

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& inputDataSet) override;

  bool ComputePointGradient = false;
  bool ComputeDivergence = false;
  bool ComputeVorticity = false;
  bool ComputeQCriterion = false;
  bool StoreGradient = true;
  bool RowOrdering = true;

  std::string DivergenceName = "Divergence";
  std::string GradientsName = "Gradients";
  std::string QCriterionName = "QCriterion";
  std::string VorticityName = "Vorticity";
};

} // namespace vector_analysis
} // namespace filter
} // namespace viskores::filter

#endif // viskores_filter_vector_analysis_Gradient_h
