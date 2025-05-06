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
#ifndef viskores_cont_DataSetBuilderExplicit_h
#define viskores_cont_DataSetBuilderExplicit_h

#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>

namespace viskores
{
namespace cont
{

//Coordinates builder??

class VISKORES_CONT_EXPORT DataSetBuilderExplicit
{
public:
  VISKORES_CONT
  DataSetBuilderExplicit() {}

  /// \brief Create a 1D `DataSet` with arbitrary cell connectivity.
  ///
  /// The cell connectivity is specified with arrays defining the shape and point
  /// connections of each cell.
  /// In this form, the cell connectivity and coordinates are specified as `std::vector`
  /// and the data will be copied to create the data object.
  ///
  /// @param[in] xVals An array providing the x coordinate of each point.
  /// @param[in] shapes An array of shapes for each cell. Each entry should be one of the
  ///   `viskores::CELL_SHAPE_*` values identifying the shape of the corresponding cell.
  /// @param[in] numIndices An array containing for each cell the number of points incident
  ///   on that cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   the @a numIndices array. These variable length arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(
    const std::vector<T>& xVals,
    const std::vector<viskores::UInt8>& shapes,
    const std::vector<viskores::IdComponent>& numIndices,
    const std::vector<viskores::Id>& connectivity,
    const std::string& coordsNm = "coords")
  {
    std::vector<T> yVals(xVals.size(), 0), zVals(xVals.size(), 0);
    return DataSetBuilderExplicit::Create(
      xVals, yVals, zVals, shapes, numIndices, connectivity, coordsNm);
  }

  /// \brief Create a 2D `DataSet` with arbitrary cell connectivity.
  ///
  /// The cell connectivity is specified with arrays defining the shape and point
  /// connections of each cell.
  /// In this form, the cell connectivity and coordinates are specified as `std::vector`
  /// and the data will be copied to create the data object.
  ///
  /// @param[in] xVals An array providing the x coordinate of each point.
  /// @param[in] yVals An array providing the x coordinate of each point.
  /// @param[in] shapes An array of shapes for each cell. Each entry should be one of the
  ///   `viskores::CELL_SHAPE_*` values identifying the shape of the corresponding cell.
  /// @param[in] numIndices An array containing for each cell the number of points incident
  ///   on that cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   the @a numIndices array. These variable length arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(
    const std::vector<T>& xVals,
    const std::vector<T>& yVals,
    const std::vector<viskores::UInt8>& shapes,
    const std::vector<viskores::IdComponent>& numIndices,
    const std::vector<viskores::Id>& connectivity,
    const std::string& coordsNm = "coords")
  {
    std::vector<T> zVals(xVals.size(), 0);
    return DataSetBuilderExplicit::Create(
      xVals, yVals, zVals, shapes, numIndices, connectivity, coordsNm);
  }

