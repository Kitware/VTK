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
#ifndef viskores_cont_PartitionedDataSet_h
#define viskores_cont_PartitionedDataSet_h
#include <limits>
#include <viskores/StaticAssert.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/internal/FieldCollection.h>

namespace viskores
{
namespace cont
{

/// @brief Comprises a set of `viskores::cont::DataSet` objects.
class VISKORES_CONT_EXPORT PartitionedDataSet
{
  using StorageVec = std::vector<viskores::cont::DataSet>;

public:
  using iterator = typename StorageVec::iterator;
  using const_iterator = typename StorageVec::const_iterator;
  using value_type = typename StorageVec::value_type;
  using reference = typename StorageVec::reference;
  using const_reference = typename StorageVec::const_reference;

  /// Create a new PartitionedDataSet containng a single DataSet @a ds.
  VISKORES_CONT
  PartitionedDataSet(const viskores::cont::DataSet& ds);

  /// Create a new PartitionedDataSet with a DataSet vector @a partitions.
  VISKORES_CONT
  explicit PartitionedDataSet(const std::vector<viskores::cont::DataSet>& partitions);
  /// Create a new PartitionedDataSet with the capacity set to be @a size.
  VISKORES_CONT
  explicit PartitionedDataSet(viskores::Id size);

  VISKORES_CONT
  PartitionedDataSet() = default;

  /// Get the field @a field_name from partition @a partition_index.
  VISKORES_CONT
  viskores::cont::Field GetFieldFromPartition(const std::string& field_name,
                                              int partition_index) const;

  /// Get number of DataSet objects stored in this PartitionedDataSet.
  VISKORES_CONT
  viskores::Id GetNumberOfPartitions() const;

  /// Get number of partations across all MPI ranks.
  /// @warning This method requires global communication (MPI_Allreduce) if MPI is enabled.
  VISKORES_CONT
  viskores::Id GetGlobalNumberOfPartitions() const;

  /// Get the DataSet @a partId.
  VISKORES_CONT
  const viskores::cont::DataSet& GetPartition(viskores::Id partId) const;

  /// Get an STL vector of all DataSet objects stored in PartitionedDataSet.
  VISKORES_CONT const std::vector<viskores::cont::DataSet>& GetPartitions() const;

  /// Add DataSet @a ds to the end of the list of partitions.
  VISKORES_CONT void AppendPartition(const viskores::cont::DataSet& ds);

  /// @brief Add DataSet @a ds to position @a index of the contained DataSet vector.
  ///
  /// All partitions at or after this location are pushed back.
  VISKORES_CONT void InsertPartition(viskores::Id index, const viskores::cont::DataSet& ds);

  /// Replace the @a index positioned element of the contained DataSet vector
  /// with @a ds.
  VISKORES_CONT void ReplacePartition(viskores::Id index, const viskores::cont::DataSet& ds);

  /// Append the DataSet vector @a partitions to the end of list of partitions.
  ///
  /// This list can be provided as a `std::vector`, or it can be an initializer
  /// list (declared in `{ }` curly braces).
  VISKORES_CONT void AppendPartitions(const std::vector<viskores::cont::DataSet>& partitions);

  /// Methods to Add and Get fields on a PartitionedDataSet
  VISKORES_CONT viskores::IdComponent GetNumberOfFields() const
  {
    return this->Fields.GetNumberOfFields();
  }

  /// @brief Adds a field that is applied to the meta-partition structure.
  ///
  /// The `field` must have an association that applies across all partitions.
  VISKORES_CONT void AddField(const Field& field) { this->Fields.AddField(field); }

  /// @brief Adds a field that is applied to the meta-partition structure.
  ///
  /// The `field` must have an association that applies across all partitions.
  VISKORES_CONT void AddField(const std::string& name,
                              viskores::cont::Field::Association association,
                              const viskores::cont::UnknownArrayHandle& data)
  {
    this->AddField({ name, association, data });
  }

  /// @brief Add a field with a global association.
  template <typename T, typename Storage>
  VISKORES_CONT void AddGlobalField(const std::string& fieldName,
                                    const viskores::cont::ArrayHandle<T, Storage>& field)
  {
    this->AddField(fieldName, viskores::cont::Field::Association::Global, field);
  }

  template <typename T>
  VISKORES_CONT void AddGlobalField(const std::string& fieldName, const std::vector<T>& field)
  {
    this->AddField(make_Field(
      fieldName, viskores::cont::Field::Association::Global, field, viskores::CopyFlag::On));
  }

  template <typename T>
  VISKORES_CONT void AddGlobalField(const std::string& fieldName,
                                    const T* field,
                                    const viskores::Id& n)
  {
    this->AddField(make_Field(
      fieldName, viskores::cont::Field::Association::Global, field, n, viskores::CopyFlag::On));
  }

