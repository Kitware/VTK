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

#ifndef viskores_filter_contour_AbstractContour_h
#define viskores_filter_contour_AbstractContour_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/contour/ContourDimension.h>
#include <viskores/filter/contour/viskores_filter_contour_export.h>
#include <viskores/filter/vector_analysis/SurfaceNormals.h>

namespace viskores
{
namespace filter
{
namespace contour
{
/// \brief Contour filter interface
///
/// Provides common configuration & execution methods for contour filters
/// Only the method `DoExecute` executing the contour algorithm needs to be implemented
class VISKORES_FILTER_CONTOUR_EXPORT AbstractContour : public viskores::filter::Filter
{
public:
  void SetNumberOfIsoValues(viskores::Id num)
  {
    if (num >= 0)
    {
      this->IsoValues.resize(static_cast<std::size_t>(num));
    }
  }

  viskores::Id GetNumberOfIsoValues() const
  {
    return static_cast<viskores::Id>(this->IsoValues.size());
  }

  /// @brief Set a field value on which to extract a contour.
  ///
  /// This form of the method is usually used when only one contour is being extracted.
  void SetIsoValue(viskores::Float64 v) { this->SetIsoValue(0, v); }

  /// @brief Set a field value on which to extract a contour.
  ///
  /// This form is used to specify multiple contours. The method is called
  /// multiple times with different @a index parameters.
  void SetIsoValue(viskores::Id index, viskores::Float64 v)
  {
    std::size_t i = static_cast<std::size_t>(index);
    if (i >= this->IsoValues.size())
    {
      this->IsoValues.resize(i + 1);
    }
    this->IsoValues[i] = v;
  }

  /// @brief Set multiple iso values at once.
  ///
  /// The iso values can be specified as either a `std::vector` or an initializer list.
  /// So, both
  ///
  /// @code{.cpp}
  /// std::vector<viskores::Float64> isovalues = { 0.2, 0.5, 0.7 };
  /// contour.SetIsoValues(isovalues);
  /// @endcode
  ///
  /// and
  ///
  /// @code{.cpp}
  /// contour.SetIsoValues({ 0.2, 0.5, 0.7 });
  /// @endcode
  ///
  /// work.
  void SetIsoValues(const std::vector<viskores::Float64>& values) { this->IsoValues = values; }

  /// @brief Return a value used to contour the mesh.
  viskores::Float64 GetIsoValue(viskores::Id index = 0) const
  {
    return this->IsoValues[static_cast<std::size_t>(index)];
  }

  /// @brief Set whether normals should be generated.
  ///
  /// Normals are used in shading calculations during rendering and can make the
  /// surface appear more smooth.
  ///
  /// Off by default.
  VISKORES_CONT
  void SetGenerateNormals(bool flag) { this->GenerateNormals = flag; }
  /// Get whether normals should be generated.
  VISKORES_CONT
  bool GetGenerateNormals() const { return this->GenerateNormals; }

  /// Set whether to append the ids of the intersected edges to the vertices of the isosurface
  /// triangles. Off by default.
  VISKORES_CONT
  void SetAddInterpolationEdgeIds(bool flag) { this->AddInterpolationEdgeIds = flag; }
  /// Get whether to append the ids of the intersected edges to the vertices of the isosurface
  /// triangles.
  VISKORES_CONT
  bool GetAddInterpolationEdgeIds() const { return this->AddInterpolationEdgeIds; }

  /// @brief Set whether the fast path should be used for normals computation.
  ///
  /// When this flag is off (the default), the generated normals are based on
  /// the gradient of the field being contoured and can be quite expensive to compute.
  /// When the flag is on, a faster method that computes the normals based on the faces
  /// of the isosurface mesh is used, but the normals do not look as good as the
  /// gradient based normals.
  ///
  /// This flag has no effect if `SetGenerateNormals` is false.
  VISKORES_CONT
  void SetComputeFastNormals(bool flag) { this->ComputeFastNormals = flag; }
  /// Get whether the fast path should be used for normals computation.
  VISKORES_CONT
  bool GetComputeFastNormals() const { return this->ComputeFastNormals; }

  /// Set the name of the field for the generated normals.
  VISKORES_CONT
  void SetNormalArrayName(const std::string& name) { this->NormalArrayName = name; }

  /// Get the name of the field for the generated normals.
  VISKORES_CONT
  const std::string& GetNormalArrayName() const { return this->NormalArrayName; }

  /// @brief Specify the dimension of cells on which to operate the contour.
  ///
  /// The contour filters operate on cells of a particular dimension
  /// (i.e., polyhedra, polygons, or lines) and generate simplicies
  /// of one less dimension (i.e., triangles, lines, or vertices).
  /// The default is `viskores::filter::contour::ContourDimension::Auto`.
  VISKORES_CONT void SetInputCellDimension(viskores::filter::contour::ContourDimension dimension)
  {
    this->InputCellDimension = dimension;
  }

  /// @copydoc SetInputCellDimension
  VISKORES_CONT viskores::filter::contour::ContourDimension GetInputCellDimension() const
  {
    return this->InputCellDimension;
  }

  /// @brief Specifies an automatic selection of the input cell dimension.
  ///
  /// This option first tries to contour polyhedra. If any polyhedra have the
  /// contour, that is used. Otherwise, it tries to contour polygons.
  /// If that fails, lines are contoured.
  VISKORES_CONT void SetInputCellDimensionToAuto()
  {
    this->SetInputCellDimension(viskores::filter::contour::ContourDimension::Auto);
  }