  /// \brief Create a 3D `DataSet` with arbitrary cell connectivity.
  ///
  /// The cell connectivity is specified with arrays defining the shape and point
  /// connections of each cell.
  /// In this form, the cell connectivity and coordinates are specified as `std::vector`
  /// and the data will be copied to create the data object.
  ///
  /// @param[in] xVals An array providing the x coordinate of each point.
  /// @param[in] yVals An array providing the x coordinate of each point.
  /// @param[in] zVals An array providing the x coordinate of each point.
  /// @param[in] shapes An array of shapes for each cell. Each entry should be one of the
  ///   `viskores::CELL_SHAPE_*` values identifying the shape of the corresponding cell.
  /// @param[in] numIndices An array containing for each cell the number of points incident
  ///   on that cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   the @a numIndices array. These variable length arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(
    const std::vector<T>& xVals,
    const std::vector<T>& yVals,
    const std::vector<T>& zVals,
    const std::vector<viskores::UInt8>& shapes,
    const std::vector<viskores::IdComponent>& numIndices,
    const std::vector<viskores::Id>& connectivity,
    const std::string& coordsNm = "coords");

  /// \brief Create a 3D `DataSet` with arbitrary cell connectivity.
  ///
  /// The cell connectivity is specified with arrays defining the shape and point
  /// connections of each cell.
  /// In this form, the cell connectivity and coordinates are specified as `std::vector`
  /// and the data will be copied to create the data object.
  ///
  /// @param[in] coords An array of point coordinates.
  /// @param[in] shapes An array of shapes for each cell. Each entry should be one of the
  ///   `viskores::CELL_SHAPE_*` values identifying the shape of the corresponding cell.
  /// @param[in] numIndices An array containing for each cell the number of points incident
  ///   on that cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   the @a numIndices array. These variable length arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(
    const std::vector<viskores::Vec<T, 3>>& coords,
    const std::vector<viskores::UInt8>& shapes,
    const std::vector<viskores::IdComponent>& numIndices,
    const std::vector<viskores::Id>& connectivity,
    const std::string& coordsNm = "coords");

  /// \brief Create a 3D `DataSet` with arbitrary cell connectivity.
  ///
  /// The cell connectivity is specified with arrays defining the shape and point
  /// connections of each cell.
  /// In this form, the cell connectivity and coordinates are specified as `ArrayHandle`
  /// and the memory will be shared with the created data object. That said, the `DataSet`
  /// construction will generate a new array for offsets.
  ///
  /// @param[in] coords An array of point coordinates.
  /// @param[in] shapes An array of shapes for each cell. Each entry should be one of the
  ///   `viskores::CELL_SHAPE_*` values identifying the shape of the corresponding cell.
  /// @param[in] numIndices An array containing for each cell the number of points incident
  ///   on that cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   the @a numIndices array. These variable length arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(
    const viskores::cont::ArrayHandle<viskores::Vec<T, 3>>& coords,
    const viskores::cont::ArrayHandle<viskores::UInt8>& shapes,
    const viskores::cont::ArrayHandle<viskores::IdComponent>& numIndices,
    const viskores::cont::ArrayHandle<viskores::Id>& connectivity,
    const std::string& coordsNm = "coords")
  {
    auto offsets = viskores::cont::ConvertNumComponentsToOffsets(numIndices);
    return DataSetBuilderExplicit::BuildDataSet(coords, shapes, offsets, connectivity, coordsNm);
  }

  /// \brief Create a 3D `DataSet` with arbitrary cell connectivity for a single cell type.
  ///
  /// The cell connectivity is specified with an array defining the point
  /// connections of each cell.
  /// All the cells in the `DataSet` are of the same shape and contain the same number
  /// of incident points.
  /// In this form, the cell connectivity and coordinates are specified as `std::vector`
  /// and the data will be copied to create the data object.
  ///
  /// @param[in] coords An array of point coordinates.
  /// @param[in] tag A tag object representing the shape of all the cells in the mesh.
  ///   Cell shape tag objects have a name of the form `viskores::CellShapeTag*` such as
  ///   `viskores::CellShapeTagTriangle` or `viskores::CellShapeTagHexahedron`. To specify a
  ///   cell shape determined at runtime, use `viskores::CellShapeTagGeneric`.
  /// @param[in] numberOfPointsPerCell The number of points that are incident to each cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   @a numberOfPointsPerCell. These short arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T, typename CellShapeTag>
  VISKORES_CONT static viskores::cont::DataSet Create(
    const std::vector<viskores::Vec<T, 3>>& coords,
    CellShapeTag tag,
    viskores::IdComponent numberOfPointsPerCell,
    const std::vector<viskores::Id>& connectivity,
    const std::string& coordsNm = "coords");

  /// \brief Create a 3D `DataSet` with arbitrary cell connectivity for a single cell type.
  ///
  /// The cell connectivity is specified with an array defining the point
  /// connections of each cell.
  /// All the cells in the `DataSet` are of the same shape and contain the same number
  /// of incident points.
  /// In this form, the cell connectivity and coordinates are specified as `ArrayHandle`
  /// and the memory will be shared with the created data object.
  ///
  /// @param[in] coords An array of point coordinates.
  /// @param[in] tag A tag object representing the shape of all the cells in the mesh.
  ///   Cell shape tag objects have a name of the form `viskores::CellShapeTag*` such as
  ///   `viskores::CellShapeTagTriangle` or `viskores::CellShapeTagHexahedron`. To specify a
  ///   cell shape determined at runtime, use `viskores::CellShapeTagGeneric`.
  /// @param[in] numberOfPointsPerCell The number of points that are incident to each cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   @a numberOfPointsPerCell. These short arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T, typename CellShapeTag>
  VISKORES_CONT static viskores::cont::DataSet Create(
    const viskores::cont::ArrayHandle<viskores::Vec<T, 3>>& coords,
    CellShapeTag tag,
    viskores::IdComponent numberOfPointsPerCell,
    const viskores::cont::ArrayHandle<viskores::Id>& connectivity,
    const std::string& coordsNm = "coords")
  {
    return DataSetBuilderExplicit::BuildDataSet(
      coords, tag, numberOfPointsPerCell, connectivity, coordsNm);
  }

private:
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet BuildDataSet(
    const viskores::cont::ArrayHandle<viskores::Vec<T, 3>>& coords,
    const viskores::cont::ArrayHandle<viskores::UInt8>& shapes,
    const viskores::cont::ArrayHandle<viskores::Id>& offsets,
    const viskores::cont::ArrayHandle<viskores::Id>& connectivity,
    const std::string& coordsNm);

  template <typename T, typename CellShapeTag>
  VISKORES_CONT static viskores::cont::DataSet BuildDataSet(
    const viskores::cont::ArrayHandle<viskores::Vec<T, 3>>& coords,
    CellShapeTag tag,
    viskores::IdComponent numberOfPointsPerCell,
    const viskores::cont::ArrayHandle<viskores::Id>& connectivity,
    const std::string& coordsNm);
};

