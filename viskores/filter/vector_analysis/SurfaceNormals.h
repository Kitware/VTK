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
#ifndef viskores_filter_vector_analysis_SurfaceNormal_h
#define viskores_filter_vector_analysis_SurfaceNormal_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/vector_analysis/viskores_filter_vector_analysis_export.h>

namespace viskores
{
namespace filter
{
namespace vector_analysis
{
/// \brief Computes normals for polygonal mesh.
///
/// This filter computes surface normals on points and/or cells of a polygonal dataset.
/// The cell normals are faceted and are computed based on the plane where a
/// face lies. The point normals are smooth normals, computed by averaging
/// the face normals of incident cells. The normals will be consistently oriented to
/// point in the direction of the same connected surface if possible.
///
/// The point and cell normals may be oriented to a point outside of the manifold surface
/// by turning on the auto orient normals option (`SetAutoOrientNormals()`), or they may
/// point inward by also setting flip normals (`SetFlipNormals()`) to true.
///
/// Triangle vertices will be reordered to be wound counter-clockwise around the cell
/// normals when the consistency option (`SetConsistency()`) is enabled.
///
/// For non-polygonal cells, a zeroed vector is assigned. The point normals are computed
/// by averaging the cell normals of the incident cells of each point.
///
/// The default name for the output fields is `Normals`, but that can be overridden using
/// the `SetCellNormalsName()` and `SetPointNormalsName()` methods. The filter will also
/// respect the name in `SetOutputFieldName()` if neither of the others are set.
///
class VISKORES_FILTER_VECTOR_ANALYSIS_EXPORT SurfaceNormals : public viskores::filter::Filter
{
public:
  /// Create SurfaceNormals filter. This calls
  /// this->SetUseCoordinateSystemAsField(true) since that is the most common
  /// use-case for surface normals.
  SurfaceNormals();

  /// @brief Specify whether cell normals should be generated.
  ///
  /// Default is off.
  void SetGenerateCellNormals(bool value) { this->GenerateCellNormals = value; }
  /// @copydoc SetGenerateCellNormals
  bool GetGenerateCellNormals() const { return this->GenerateCellNormals; }

  /// @brief Specify whether the cell normals should be normalized.
  ///
  /// Default value is true.
  /// The intended use case of this flag is for faster, approximate point
  /// normals generation by skipping the normalization of the face normals.
  /// Note that when set to false, the result cell normals will not be unit
  /// length normals and the point normals will be different.
  void SetNormalizeCellNormals(bool value) { this->NormalizeCellNormals = value; }
  /// @copydoc SetNormalizeCellNormals
  bool GetNormalizeCellNormals() const { return this->NormalizeCellNormals; }

  /// @brief Specify whether the point normals should be generated.
  ///
  /// Default is on.
  void SetGeneratePointNormals(bool value) { this->GeneratePointNormals = value; }
  /// @copydoc SetGeneratePointNormals
  bool GetGeneratePointNormals() const { return this->GeneratePointNormals; }

  /// @brief Specify the name of the cell normals field.
  ///
  /// Default is `Normals`.
  void SetCellNormalsName(const std::string& name) { this->CellNormalsName = name; }
  /// @copydoc SetCellNormalsName
  const std::string& GetCellNormalsName() const { return this->CellNormalsName; }

  /// @brief Specify the name of the point normals field.
  ///
  /// Default is `Normals`.
  void SetPointNormalsName(const std::string& name) { this->PointNormalsName = name; }
  /// @copydoc SetPointNormalsName
  const std::string& GetPointNormalsName() const { return this->PointNormalsName; }

  /// @brief Specify whether to orient the normals outwards from the surface.
  ///
  /// This requires a closed manifold surface or the behavior is undefined.
  /// This option is expensive but might be necessary for rendering.
  /// To make the normals point inward, set FlipNormals to true.
  /// Default is off.
  void SetAutoOrientNormals(bool v) { this->AutoOrientNormals = v; }
  /// @copydoc SetAutoOrientNormals
  bool GetAutoOrientNormals() const { return this->AutoOrientNormals; }

  /// @brief Specify the direction to point normals when `SetAutoOrientNormals()` is true.
  ///
  /// When this flag is false (the default), the normals will be oriented to point outward.
  /// When the flag is true, the normals will point inward.
  /// This option has no effect if auto orient normals is off.
  void SetFlipNormals(bool v) { this->FlipNormals = v; }
  /// @copydoc SetFlipNormals
  bool GetFlipNormals() const { return this->FlipNormals; }

  /// @brief Specify whtehr polygon winding should be made consistent with normal orientation.
  ///
  /// Triangles are wound such that their points are counter-clockwise around
  /// the generated cell normal. Default is true.
  /// This currently only affects triangles.
  /// This is only applied when cell normals are generated.
  void SetConsistency(bool v) { this->Consistency = v; }
  /// @copydoc SetConsistency
  bool GetConsistency() const { return this->Consistency; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(
    const viskores::cont::DataSet& inputDataSet) override;

  bool GenerateCellNormals = false;
  bool NormalizeCellNormals = true;
  bool GeneratePointNormals = true;
  bool AutoOrientNormals = false;
  bool FlipNormals = false;
  bool Consistency = true;

  std::string CellNormalsName;
  std::string PointNormalsName;
};
} // namespace vector_analysis
} // namespace filter
} // namespace viskores

#endif // viskores_filter_vector_analysis_SurfaceNormal_h