  /// @brief Specifies a combination of all possible contours.
  ///
  /// This option runs contour on all possible dimension types and then merges all contours together.
  VISKORES_CONT void SetInputCellDimensionToAll()
  {
    this->SetInputCellDimension(viskores::filter::contour::ContourDimension::All);
  }

  /// @brief Specifies running contours on polyhedra.
  ///
  /// This option runs contour on polyhedra, generating triangles.
  VISKORES_CONT void SetInputCellDimensionToPolyhedra()
  {
    this->SetInputCellDimension(viskores::filter::contour::ContourDimension::Polyhedra);
  }

  /// @brief Specifies running contours on polygons.
  ///
  /// This option runs contour on polygons, generating lines.
  VISKORES_CONT void SetInputCellDimensionToPolygons()
  {
    this->SetInputCellDimension(viskores::filter::contour::ContourDimension::Polygons);
  }

  /// @brief Specifies running contours on lines.
  ///
  /// This option runs contour on lines, generating vertices.
  VISKORES_CONT void SetInputCellDimensionToLines()
  {
    this->SetInputCellDimension(viskores::filter::contour::ContourDimension::Lines);
  }

  /// Set whether the points generated should be unique for every triangle
  /// or will duplicate points be merged together. Duplicate points are identified
  /// by the unique edge it was generated from.
  ///
  /// Because the contour filter (like all filters in Viskores) runs in parallel, parallel
  /// threads can (and often do) create duplicate versions of points. When this flag is
  /// set to true, a secondary operation will find all duplicated points and combine
  /// them together. If false, points will be duplicated. In addition to requiring more
  /// storage, duplicated points mean that triangles next to each other will not be
  /// considered adjecent to subsequent filters.
  ///
  VISKORES_CONT
  void SetMergeDuplicatePoints(bool on) { this->MergeDuplicatedPoints = on; }

  /// Get whether the points generated should be unique for every triangle
  /// or will duplicate points be merged together.
  VISKORES_CONT
  bool GetMergeDuplicatePoints() const { return this->MergeDuplicatedPoints; }

protected:
  /// \brief Map a given field to the output \c DataSet , depending on its type.
  ///
  /// The worklet needs to implement \c ProcessPointField to process point fields as arrays
  /// and \c GetCellIdMap function giving the cell id mapping from input to output
  template <typename WorkletType>
  VISKORES_CONT static bool DoMapField(viskores::cont::DataSet& result,
                                       const viskores::cont::Field& field,
                                       WorkletType& worklet)
  {
    if (field.IsPointField())
    {
      viskores::cont::UnknownArrayHandle inputArray = field.GetData();
      viskores::cont::UnknownArrayHandle outputArray = inputArray.NewInstanceBasic();

      auto functor = [&](const auto& concrete)
      {
        using ComponentType = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
        auto fieldArray = outputArray.ExtractArrayFromComponents<ComponentType>();
        worklet.ProcessPointField(concrete, fieldArray);
      };
      inputArray.CastAndCallWithExtractedArray(functor);
      result.AddPointField(field.GetName(), outputArray);
      return true;
    }
    else if (field.IsCellField())
    {
      // Use the precompiled field permutation function.
      viskores::cont::ArrayHandle<viskores::Id> permutation = worklet.GetCellIdMap();
      return viskores::filter::MapFieldPermutation(field, permutation, result);
    }
    else if (field.IsWholeDataSetField())
    {
      result.AddField(field);
      return true;
    }
    return false;
  }

  VISKORES_CONT void ExecuteGenerateNormals(
    viskores::cont::DataSet& output,
    const viskores::cont::ArrayHandle<viskores::Vec3f>& normals)
  {
    if (this->GenerateNormals)
    {
      if (this->GetComputeFastNormals())
      {
        viskores::filter::vector_analysis::SurfaceNormals surfaceNormals;
        surfaceNormals.SetPointNormalsName(this->NormalArrayName);
        surfaceNormals.SetGeneratePointNormals(true);
        output = surfaceNormals.Execute(output);
      }
      else
      {
        output.AddField(viskores::cont::make_FieldPoint(this->NormalArrayName, normals));
      }
    }
  }

  template <typename WorkletType>
  VISKORES_CONT void ExecuteAddInterpolationEdgeIds(viskores::cont::DataSet& output,
                                                    WorkletType& worklet)
  {
    if (this->AddInterpolationEdgeIds)
    {
      viskores::cont::Field interpolationEdgeIdsField(this->InterpolationEdgeIdsArrayName,
                                                      viskores::cont::Field::Association::Points,
                                                      worklet.GetInterpolationEdgeIds());
      output.AddField(interpolationEdgeIdsField);
    }
  }

  VISKORES_CONT
  virtual viskores::cont::DataSet DoExecute(
    const viskores::cont::DataSet& result) = 0; // Needs to be overridden by contour implementations

  std::vector<viskores::Float64> IsoValues;
  bool GenerateNormals = true;
  bool ComputeFastNormals = false;

  viskores::filter::contour::ContourDimension InputCellDimension =
    viskores::filter::contour::ContourDimension::Auto;

  bool AddInterpolationEdgeIds = false;
  bool MergeDuplicatedPoints = true;
  std::string NormalArrayName = "normals";
  std::string InterpolationEdgeIdsArrayName = "edgeIds";
};
} // namespace contour
} // namespace filter
} // namespace viskores

#endif // viskores_filter_contour_AbstractContour_h