  /// @brief Add a field where each entry is associated with a whole partition.
  template <typename T, typename Storage>
  VISKORES_CONT void AddPartitionsField(const std::string& fieldName,
                                        const viskores::cont::ArrayHandle<T, Storage>& field)
  {
    this->AddField(fieldName, viskores::cont::Field::Association::Partitions, field);
  }

  template <typename T>
  VISKORES_CONT void AddPartitionsField(const std::string& fieldName, const std::vector<T>& field)
  {
    this->AddField(make_Field(
      fieldName, viskores::cont::Field::Association::Partitions, field, viskores::CopyFlag::On));
  }

  template <typename T>
  VISKORES_CONT void AddPartitionsField(const std::string& fieldName,
                                        const T* field,
                                        const viskores::Id& n)
  {
    this->AddField(make_Field(
      fieldName, viskores::cont::Field::Association::Partitions, field, n, viskores::CopyFlag::On));
  }

  VISKORES_CONT
  const viskores::cont::Field& GetField(viskores::Id index) const
  {
    return this->Fields.GetField(index);
  }

  VISKORES_CONT
  viskores::cont::Field& GetField(viskores::Id index) { return this->Fields.GetField(index); }

  VISKORES_CONT
  const viskores::cont::Field& GetField(
    const std::string& name,
    viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any) const
  {
    return this->Fields.GetField(name, assoc);
  }

  /// @brief Get a field associated with the partitioned data structure.
  ///
  /// The field is selected by name and, optionally, the association.
  VISKORES_CONT
  viskores::cont::Field& GetField(
    const std::string& name,
    viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any)
  {
    return this->Fields.GetField(name, assoc);
  }

  /// @brief Get a global field.
  VISKORES_CONT
  const viskores::cont::Field& GetGlobalField(const std::string& name) const
  {
    return this->GetField(name, viskores::cont::Field::Association::Global);
  }

  /// @brief Get a field associated with the partitions.
  VISKORES_CONT
  const viskores::cont::Field& GetPartitionsField(const std::string& name) const
  {
    return this->GetField(name, viskores::cont::Field::Association::Partitions);
  }

  VISKORES_CONT
  viskores::cont::Field& GetGlobalField(const std::string& name)
  {
    return this->GetField(name, viskores::cont::Field::Association::Global);
  }

  VISKORES_CONT
  viskores::cont::Field& GetPartitionsField(const std::string& name)
  {
    return this->GetField(name, viskores::cont::Field::Association::Partitions);
  }

  /// @brief Query whether the partitioned data set has the named field.
  VISKORES_CONT
  bool HasField(
    const std::string& name,
    viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any) const
  {
    return this->Fields.HasField(name, assoc);
  }

  /// @brief Query whether the partitioned data set has the named global field.
  VISKORES_CONT bool HasGlobalField(const std::string& name) const
  {
    return (this->Fields.GetFieldIndex(name, viskores::cont::Field::Association::Global) != -1);
  }

  /// @brief Query whether the partitioned data set has the named partition field.
  VISKORES_CONT bool HasPartitionsField(const std::string& name) const
  {
    return (this->Fields.GetFieldIndex(name, viskores::cont::Field::Association::Partitions) != -1);
  }

  /// Copies the partitions from the source. The fields on the PartitionedDataSet are not copied.
  VISKORES_CONT
  void CopyPartitions(const viskores::cont::PartitionedDataSet& source);

  VISKORES_CONT
  void PrintSummary(std::ostream& stream) const;

  /// @name Iterators
  ///
  /// `PartitionedDataSet` provides an iterator interface that allows you to iterate
  /// over the contained partitions using the `for (auto ds : pds)` syntax.
  /// @{
  VISKORES_CONT
  iterator begin() noexcept { return this->Partitions.begin(); }
  VISKORES_CONT
  iterator end() noexcept { return this->Partitions.end(); }
  VISKORES_CONT
  const_iterator begin() const noexcept { return this->Partitions.begin(); }
  VISKORES_CONT
  const_iterator end() const noexcept { return this->Partitions.end(); }
  VISKORES_CONT
  const_iterator cbegin() const noexcept { return this->Partitions.cbegin(); }
  VISKORES_CONT
  const_iterator cend() const noexcept { return this->Partitions.cend(); }
  /// @}

private:
  std::vector<viskores::cont::DataSet> Partitions;

  viskores::cont::internal::FieldCollection Fields{ viskores::cont::Field::Association::Partitions,
                                                    viskores::cont::Field::Association::Global };
};
}
} // namespace viskores::cont

#endif
