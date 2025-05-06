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
#ifndef viskores_cont_DataSet_h
#define viskores_cont_DataSet_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/UnknownArrayHandle.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/cont/internal/FieldCollection.h>

namespace viskores
{
namespace cont
{

VISKORES_CONT_EXPORT VISKORES_CONT std::string& GlobalGhostCellFieldName() noexcept;

VISKORES_CONT_EXPORT VISKORES_CONT const std::string& GetGlobalGhostCellFieldName() noexcept;

VISKORES_CONT_EXPORT VISKORES_CONT void SetGlobalGhostCellFieldName(
  const std::string& name) noexcept;

/// @brief Contains and manages the geometric data structures that Viskores operates on.
///
/// A `DataSet` is the main data structure used by Viskores to pass data in and out of
/// filters, rendering, and other components. A data set comprises the following 3
/// data structures.
///
/// * **CellSet** A cell set describes topological connections. A cell set defines some
///   number of points in space and how they connect to form cells, filled regions of
///   space. A data set has exactly one cell set.
/// * **Field** A field describes numerical data associated with the topological elements
///   in a cell set. The field is represented as an array, and each entry in the field
///   array corresponds to a topological element (point, edge, face, or cell). Together
///   the cell set topology and discrete data values in the field provide an interpolated
///   function throughout the volume of space covered by the data set. A cell set can
///   have any number of fields.
/// * **CoordinateSystem** A coordinate system is a special field that describes the
///   physical location of the points in a data set. Although it is most common for a
///   data set to contain a single coordinate system, Viskores supports data sets with no
///   coordinate system such as abstract data structures like graphs that might not have
///   positions in a space. `DataSet` also supports multiple coordinate systems for data
///   that have multiple representations for position. For example, geospatial data could
///   simultaneously have coordinate systems defined by 3D position, latitude-longitude,
///   and any number of 2D projections.
class VISKORES_CONT_EXPORT DataSet
{
public:
  DataSet() = default;

  DataSet(viskores::cont::DataSet&&) = default;

  DataSet(const viskores::cont::DataSet&) = default;

  viskores::cont::DataSet& operator=(viskores::cont::DataSet&&) = default;

  viskores::cont::DataSet& operator=(const viskores::cont::DataSet&) = default;

  VISKORES_CONT void Clear();

  /// \brief Get the number of cells contained in this DataSet
  VISKORES_CONT viskores::Id GetNumberOfCells() const;

  /// \brief Get the number of points contained in this DataSet
  ///
  /// Note: All coordinate systems for a DataSet are expected
  /// to have the same number of points.
  VISKORES_CONT viskores::Id GetNumberOfPoints() const;

  /// \brief Adds a field to the `DataSet`.
  ///
  /// Note that the indexing of fields is not the same as the order in which they are
  /// added, and that adding a field can arbitrarily reorder the integer indexing of
  /// all the fields. To retrieve a specific field, retrieve the field by name, not by
  /// integer index.
  VISKORES_CONT void AddField(const Field& field);

  /// \brief Adds a field to the `DataSet`.
  ///
  /// Note that the indexing of fields is not the same as the order in which they are
  /// added, and that adding a field can arbitrarily reorder the integer indexing of
  /// all the fields. To retrieve a specific field, retrieve the field by name, not by
  /// integer index.
  VISKORES_CONT void AddField(const std::string& name,
                              viskores::cont::Field::Association association,
                              const viskores::cont::UnknownArrayHandle& data);

  ///@{
  /// \brief Retrieves a field by index.
  ///
  /// Note that the indexing of fields is not the same as the order in which they are
  /// added, and that adding a field can arbitrarily reorder the integer indexing of
  /// all the fields. To retrieve a specific field, retrieve the field by name, not by
  /// integer index. This method is most useful for iterating over all the fields of
  /// a `DataSet` (indexed from `0` to `NumberOfFields() - 1`).
  VISKORES_CONT
  const viskores::cont::Field& GetField(viskores::Id index) const
  {
    return this->Fields.GetField(index);
  }

  VISKORES_CONT
  viskores::cont::Field& GetField(viskores::Id index) { return this->Fields.GetField(index); }
  ///@}

  VISKORES_CONT
  bool HasField(
    const std::string& name,
    viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any) const
  {
    return this->Fields.HasField(name, assoc);
  }

