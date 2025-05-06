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
#ifndef viskores_cont_Field_h
#define viskores_cont_Field_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/Range.h>
#include <viskores/Types.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CastAndCall.h>
#include <viskores/cont/UnknownArrayHandle.h>

namespace viskores
{
namespace cont
{


/// A \c Field encapsulates an array on some piece of the mesh, such as
/// the points, a cell set, a point logical dimension, or the whole mesh.
///
class VISKORES_CONT_EXPORT Field
{
public:
  /// @brief Identifies what elements of a data set a field is associated with.
  ///
  /// The `Association` enum is used by `viskores::cont::Field` to specify on what
  /// topological elements each item in the field is associated with.
  enum struct Association
  {
    // Documentation is below (for better layout in generated documents).
    Any,
    WholeDataSet,
    Points,
    Cells,
    Partitions,
    Global,
  };

  /// @var Association Any
  /// @brief Any field regardless of the association.
  ///
  /// This is used when choosing a `viskores::cont::Field` that could be of any
  /// association. It is often used as the default if no association is given.

  /// @var Association WholeDataSet
  /// @brief A "global" field that applies to the entirety of a `viskores::cont::DataSet`.
  ///
  /// Fields of this association often contain summary or annotation information.
  /// An example of a whole data set field could be the region that the mesh covers.

  /// @var Association Points
  /// @brief A field that applies to points.
  ///
  /// There is a separate field value attached to each point. Point fields usually represent
  /// samples of continuous data that can be reinterpolated through cells. Physical properties
  /// such as temperature, pressure, density, velocity, etc. are usually best represented in
  /// point fields. Data that deals with the points of the topology, such as displacement
  /// vectors, are also appropriate for point data.

  /// @var Association Cells
  /// @brief A field that applies to cells.
  ///
  /// There is a separate field value attached to each cell in a cell set. Cell fields
  /// usually represent values from an integration over the finite cells of the mesh.
  /// Integrated values like mass or volume are best represented in cell fields. Statistics
  /// about each cell like strain or cell quality are also appropriate for cell data.

  /// @var Association Partitions
  /// @brief A field that applies to partitions.
  ///
  /// This type of field is attached to a `viskores::cont::PartitionedDataSet`. There is a
  /// separate field value attached to each partition. Identification or information
  /// about the arrangement of partitions such as hierarchy levels are usually best
  /// represented in partition fields.

  /// @var Association Global
  /// @brief A field that applies to all partitions.
  ///
  /// This type of field is attached to a `viskores::cont::PartitionedDataSet`. It contains
  /// values that are "global" across all partitions and data therin.

  VISKORES_CONT
  Field() = default;

  /// Create a field with the given name, association, and data.
  VISKORES_CONT
  Field(std::string name, Association association, const viskores::cont::UnknownArrayHandle& data);

  /// Create a field with the given name, association, and data.
  template <typename T, typename Storage>
  VISKORES_CONT Field(std::string name,
                      Association association,
                      const viskores::cont::ArrayHandle<T, Storage>& data)
    : Field(name, association, viskores::cont::UnknownArrayHandle{ data })
  {
  }

  Field(const viskores::cont::Field& src);
  Field(viskores::cont::Field&& src) noexcept;

  VISKORES_CONT virtual ~Field();

  VISKORES_CONT Field& operator=(const viskores::cont::Field& src);
  VISKORES_CONT Field& operator=(viskores::cont::Field&& src) noexcept;

  /// Return true if this field is associated with cells.
  VISKORES_CONT bool IsCellField() const { return this->FieldAssociation == Association::Cells; }
  /// Return true if this field is associated with points.
  VISKORES_CONT bool IsPointField() const { return this->FieldAssociation == Association::Points; }
  /// Return true if this field is associated with the whole data set.
  VISKORES_CONT bool IsWholeDataSetField() const
  {
    return this->FieldAssociation == Association::WholeDataSet;
  }
  /// Return true if this field is associated with partitions in a partitioned data set.
  VISKORES_CONT bool IsPartitionsField() const
  {
    return this->FieldAssociation == Association::Partitions;
  }
  /// Return true if this field is global.
  /// A global field is applied to a `viskores::cont::PartitionedDataSet` to refer to data that
  /// applies across an entire collection of data.
  VISKORES_CONT bool IsGlobalField() const { return this->FieldAssociation == Association::Global; }

  /// Returns true if the array of the field has a value type that matches something in
  /// `VISKORES_FIELD_TYPE_LIST` and a storage that matches something in `VISKORES_FIELD_STORAGE_LIST`.
  VISKORES_CONT bool IsSupportedType() const;

