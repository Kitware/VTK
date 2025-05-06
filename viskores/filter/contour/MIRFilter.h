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

#ifndef viskores_m_filter_contour_MIRFilter_h
#define viskores_m_filter_contour_MIRFilter_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/contour/viskores_filter_contour_export.h>

namespace viskores
{
namespace filter
{
namespace contour
{
/// @brief Calculates and subdivides a mesh based on the material interface reconstruction algorithm.
///
/// Subdivides a mesh given volume fraction information for each _cell_. It does this by applying a
///     mixture of the painters algorithm and isosurfacing algorithm. This filter will return
///     a dataset where cells are subdivided into new cells of a certain "Material", and fields passed
///     will do 1 of 3 things:
///     1) They will not pass if they are an array associated with the whole mesh,
///     2) They will simply be passed to new cells if the array is associated with the cell set
///     3) They will be interpolated to new point locations if the array is associated with the point set
///
/// This algorithm requires passing a cell set of volume fraction information, not a point cell set.
///     The exact fields are required:
///     1) A length cell set that specifies the number of materials associated to the cell.
///     2) A position cell set (or offset cell set) that specifies where the material IDs and VFs occur in the ID and VF arrays.
///     3) An ID array (whole array set) that stores the material ID information
///     4) An VF array (whole array set) that stores the fractional volume information for the respective material ID.
///     Note that the cell VF information should add up to 1.0 across all materials for the cell, however this isn't checked in the code and might
///     lead to undesirable results when iterating.
///
/// Note that this algorithm does not guarantee that the newly constructed cells will match the provided
///     volume fractions, nor does it guarantee that there will exist a subcell of every material ID from the original cell.
///     This usually occurs when the resolution of the mesh is too low (isolated materials in a single cell).
///
/// If wanted, this algorithm can iterate, adjusting cell VFs based on distance from the target values and the previous calculated iteration.
///     This is done by setting the max iterations >0. In addition, the max percent error will allow for the filter to return early if the
///     total error % of the entire dataset is less than the specified amount (defaults to 1.0, returns after first iteration). Finally,
///     the error scaling and scaling decay allows for setting how much the cell VFs should react to the delta between target and calculated cell VFs.
///     the error scaling will decay by the decay variable every iteration (multiplicitively).
class VISKORES_FILTER_CONTOUR_EXPORT MIRFilter : public viskores::filter::Filter
{
public:
  /// @brief Sets the name of the offset/position cellset field in the dataset passed to the filter
  VISKORES_CONT void SetPositionCellSetName(std::string name) { this->pos_name = name; }
  /// @brief Sets the name of the length cellset field in the dataset passed to the filter
  VISKORES_CONT void SetLengthCellSetName(std::string name) { this->len_name = name; }
  /// @brief Sets the name of the ID whole-array set field in the dataset passed to the filter
  VISKORES_CONT void SetIDWholeSetName(std::string name) { this->id_name = name; }
  /// @brief Sets the name of the VF whole-array set field in the dataset passed to the filter
  VISKORES_CONT void SetVFWholeSetName(std::string name) { this->vf_name = name; }
  VISKORES_CONT void SetMaxPercentError(viskores::Float64 ma) { this->max_error = ma; }
  VISKORES_CONT void SetMaxIterations(viskores::IdComponent ma) { this->max_iter = ma; }
  VISKORES_CONT void SetErrorScaling(viskores::Float64 sc) { this->error_scaling = sc; }
  VISKORES_CONT void SetScalingDecay(viskores::Float64 sc) { this->scaling_decay = sc; }
  /// @brief Gets the output cell-set field name for the filter
  VISKORES_CONT std::string GetOutputFieldName() { return this->OutputFieldName; }
  /// @brief Sets the output cell-set field name for the filter
  VISKORES_CONT void SetOutputFieldName(std::string name) { this->OutputFieldName = name; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
  VISKORES_CONT bool DoMapField(
    viskores::cont::DataSet& result,
    const viskores::cont::Field& field,
    const viskores::cont::ArrayHandle<viskores::Id>& filterCellInterp,
    const viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 8>>& MIRWeights,
    const viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 8>> MIRIDs);

  std::string pos_name;
  std::string len_name;
  std::string id_name;
  std::string vf_name;
  std::string OutputFieldName = std::string("cellMat");
  viskores::Float64 max_error = viskores::Float64(1.0);
  viskores::Float64 scaling_decay = viskores::Float64(1.0);
  viskores::IdComponent max_iter = viskores::IdComponent(0);
  viskores::Float64 error_scaling = viskores::Float64(0.0);
};
} // namespace contour
} // namespace filter
} // namespace viskores

#endif // viskores_m_filter_contour_MIRFilter_h