template <typename T>
inline VISKORES_CONT viskores::cont::DataSet DataSetBuilderExplicit::Create(
  const std::vector<T>& xVals,
  const std::vector<T>& yVals,
  const std::vector<T>& zVals,
  const std::vector<viskores::UInt8>& shapes,
  const std::vector<viskores::IdComponent>& numIndices,
  const std::vector<viskores::Id>& connectivity,
  const std::string& coordsNm)
{
  VISKORES_ASSERT(xVals.size() == yVals.size() && yVals.size() == zVals.size() && xVals.size() > 0);

  viskores::cont::ArrayHandle<viskores::Vec3f> coordsArray;
  coordsArray.Allocate(static_cast<viskores::Id>(xVals.size()));
  auto coordsPortal = coordsArray.WritePortal();
  for (std::size_t index = 0; index < xVals.size(); ++index)
  {
    coordsPortal.Set(static_cast<viskores::Id>(index),
                     viskores::make_Vec(static_cast<viskores::FloatDefault>(xVals[index]),
                                        static_cast<viskores::FloatDefault>(yVals[index]),
                                        static_cast<viskores::FloatDefault>(zVals[index])));
  }

  auto shapesArray = viskores::cont::make_ArrayHandle(shapes, viskores::CopyFlag::On);
  auto connArray = viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On);

  auto offsetsArray = viskores::cont::ConvertNumComponentsToOffsets(
    viskores::cont::make_ArrayHandle(numIndices, viskores::CopyFlag::Off));

  return DataSetBuilderExplicit::BuildDataSet(
    coordsArray, shapesArray, offsetsArray, connArray, coordsNm);
}

template <typename T>
inline VISKORES_CONT viskores::cont::DataSet DataSetBuilderExplicit::Create(
  const std::vector<viskores::Vec<T, 3>>& coords,
  const std::vector<viskores::UInt8>& shapes,
  const std::vector<viskores::IdComponent>& numIndices,
  const std::vector<viskores::Id>& connectivity,
  const std::string& coordsNm)
{
  auto coordsArray = viskores::cont::make_ArrayHandle(coords, viskores::CopyFlag::On);
  auto shapesArray = viskores::cont::make_ArrayHandle(shapes, viskores::CopyFlag::On);
  auto connArray = viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On);

  auto offsetsArray = viskores::cont::ConvertNumComponentsToOffsets(
    viskores::cont::make_ArrayHandle(numIndices, viskores::CopyFlag::Off));

  return DataSetBuilderExplicit::BuildDataSet(
    coordsArray, shapesArray, offsetsArray, connArray, coordsNm);
}

template <typename T>
inline VISKORES_CONT viskores::cont::DataSet DataSetBuilderExplicit::BuildDataSet(
  const viskores::cont::ArrayHandle<viskores::Vec<T, 3>>& coords,
  const viskores::cont::ArrayHandle<viskores::UInt8>& shapes,
  const viskores::cont::ArrayHandle<viskores::Id>& offsets,
  const viskores::cont::ArrayHandle<viskores::Id>& connectivity,
  const std::string& coordsNm)
{
  viskores::cont::DataSet dataSet;

  dataSet.AddCoordinateSystem(viskores::cont::CoordinateSystem(coordsNm, coords));
  viskores::Id nPts = static_cast<viskores::Id>(coords.GetNumberOfValues());
  viskores::cont::CellSetExplicit<> cellSet;

  cellSet.Fill(nPts, shapes, connectivity, offsets);
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

template <typename T, typename CellShapeTag>
inline VISKORES_CONT viskores::cont::DataSet DataSetBuilderExplicit::Create(
  const std::vector<viskores::Vec<T, 3>>& coords,
  CellShapeTag tag,
  viskores::IdComponent numberOfPointsPerCell,
  const std::vector<viskores::Id>& connectivity,
  const std::string& coordsNm)
{
  auto coordsArray = viskores::cont::make_ArrayHandle(coords, viskores::CopyFlag::On);
  auto connArray = viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On);

  return DataSetBuilderExplicit::Create(
    coordsArray, tag, numberOfPointsPerCell, connArray, coordsNm);
}

