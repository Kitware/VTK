//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_FieldDataManager_h
#define fides_datamodel_FieldDataManager_h

#include <fides/FieldData.h>

#include <unordered_map>

#include "fides_export.h"

namespace fides
{

namespace io
{
  // fwd declaration needed for friend class
  class DataSetReader;
}

namespace datamodel
{

/// \brief Stores all FieldData.
///
/// Use this to access fields that are marked as fides::Association::FIELD_DATA.
class FIDES_EXPORT FieldDataManager
{
public:
  /// Add the given FieldData to the Manager. Throws error if
  /// field already exists.
  /// \param name name of field to add
  /// \param array FieldData object to add
  void AddField(const std::string& name, FieldData array);

  /// Check to see if the Manager already contains FieldData with the given name.
  /// \param name name of field
  bool HasField(const std::string& name);

  /// Returns the FieldData stored with the given name.
  /// Throws an error if the field isn't found.
  /// \param name name of field
  FieldData& GetField(const std::string& name);

  /// Returns a ref to the unordered_map containing all fields.
  const std::unordered_map<std::string, FieldData>& GetAllFields();

private:
  // Only let DataSetReader have the ability to clear
  friend class fides::io::DataSetReader;
  void Clear();

  std::unordered_map<std::string, FieldData> Data;
};

}
}

#endif