  VISKORES_CONT
  bool HasCellField(const std::string& name) const
  {
    return (this->Fields.GetFieldIndex(name, viskores::cont::Field::Association::Cells) != -1);
  }

  VISKORES_CONT
  bool HasGhostCellField() const;

  VISKORES_CONT
  const std::string& GetGhostCellFieldName() const;

  VISKORES_CONT
  bool HasPointField(const std::string& name) const
  {
    return (this->Fields.GetFieldIndex(name, viskores::cont::Field::Association::Points) != -1);
  }


  /// \brief Returns the field that matches the provided name and association.
  ///
  /// This method will return -1 if no match for the field is found.
  ///
  /// Note that the indexing of fields is not the same as the order in which they are
  /// added, and that adding a field can arbitrarily reorder the integer indexing of
  /// all the fields. To retrieve a specific field, retrieve the field by name, not by
  /// integer index.
  VISKORES_CONT
  viskores::Id GetFieldIndex(
    const std::string& name,
    viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any) const
  {
    return this->Fields.GetFieldIndex(name, assoc);
  }

  /// \brief Returns the field that matches the provided name and association.
  ///
  /// This method will throw an exception if no match is found. Use `HasField()` to query
  /// whether a particular field exists.
  ///@{
  VISKORES_CONT
  const viskores::cont::Field& GetField(
    const std::string& name,
    viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any) const
  {
    return this->Fields.GetField(name, assoc);
  }


  VISKORES_CONT
  viskores::cont::Field& GetField(
    const std::string& name,
    viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any)
  {
    return this->Fields.GetField(name, assoc);
  }
  ///@}

  /// \brief Returns the first cell field that matches the provided name.
  ///
  /// This method will throw an exception if no match is found. Use `HasCellField()` to query
  /// whether a particular field exists.
  ///@{
  VISKORES_CONT
  const viskores::cont::Field& GetCellField(const std::string& name) const
  {
    return this->GetField(name, viskores::cont::Field::Association::Cells);
  }

  VISKORES_CONT
  viskores::cont::Field& GetCellField(const std::string& name)
  {
    return this->GetField(name, viskores::cont::Field::Association::Cells);
  }
  ///@}

  /// \brief Returns the cell field that matches the ghost cell field name.
  ///
  /// This method will return a constant array of zeros if no match is found. Use `HasGhostCellField()` to query
  /// whether a particular field exists.
  ///@{
  VISKORES_CONT
  viskores::cont::Field GetGhostCellField() const;
  ///@}

  /// \brief Returns the first point field that matches the provided name.
  ///
  /// This method will throw an exception if no match is found. Use `HasPointField()` to query
  /// whether a particular field exists.
  ///@{
  VISKORES_CONT
  const viskores::cont::Field& GetPointField(const std::string& name) const
  {
    return this->GetField(name, viskores::cont::Field::Association::Points);
  }

  VISKORES_CONT
  viskores::cont::Field& GetPointField(const std::string& name)
  {
    return this->GetField(name, viskores::cont::Field::Association::Points);
  }
  ///@}

  ///@{
  /// \brief Adds a point field of a given name to the `DataSet`.
  ///
  /// Note that the indexing of fields is not the same as the order in which they are
  /// added, and that adding a field can arbitrarily reorder the integer indexing of
  /// all the fields. To retrieve a specific field, retrieve the field by name, not by
  /// integer index.
  VISKORES_CONT
  void AddPointField(const std::string& fieldName, const viskores::cont::UnknownArrayHandle& field)
  {
    this->AddField(fieldName, viskores::cont::Field::Association::Points, field);
  }

  template <typename T, typename Storage>
  VISKORES_CONT void AddPointField(const std::string& fieldName,
                                   const viskores::cont::ArrayHandle<T, Storage>& field)
  {
    this->AddPointField(fieldName, viskores::cont::UnknownArrayHandle{ field });
  }

  template <typename T>
  VISKORES_CONT void AddPointField(const std::string& fieldName, const std::vector<T>& field)
  {
    this->AddField(make_Field(
      fieldName, viskores::cont::Field::Association::Points, field, viskores::CopyFlag::On));
  }

  template <typename T>
  VISKORES_CONT void AddPointField(const std::string& fieldName,
                                   const T* field,
                                   const viskores::Id& n)
  {
    this->AddField(make_Field(
      fieldName, viskores::cont::Field::Association::Points, field, n, viskores::CopyFlag::On));
  }
  ///@}

