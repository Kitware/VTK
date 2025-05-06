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

#ifndef viskores_filter_mesh_info_CellMeasures_h
#define viskores_filter_mesh_info_CellMeasures_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/mesh_info/viskores_filter_mesh_info_export.h>

#include <viskores/Deprecated.h>

namespace viskores
{
namespace filter
{
namespace mesh_info
{

/// \brief Specifies over what types of mesh elements CellMeasures will operate.
///
/// The values of `IntegrationType` may be `|`-ed together to select multiple
enum struct IntegrationType
{
  None = 0x00,
  /// @copydoc CellMeasures::SetMeasureToArcLength
  ArcLength = 0x01,
  /// @copydoc CellMeasures::SetMeasureToArea
  Area = 0x02,
  /// @copydoc CellMeasures::SetMeasureToVolume
  Volume = 0x04,
  /// @copydoc CellMeasures::SetMeasureToAll
  AllMeasures = ArcLength | Area | Volume
};

VISKORES_EXEC_CONT inline IntegrationType operator&(IntegrationType left, IntegrationType right)
{
  return static_cast<IntegrationType>(static_cast<int>(left) & static_cast<int>(right));
}
VISKORES_EXEC_CONT inline IntegrationType operator|(IntegrationType left, IntegrationType right)
{
  return static_cast<IntegrationType>(static_cast<int>(left) | static_cast<int>(right));
}

/// @brief Compute the size measure of each cell in a dataset.
///
/// CellMeasures is a filter that generates a new cell data array (i.e., one value
/// specified per cell) holding the signed measure of the cell
/// or 0 (if measure is not well defined or the cell type is unsupported).
///
/// By default, the new cell-data array is named "measure".
class VISKORES_FILTER_MESH_INFO_EXPORT CellMeasures : public viskores::filter::Filter
{
public:
  VISKORES_CONT CellMeasures();

  VISKORES_DEPRECATED(2.2, "Use default constructor and `SetIntegrationType`.")
  VISKORES_CONT explicit CellMeasures(IntegrationType);

  /// @brief Specify the type of integrations to support.
  ///
  /// This filter can support integrating the size of 1D elements (arclength measurements),
  /// 2D elements (area measurements), and 3D elements (volume measurements). The measures to
  /// perform are specified with a `viskores::filter::mesh_info::IntegrationType`.
  ///
  /// By default, the size measure for all types of elements is performed.
  VISKORES_CONT void SetMeasure(viskores::filter::mesh_info::IntegrationType measure)
  {
    this->Measure = measure;
  }
  /// @copydoc SetMeasure
  VISKORES_CONT viskores::filter::mesh_info::IntegrationType GetMeasure() const
  {
    return this->Measure;
  }
  /// @brief Compute the length of 1D elements.
  VISKORES_CONT void SetMeasureToArcLength()
  {
    this->SetMeasure(viskores::filter::mesh_info::IntegrationType::ArcLength);
  }
  /// @brief Compute the area of 2D elements.
  VISKORES_CONT void SetMeasureToArea()
  {
    this->SetMeasure(viskores::filter::mesh_info::IntegrationType::Area);
  }
  /// @brief Compute the volume of 3D elements.
  VISKORES_CONT void SetMeasureToVolume()
  {
    this->SetMeasure(viskores::filter::mesh_info::IntegrationType::Volume);
  }
  /// @brief Compute the size of all types of elements.
  VISKORES_CONT void SetMeasureToAll()
  {
    this->SetMeasure(viskores::filter::mesh_info::IntegrationType::AllMeasures);
  }

  /// @brief Specify the name of the field generated.
  ///
  /// If not set, `measure` is used.
  VISKORES_CONT void SetCellMeasureName(const std::string& name) { this->SetOutputFieldName(name); }
  /// @copydoc SetCellMeasureName
  VISKORES_CONT const std::string& GetCellMeasureName() const { return this->GetOutputFieldName(); }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  IntegrationType Measure = viskores::filter::mesh_info::IntegrationType::AllMeasures;
};
} // namespace mesh_info
} // namespace filter
} // namespace viskores

#endif // viskores_filter_mesh_info_CellMeasures_h
