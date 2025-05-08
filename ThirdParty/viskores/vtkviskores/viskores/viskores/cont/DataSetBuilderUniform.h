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
#ifndef viskores_cont_DataSetBuilderUniform_h
#define viskores_cont_DataSetBuilderUniform_h

#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>

namespace viskores
{
namespace cont
{

class VISKORES_CONT_EXPORT DataSetBuilderUniform
{
  using VecType = viskores::Vec3f;

public:
  VISKORES_CONT
  DataSetBuilderUniform();

  /// @brief Create a 1D uniform `DataSet`.
  ///
  /// @param[in] dimension The size of the grid. The dimensions are specified
  ///   based on the number of points (as opposed to the number of cells).
  /// @param[in] origin The origin of the data. This is the point coordinate with
  ///   the minimum value in all dimensions.
  /// @param[in] spacing The uniform distance between adjacent points.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(const viskores::Id& dimension,
                                                      const T& origin,
                                                      const T& spacing,
                                                      const std::string& coordNm = "coords")
  {
    return DataSetBuilderUniform::CreateDataSet(
      viskores::Id3(dimension, 1, 1),
      VecType(static_cast<viskores::FloatDefault>(origin), 0, 0),
      VecType(static_cast<viskores::FloatDefault>(spacing), 1, 1),
      coordNm);
  }

  /// @brief Create a 1D uniform `DataSet`.
  ///
  /// The origin is set to 0 and the spacing is set to 1.
  ///
  /// @param[in] dimension The size of the grid. The dimensions are specified
  ///   based on the number of points (as opposed to the number of cells).
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  VISKORES_CONT static viskores::cont::DataSet Create(const viskores::Id& dimension,
                                                      const std::string& coordNm = "coords");

  /// @brief Create a 2D uniform `DataSet`.
  ///
  /// @param[in] dimensions The size of the grid. The dimensions are specified
  ///   based on the number of points (as opposed to the number of cells).
  /// @param[in] origin The origin of the data. This is the point coordinate with
  ///   the minimum value in all dimensions.
  /// @param[in] spacing The uniform distance between adjacent points.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(const viskores::Id2& dimensions,
                                                      const viskores::Vec<T, 2>& origin,
                                                      const viskores::Vec<T, 2>& spacing,
                                                      const std::string& coordNm = "coords")
  {
    return DataSetBuilderUniform::CreateDataSet(
      viskores::Id3(dimensions[0], dimensions[1], 1),
      VecType(static_cast<viskores::FloatDefault>(origin[0]),
              static_cast<viskores::FloatDefault>(origin[1]),
              0),
      VecType(static_cast<viskores::FloatDefault>(spacing[0]),
              static_cast<viskores::FloatDefault>(spacing[1]),
              1),
      coordNm);
  }

  /// @brief Create a 2D uniform `DataSet`.
  ///
  /// The origin is set to (0,0) and the spacing is set to (1,1).
  ///
  /// @param[in] dimensions The size of the grid. The dimensions are specified
  ///   based on the number of points (as opposed to the number of cells).
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  VISKORES_CONT static viskores::cont::DataSet Create(const viskores::Id2& dimensions,
                                                      const std::string& coordNm = "coords");

  /// @brief Create a 3D uniform `DataSet`.
  ///
  /// @param[in] dimensions The size of the grid. The dimensions are specified
  ///   based on the number of points (as opposed to the number of cells).
  /// @param[in] origin The origin of the data. This is the point coordinate with
  ///   the minimum value in all dimensions.
  /// @param[in] spacing The uniform distance between adjacent points.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(const viskores::Id3& dimensions,
                                                      const viskores::Vec<T, 3>& origin,
                                                      const viskores::Vec<T, 3>& spacing,
                                                      const std::string& coordNm = "coords")
  {
    return DataSetBuilderUniform::CreateDataSet(
      viskores::Id3(dimensions[0], dimensions[1], dimensions[2]),
      VecType(static_cast<viskores::FloatDefault>(origin[0]),
              static_cast<viskores::FloatDefault>(origin[1]),
              static_cast<viskores::FloatDefault>(origin[2])),
      VecType(static_cast<viskores::FloatDefault>(spacing[0]),
              static_cast<viskores::FloatDefault>(spacing[1]),
              static_cast<viskores::FloatDefault>(spacing[2])),
      coordNm);
  }

  /// @brief Create a 3D uniform `DataSet`.
  ///
  /// The origin is set to (0,0,0) and the spacing is set to (1,1,1).
  ///
  /// @param[in] dimensions The size of the grid. The dimensions are specified
  ///   based on the number of points (as opposed to the number of cells).
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  VISKORES_CONT static viskores::cont::DataSet Create(const viskores::Id3& dimensions,
                                                      const std::string& coordNm = "coords");

private:
  VISKORES_CONT
  static viskores::cont::DataSet CreateDataSet(const viskores::Id3& dimensions,
                                               const viskores::Vec3f& origin,
                                               const viskores::Vec3f& spacing,
                                               const std::string& coordNm);
};

} // namespace cont
} // namespace viskores

#endif //viskores_cont_DataSetBuilderUniform_h