  /// Return the number of values in the field array.
  VISKORES_CONT viskores::Id GetNumberOfValues() const { return this->Data.GetNumberOfValues(); }

  /// Return the name of the field.
  VISKORES_CONT const std::string& GetName() const { return this->Name; }
  /// Return the association of the field.
  VISKORES_CONT Association GetAssociation() const { return this->FieldAssociation; }
  /// Get the array of the data for the field.
  const viskores::cont::UnknownArrayHandle& GetData() const;
  /// Get the array of the data for the field.
  viskores::cont::UnknownArrayHandle& GetData();

  /// @brief Returns the range of each component in the field array.
  ///
  /// The ranges of each component are returned in an `ArrayHandle` containing `viskores::Range`
  /// values.
  /// So, for example, calling `GetRange` on a scalar field will return an `ArrayHandle`
  /// with exactly 1 entry in it. Calling `GetRange` on a field of 3D vectors will return
  /// an `ArrayHandle` with exactly 3 entries corresponding to each of the components in
  /// the range.
  VISKORES_CONT const viskores::cont::ArrayHandle<viskores::Range>& GetRange() const;

  /// @brief Returns the range of each component in the field array.
  ///
  /// A C array of `viskores::Range` objects is passed in as a place to store the result.
  /// It is imperative that the array be allocated to be large enough to hold an entry
  /// for each component.
  VISKORES_CONT void GetRange(viskores::Range* range) const;

  /// \brief Get the data as an array with `viskores::FloatDefault` components.
  ///
  /// Returns a `viskores::cont::UnknownArrayHandle` that contains an array that either contains
  /// values of type `viskores::FloatDefault` or contains `Vec`s with components of type
  /// `viskores::FloatDefault`. If the array has value types that do not match this type, then
  /// it will be copied into an array that does.
  ///
  /// Additionally, the returned array will have a storage that is compatible with
  /// something in `VISKORES_FIELD_STORAGE_LIST`. If this condition is not met, then the
  /// array will be copied.
  ///
  /// If the array contained in the field already matches the required criteria, the array
  /// will be returned without copying.
  ///
  VISKORES_CONT viskores::cont::UnknownArrayHandle GetDataAsDefaultFloat() const;

  /// \brief Get the data as an array of an expected type.
  ///
  /// Returns a `viskores::cont::UnknownArrayHandle` that contains an array that (probably) has
  /// a value type that matches something in `VISKORES_FIELD_TYPE_LIST` and a storage that matches
  /// something in `VISKORES_FIELD_STORAGE_LIST`. If the array has a value type and storage that
  /// match `VISKORES_FIELD_TYPE_LIST` and `VISKORES_FIELD_STORAGE_LIST` respectively, then the same
  /// array is returned. If something does not match, then the data are copied to a
  /// `viskores::cont::ArrayHandleBasic` with a value type component of `viskores::FloatDefault`.
  ///
  /// Note that the returned array is likely to be compatible with `VISKORES_FIELD_TYPE_LIST`, but
  /// not guaranteed. In particular, if this field contains `Vec`s, the returned array will also
  /// contain `Vec`s of the same size. For example, if the field contains `viskores::Vec2i_16` values,
  /// they will (likely) be converted to `viskores::Vec2f`. Howver, `viskores::Vec2f` may still not be
  /// in `VISKORES_FIELD_TYPE_LIST`.
  ///
  VISKORES_CONT viskores::cont::UnknownArrayHandle GetDataWithExpectedTypes() const;

  /// \brief Convert this field to use an array of an expected type.
  ///
  /// Copies the internal data, as necessary, to an array that (probably) has a value type
  /// that matches something in `VISKORES_FIELD_TYPE_LIST` and a storage that matches something
  /// in `VISKORES_FIELD_STORAGE_LIST`. If the field already has a value type and storage that
  /// match `VISKORES_FIELD_TYPE_LIST` and `VISKORES_FIELD_STORAGE_LIST` respectively, then nothing
  /// in the field is changed. If something does not match, then the data are copied to a
  /// `viskores::cont::ArrayHandleBasic` with a value type component of `viskores::FloatDefault`.
  ///
  /// Note that the returned array is likely to be compatible with `VISKORES_FIELD_TYPE_LIST`, but
  /// not guaranteed. In particular, if this field contains `Vec`s, the returned array will also
  /// contain `Vec`s of the same size. For example, if the field contains `viskores::Vec2i_16` values,
  /// they will (likely) be converted to `viskores::Vec2f`. Howver, `viskores::Vec2f` may still not be
  /// in `VISKORES_FIELD_TYPE_LIST`.
  ///
  VISKORES_CONT void ConvertToExpected();