  ///@{
  /// \brief Adds a cell field of a given name to the `DataSet`.
  ///
  /// Note that the indexing of fields is not the same as the order in which they are
  /// added, and that adding a field can arbitrarily reorder the integer indexing of
  /// all the fields. To retrieve a specific field, retrieve the field by name, not by
  /// integer index.
  VISKORES_CONT
  void AddCellField(const std::string& fieldName, const viskores::cont::UnknownArrayHandle& field)
  {
    this->AddField(fieldName, viskores::cont::Field::Association::Cells, field);
  }

  template <typename T, typename Storage>
  VISKORES_CONT void AddCellField(const std::string& fieldName,
                                  const viskores::cont::ArrayHandle<T, Storage>& field)
  {
    this->AddCellField(fieldName, viskores::cont::UnknownArrayHandle{ field });
  }

  template <typename T>
  VISKORES_CONT void AddCellField(const std::string& fieldName, const std::vector<T>& field)
  {
    this->AddField(make_Field(
      fieldName, viskores::cont::Field::Association::Cells, field, viskores::CopyFlag::On));
  }

  template <typename T>
  VISKORES_CONT void AddCellField(const std::string& fieldName,
                                  const T* field,
                                  const viskores::Id& n)
  {
    this->AddField(make_Field(
      fieldName, viskores::cont::Field::Association::Cells, field, n, viskores::CopyFlag::On));
  }
  ///@}

  /// \brief Sets the name of the field to use for cell ghost levels.
  ///
  /// This value can be set regardless of whether such a cell field actually exists.
  VISKORES_CONT void SetGhostCellFieldName(const std::string& name);

  /// \brief Sets the cell field of the given name as the cell ghost levels.
  ///
  /// If a cell field of the given name does not exist, an exception is thrown.
  VISKORES_CONT void SetGhostCellField(const std::string& name);

  ///@{
  /// \brief Sets the ghost cell levels.
  ///
  /// A field of the given name is added to the `DataSet`, and that field is set as the cell
  /// ghost levels.
  ///
  /// Note that the indexing of fields is not the same as the order in which they are
  /// added, and that adding a field can arbitrarily reorder the integer indexing of
  /// all the fields. To retrieve a specific field, retrieve the field by name, not by
  /// integer index.
  VISKORES_CONT void SetGhostCellField(const viskores::cont::Field& field);
  VISKORES_CONT void SetGhostCellField(const std::string& fieldName,
                                       const viskores::cont::UnknownArrayHandle& field);
  ///@}

  /// \brief Sets the ghost cell levels to the given array.
  ///
  /// A field with the global ghost cell field name (see `GlobalGhostCellFieldName`) is added
  /// to the `DataSet` and made to be the cell ghost levels.
  ///
  /// Note that the indexing of fields is not the same as the order in which they are
  /// added, and that adding a field can arbitrarily reorder the integer indexing of
  /// all the fields. To retrieve a specific field, retrieve the field by name, not by
  /// integer index.
  VISKORES_CONT void SetGhostCellField(const viskores::cont::UnknownArrayHandle& field);

  VISKORES_DEPRECATED(2.0, "Use SetGhostCellField.")
  VISKORES_CONT
  void AddGhostCellField(const std::string& fieldName,
                         const viskores::cont::UnknownArrayHandle& field)
  {
    this->SetGhostCellField(fieldName, field);
  }

  VISKORES_DEPRECATED(2.0, "Use SetGhostCellField.")
  VISKORES_CONT
  void AddGhostCellField(const viskores::cont::UnknownArrayHandle& field)
  {
    this->SetGhostCellField(field);
  }

  VISKORES_DEPRECATED(2.0, "Use SetGhostCellField.")
  VISKORES_CONT
  void AddGhostCellField(const viskores::cont::Field& field) { this->SetGhostCellField(field); }


  /// \brief Adds the given `CoordinateSystem` to the `DataSet`.
  ///
  /// The coordinate system will also be added as a point field of the same name.
  ///
  /// \returns the field index assigned to the added coordinate system.
  VISKORES_CONT
  viskores::IdComponent AddCoordinateSystem(const viskores::cont::CoordinateSystem& cs);