template <typename T, typename CellShapeTag>
inline VISKORES_CONT viskores::cont::DataSet DataSetBuilderExplicit::BuildDataSet(
  const viskores::cont::ArrayHandle<viskores::Vec<T, 3>>& coords,
  CellShapeTag tag,
  viskores::IdComponent numberOfPointsPerCell,
  const viskores::cont::ArrayHandle<viskores::Id>& connectivity,
  const std::string& coordsNm)
{
  (void)tag; //C4100 false positive workaround
  viskores::cont::DataSet dataSet;

  dataSet.AddCoordinateSystem(viskores::cont::CoordinateSystem(coordsNm, coords));
  viskores::cont::CellSetSingleType<> cellSet;

  cellSet.Fill(coords.GetNumberOfValues(), tag.Id, numberOfPointsPerCell, connectivity);
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

/// @brief Helper class to build a `DataSet` by iteratively adding points and cells.
///
/// This class allows you to specify a `DataSet` by adding points and cells one at a time.
class VISKORES_CONT_EXPORT DataSetBuilderExplicitIterative
{
public:
  VISKORES_CONT DataSetBuilderExplicitIterative();

  /// @brief Begin defining points and cells of a `DataSet`.
  ///
  /// The state of this object is initialized to be ready to use `AddPoint` and
  /// `AddCell` methods.
  ///
  /// @param[in] coordName (optional) The name to register the coordinates as.
  VISKORES_CONT void Begin(const std::string& coordName = "coords");

  /// @brief Add a point to the `DataSet`.
  ///
  /// @param[in] pt The coordinates of the point to add.
  /// @returns The index of the newly created point.
  VISKORES_CONT viskores::Id AddPoint(const viskores::Vec3f& pt);

  /// @brief Add a point to the `DataSet`.
  ///
  /// @param[in] pt The coordinates of the point to add.
  /// @returns The index of the newly created point.
  template <typename T>
  VISKORES_CONT viskores::Id AddPoint(const viskores::Vec<T, 3>& pt)
  {
    return AddPoint(static_cast<viskores::Vec3f>(pt));
  }

  /// @brief Add a point to the `DataSet`.
  ///
  /// @param[in] x The x coordinate of the newly created point.
  /// @param[in] y The y coordinate of the newly created point.
  /// @param[in] z The z coordinate of the newly created point.
  /// @returns The index of the newly created point.
  VISKORES_CONT viskores::Id AddPoint(const viskores::FloatDefault& x,
                                      const viskores::FloatDefault& y,
                                      const viskores::FloatDefault& z = 0);

  /// @brief Add a point to the `DataSet`.
  ///
  /// @param[in] x The x coordinate of the newly created point.
  /// @param[in] y The y coordinate of the newly created point.
  /// @param[in] z The z coordinate of the newly created point.
  /// @returns The index of the newly created point.
  template <typename T>
  VISKORES_CONT viskores::Id AddPoint(const T& x, const T& y, const T& z = 0)
  {
    return AddPoint(static_cast<viskores::FloatDefault>(x),
                    static_cast<viskores::FloatDefault>(y),
                    static_cast<viskores::FloatDefault>(z));
  }

  /// @brief Add a cell to the `DataSet`.
  ///
  /// @param[in] shape Identifies the shape of the cell. Use one of the
  ///   `viskores::CELL_SHAPE_*` values.
  /// @param[in] conn List of indices to the incident points.
  VISKORES_CONT void AddCell(const viskores::UInt8& shape, const std::vector<viskores::Id>& conn);

  /// @brief Add a cell to the `DataSet`.
  ///
  /// @param[in] shape Identifies the shape of the cell. Use one of the
  ///   `viskores::CELL_SHAPE_*` values.
  /// @param[in] conn List of indices to the incident points.
  /// @param[in] n The number of incident points (and the length of the `conn` array).
  VISKORES_CONT void AddCell(const viskores::UInt8& shape,
                             const viskores::Id* conn,
                             const viskores::IdComponent& n);

  /// @brief Start adding a cell to the `DataSet`.
  ///
  /// The incident points are later added one at a time using `AddCellPoint`.
  /// The cell is completed the next time `AddCell` or `Create` is called.
  ///
  /// @param[in] shape Identifies the shape of the cell. Use one of the
  VISKORES_CONT void AddCell(viskores::UInt8 shape);

  /// @brief Add an incident point to the current cell.
  ///
  /// @param[in] pointIndex Index to the incident point.
  VISKORES_CONT void AddCellPoint(viskores::Id pointIndex);

  /// @brief Produce the `DataSet`.
  ///
  /// The points and cells previously added are finalized and the resulting `DataSet`
  /// is returned.
  VISKORES_CONT viskores::cont::DataSet Create();

private:
  std::string coordNm;

  std::vector<viskores::Vec3f> points;
  std::vector<viskores::UInt8> shapes;
  std::vector<viskores::IdComponent> numIdx;
  std::vector<viskores::Id> connectivity;
};
}
}

#endif //viskores_cont_DataSetBuilderExplicit_h