  VISKORES_CONT void SetData(const viskores::cont::UnknownArrayHandle& newdata);

  template <typename T, typename StorageTag>
  VISKORES_CONT void SetData(const viskores::cont::ArrayHandle<T, StorageTag>& newdata)
  {
    this->SetData(viskores::cont::UnknownArrayHandle(newdata));
  }

  /// Print a summary of the data in the field.
  VISKORES_CONT
  virtual void PrintSummary(std::ostream& out, bool full = false) const;

  /// Remove the data from the device memory (but preserve the data on the host).
  VISKORES_CONT
  virtual void ReleaseResourcesExecution()
  {
    this->Data.ReleaseResourcesExecution();
    this->Range.ReleaseResourcesExecution();
  }

private:
  std::string Name; ///< name of field

  Association FieldAssociation = Association::Any;
  viskores::cont::UnknownArrayHandle Data;
  mutable viskores::cont::ArrayHandle<viskores::Range> Range;
  mutable bool ModifiedFlag = true;
};

template <typename Functor, typename... Args>
void CastAndCall(const viskores::cont::Field& field, Functor&& f, Args&&... args)
{
  viskores::cont::CastAndCall(
    field.GetData(), std::forward<Functor>(f), std::forward<Args>(args)...);
}


//@{
/// Convenience functions to build fields from C style arrays and std::vector
template <typename T>
viskores::cont::Field make_Field(std::string name,
                                 Field::Association association,
                                 const T* data,
                                 viskores::Id size,
                                 viskores::CopyFlag copy)
{
  return viskores::cont::Field(
    name, association, viskores::cont::make_ArrayHandle(data, size, copy));
}

template <typename T>
viskores::cont::Field make_Field(std::string name,
                                 Field::Association association,
                                 const std::vector<T>& data,
                                 viskores::CopyFlag copy)
{
  return viskores::cont::Field(name, association, viskores::cont::make_ArrayHandle(data, copy));
}

template <typename T>
viskores::cont::Field make_FieldMove(std::string name,
                                     Field::Association association,
                                     std::vector<T>&& data)
{
  return viskores::cont::Field(
    name, association, viskores::cont::make_ArrayHandleMove(std::move(data)));
}

template <typename T>
viskores::cont::Field make_Field(std::string name,
                                 Field::Association association,
                                 std::vector<T>&& data,
                                 viskores::CopyFlag viskoresNotUsed(copy))
{
  return make_FieldMove(name, association, std::move(data));
}

template <typename T>
viskores::cont::Field make_Field(std::string name,
                                 Field::Association association,
                                 std::initializer_list<T>&& data)
{
  return make_FieldMove(name, association, viskores::cont::make_ArrayHandle(std::move(data)));
}

//@}

/// Convenience function to build point fields from viskores::cont::ArrayHandle
template <typename T, typename S>
viskores::cont::Field make_FieldPoint(std::string name,
                                      const viskores::cont::ArrayHandle<T, S>& data)
{
  return viskores::cont::Field(name, viskores::cont::Field::Association::Points, data);
}

/// Convenience function to build point fields from viskores::cont::UnknownArrayHandle
inline viskores::cont::Field make_FieldPoint(std::string name,
                                             const viskores::cont::UnknownArrayHandle& data)
{
  return viskores::cont::Field(name, viskores::cont::Field::Association::Points, data);
}

/// Convenience function to build cell fields from viskores::cont::ArrayHandle
template <typename T, typename S>
viskores::cont::Field make_FieldCell(std::string name,
                                     const viskores::cont::ArrayHandle<T, S>& data)
{
  return viskores::cont::Field(name, viskores::cont::Field::Association::Cells, data);
}


/// Convenience function to build cell fields from viskores::cont::UnknownArrayHandle
inline viskores::cont::Field make_FieldCell(std::string name,
                                            const viskores::cont::UnknownArrayHandle& data)
{
  return viskores::cont::Field(name, viskores::cont::Field::Association::Cells, data);
}

} // namespace cont
} // namespace viskores


namespace viskores
{
namespace cont
{
namespace internal
{
template <>
struct DynamicTransformTraits<viskores::cont::Field>
{
  using DynamicTag = viskores::cont::internal::DynamicTransformTagCastAndCall;
};
} // namespace internal
} // namespace cont
} // namespace viskores

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION

namespace mangled_diy_namespace
{

template <>
struct VISKORES_CONT_EXPORT Serialization<viskores::cont::Field>
{
  static VISKORES_CONT void save(BinaryBuffer& bb, const viskores::cont::Field& field);
  static VISKORES_CONT void load(BinaryBuffer& bb, viskores::cont::Field& field);
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_Field_h