  /// \brief Adds a `CoordinateSystem` with the given name and data.
  ///
  /// The coordinate system will also be added as a point field of the same name.
  ///
  /// \returns the field index assigned to the added coordinate system.
  VISKORES_CONT viskores::IdComponent AddCoordinateSystem(
    const std::string& name,
    const viskores::cont::UnknownArrayHandle& data);

  /// \brief Marks the point field with the given name as a coordinate system.
  ///
  /// If no such point field exists or the point field is of the wrong format, an exception
  /// will be throw.
  ///
  /// \returns the field index assigned to the added coordinate system.
  VISKORES_CONT viskores::IdComponent AddCoordinateSystem(const std::string& pointFieldName);

  VISKORES_CONT
  bool HasCoordinateSystem(const std::string& name) const
  {
    return this->GetCoordinateSystemIndex(name) >= 0;
  }

  VISKORES_CONT
  viskores::cont::CoordinateSystem GetCoordinateSystem(viskores::Id index = 0) const;

  /// Returns the index for the CoordinateSystem whose
  /// name matches the provided string.
  /// Will return -1 if no match is found
  VISKORES_CONT
  viskores::IdComponent GetCoordinateSystemIndex(const std::string& name) const;

  VISKORES_CONT const std::string& GetCoordinateSystemName(viskores::Id index = 0) const;

  /// Returns the CoordinateSystem that matches the provided name.
  /// Will throw an exception if no match is found
  VISKORES_CONT
  viskores::cont::CoordinateSystem GetCoordinateSystem(const std::string& name) const;

  template <typename CellSetType>
  VISKORES_CONT void SetCellSet(const CellSetType& cellSet)
  {
    VISKORES_IS_KNOWN_OR_UNKNOWN_CELL_SET(CellSetType);
    this->SetCellSetImpl(cellSet);
  }

  VISKORES_CONT
  const viskores::cont::UnknownCellSet& GetCellSet() const { return this->CellSet; }

  VISKORES_CONT
  viskores::cont::UnknownCellSet& GetCellSet() { return this->CellSet; }

  VISKORES_CONT
  viskores::IdComponent GetNumberOfFields() const { return this->Fields.GetNumberOfFields(); }

  VISKORES_CONT
  viskores::IdComponent GetNumberOfCoordinateSystems() const
  {
    return static_cast<viskores::IdComponent>(this->CoordSystemNames.size());
  }

  /// Copies the structure from the source dataset. The structure includes the cellset,
  /// the coordinate systems, and any ghost layer information. The fields that are not
  /// part of a coordinate system or ghost layers are left unchanged.
  VISKORES_CONT
  void CopyStructure(const viskores::cont::DataSet& source);

  /// \brief Convert the structures in this data set to expected types.
  ///
  /// A `DataSet` object can contain data structures of unknown types. Using the data
  /// requires casting these data structures to concrete types. It is only possible to
  /// check a finite number of data structures.
  ///
  /// The types checked by default are listed in `viskores/cont/DefaultTypes.h`, which can
  /// be configured at compile time. If a `DataSet` contains data not listed there, then
  /// it is likely going to cause problems pulling the data back out. To get around this
  /// problem, you can call this method to convert the data to a form that is likely to
  /// be recognized. This conversion is likely but not guaranteed because not all types
  /// are convertable to something recognizable.
  ///
  VISKORES_CONT void ConvertToExpected();

  VISKORES_CONT
  void PrintSummary(std::ostream& out) const;

private:
  std::vector<std::string> CoordSystemNames;
  viskores::cont::internal::FieldCollection Fields{
    viskores::cont::Field::Association::WholeDataSet,
    viskores::cont::Field::Association::Points,
    viskores::cont::Field::Association::Cells
  };

  viskores::cont::UnknownCellSet CellSet;
  std::shared_ptr<std::string> GhostCellName;

  VISKORES_CONT void SetCellSetImpl(const viskores::cont::UnknownCellSet& cellSet);
};

} // namespace cont
} // namespace viskores

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

/// \brief Specify cell sets to use when serializing a `DataSet`.
///
/// Usually when serializing a `DataSet`, it uses a fixed set of standard
/// `CellSet` types to serialize. If you are writing an algorithm with a
/// custom `CellSet`, you can specify the `CellSet`(s) as the template
/// parameter for this class (either as a list of `CellSet`s or in a
/// single `viskores::List` parameter).
///
template <typename... CellSetTypes>
struct DataSetWithCellSetTypes
{
  viskores::cont::DataSet DataSet;

