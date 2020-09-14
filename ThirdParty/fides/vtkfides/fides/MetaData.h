//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_metadata_MetaData_H_
#define fides_metadata_MetaData_H_

#include <fides/Keys.h>
#include <fides/FidesTypes.h>

#include <vtkm/cont/Field.h>

#include <cstddef>
#include <functional>
#include <set>
#include <unordered_map>

#include "fides_export.h"

namespace fides
{
namespace metadata
{

/// \brief Superclass for all meta-data classes.
struct FIDES_EXPORT MetaDataItem
{
protected:
  // This method needs to be here for the class to
  // support dynamic_cast<>
  virtual void MakePolymorphic() {}

  // added so metadata could be copied
  virtual MetaDataItem* CloneImpl() const = 0;

public:
  virtual ~MetaDataItem() {}
  std::unique_ptr<MetaDataItem> Clone() const
  {
    return std::unique_ptr<MetaDataItem>(CloneImpl());
  }
};

/// \brief Meta-data item to store size of things such as number of blocks.
struct FIDES_EXPORT Size : public MetaDataItem
{
  /// constructor
  Size(size_t nItems) : NumberOfItems(nItems) {}

  /// Number of items (e.g., blocks)
  size_t NumberOfItems;

protected:
  virtual Size* CloneImpl() const override
  {
    return new Size(*this);
  }
};

/// \brief Meta-data item to store an index to a container.
struct FIDES_EXPORT Index : public MetaDataItem
{
  Index(size_t idx) : Data(idx) {}
  size_t Data;

protected:
  virtual Index* CloneImpl() const override
  {
    return new Index(*this);
  }
};

/// \brief Simple struct representing field information.
struct FIDES_EXPORT FieldInformation
{
  FieldInformation(std::string name, vtkm::cont::Field::Association assoc) :
    Name(name)
  {
    this->Association = ConvertVTKmAssociationToFides(assoc);
  }

  FieldInformation(std::string name, fides::Association assoc) :
    Name(name), Association(assoc) {}

  /// Name of the field.
  std::string Name;
  /// Association of the field. \sa fides::Association
  fides::Association Association;
};

/// \brief Meta-data item to store a vector.
template<typename T>
struct FIDES_EXPORT Vector : public MetaDataItem
{
  Vector(std::vector<T> vec) : Data(vec) {}
  Vector() {}
  std::vector<T> Data;

protected:
  virtual Vector* CloneImpl() const override
  {
    return new Vector(*this);
  }
};

/// \brief Meta-data item to store a set.
template<typename T>
struct FIDES_EXPORT Set : public MetaDataItem
{
  Set(std::set<T> data) : Data(data) {}
  Set() {}
  std::set<T> Data;

protected:
  virtual Set* CloneImpl() const override
  {
    return new Set(*this);
  }
};

/// \brief Container of meta-data items.
/// This class is a simple wrapper around an std::map that
/// makes setting/getting a bit easier. Internally, it stores
/// objects using unique_ptrs but the interface uses stack
/// objects.
class MetaData
{
public:
  MetaData() = default;
  ~MetaData() = default;
  MetaData(MetaData&& other) = default;
  MetaData& operator=(MetaData&& other) = default;

  MetaData(const MetaData& other)
  {
    for (auto& datum : other.Data)
    {
      this->Data[datum.first] = datum.second->Clone();
    }
  }

  MetaData& operator=(const MetaData& other)
  {
    for (auto& datum : other.Data)
    {
      this->Data[datum.first] = datum.second->Clone();
    }
    return *this;
  }

  /// Add a meta-data item to the map. Supports subclasses
  /// of \c MetaDataItem only.
  template<typename T>
  void Set(fides::keys::KeyType key, const T& item)
  {
    static_assert( !std::is_same<T, fides::metadata::MetaDataItem>::value, "it is not allowed to pass base type (MetaDataItem) to Set()." );
    std::unique_ptr<T> it(new T(item));
    this->Data[key] = std::move(it);
  }

  /// Given a type, returns an object if it exists.
  /// Raises an exception if the item does not exist
  /// or if the provided template argument is incorrect.
  template<typename T>
  const T& Get(fides::keys::KeyType key) const
  {
    auto itr = this->Data.find(key);
    if(itr == this->Data.end())
    {
      throw std::runtime_error("Item not found.");
    }
    T* value = dynamic_cast<T*>(itr->second.get());
    if (!value)
    {
      throw std::runtime_error("Item is not of requested type.");
    }
    return *value;
  }

  /// Given a key, removes the item from the map
  void Remove(fides::keys::KeyType key)
  {
    this->Data.erase(key);
  }

  /// Given a key, checks whether an item exists.
  bool Has(fides::keys::KeyType key) const
  {
    auto itr = this->Data.find(key);
    if(itr == this->Data.end())
    {
      return false;
    }
    return true;
  }

private:
  using MetaDataType = std::unordered_map<fides::keys::KeyType,
    std::unique_ptr<MetaDataItem> >;
  MetaDataType Data;
};
}
}

#endif