  DataSetWithCellSetTypes() = default;

  explicit DataSetWithCellSetTypes(const viskores::cont::DataSet& dataset)
    : DataSet(dataset)
  {
  }
};

template <typename... CellSetTypes>
struct DataSetWithCellSetTypes<viskores::List<CellSetTypes...>>
  : DataSetWithCellSetTypes<CellSetTypes...>
{
  using DataSetWithCellSetTypes<CellSetTypes...>::DataSetWithCellSetTypes;
};

template <typename FieldTypeList = VISKORES_DEFAULT_TYPE_LIST,
          typename CellSetTypesList = VISKORES_DEFAULT_CELL_SET_LIST>
struct VISKORES_DEPRECATED(
  2.1,
  "Serialize DataSet directly or use DataSetWithCellSetTypes for weird CellSets.")
  SerializableDataSet : DataSetWithCellSetTypes<CellSetTypesList>
{
  using DataSetWithCellSetTypes<CellSetTypesList>::DataSetWithCellSetTypes;
};

}
} // viskores::cont

namespace mangled_diy_namespace
{

template <>
struct VISKORES_CONT_EXPORT Serialization<viskores::cont::DataSet>
{
  static VISKORES_CONT void foo();
  static VISKORES_CONT void save(BinaryBuffer& bb, const viskores::cont::DataSet& obj);
  static VISKORES_CONT void load(BinaryBuffer& bb, viskores::cont::DataSet& obj);
};

template <typename... CellSetTypes>
struct Serialization<viskores::cont::DataSetWithCellSetTypes<CellSetTypes...>>
{
private:
  using Type = viskores::cont::DataSetWithCellSetTypes<CellSetTypes...>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const Type& serializable)
  {
    const auto& dataset = serializable.DataSet;

    viskoresdiy::save(bb, dataset.GetCellSet().ResetCellSetList(viskores::List<CellSetTypes...>{}));

    viskores::IdComponent numberOfFields = dataset.GetNumberOfFields();
    viskoresdiy::save(bb, numberOfFields);
    for (viskores::IdComponent i = 0; i < numberOfFields; ++i)
    {
      viskoresdiy::save(bb, dataset.GetField(i));
    }

    viskores::IdComponent numberOfCoordinateSystems = dataset.GetNumberOfCoordinateSystems();
    viskoresdiy::save(bb, numberOfCoordinateSystems);
    for (viskores::IdComponent i = 0; i < numberOfCoordinateSystems; ++i)
    {
      viskoresdiy::save(bb, dataset.GetCoordinateSystemName(i));
    }
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, Type& serializable)
  {
    auto& dataset = serializable.DataSet;
    dataset = {}; // clear

    viskores::cont::UncertainCellSet<viskores::List<CellSetTypes...>> cells;
    viskoresdiy::load(bb, cells);
    dataset.SetCellSet(cells);

    viskores::IdComponent numberOfFields = 0;
    viskoresdiy::load(bb, numberOfFields);
    for (viskores::IdComponent i = 0; i < numberOfFields; ++i)
    {
      viskores::cont::Field field;
      viskoresdiy::load(bb, field);
      dataset.AddField(field);
    }

    viskores::IdComponent numberOfCoordinateSystems = 0;
    viskoresdiy::load(bb, numberOfCoordinateSystems);
    for (viskores::IdComponent i = 0; i < numberOfCoordinateSystems; ++i)
    {
      std::string coordName;
      viskoresdiy::load(bb, coordName);
      dataset.AddCoordinateSystem(coordName);
    }
  }
};

template <typename... CellSetTypes>
struct Serialization<viskores::cont::DataSetWithCellSetTypes<viskores::List<CellSetTypes...>>>
  : Serialization<viskores::cont::DataSetWithCellSetTypes<CellSetTypes...>>
{
};

VISKORES_DEPRECATED_SUPPRESS_BEGIN
template <typename FieldTypeList, typename CellSetTypesList>
struct Serialization<viskores::cont::SerializableDataSet<FieldTypeList, CellSetTypesList>>
  : Serialization<viskores::cont::DataSetWithCellSetTypes<CellSetTypesList>>
{
};
VISKORES_DEPRECATED_SUPPRESS_END

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_DataSet_h
