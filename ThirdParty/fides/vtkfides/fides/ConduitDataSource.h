//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_ConduitDataSource_H_
#define fides_datamodel_ConduitDataSource_H_

#include <fides/DataSource.h>
#include <fides/Deprecated.h>
#include <fides/FidesTypes.h>
#include <fides/MetaData.h>

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace fides
{
namespace io
{

class ConduitDataSource : public DataSource
{
public:
  ConduitDataSource();

#if FIDES_USE_MPI
  ConduitDataSource(MPI_Comm comm);
#endif

  ~ConduitDataSource() override;

  DataSourceBackend GetBackendType() override { return DataSourceBackend::Conduit; }

  void OpenSource(const std::unordered_map<std::string, std::string>& tokens,
                  const std::string& dataSourceName,
                  bool useMPI = true) override;

  void OpenSource(const std::string& token, bool useMPI = true) override;

  // NOTE: Selections are currently ignored for data coming from Conduit.
  // \c mode matches the base signature but is ignored.
  std::vector<fides::RawArray> ReadVariable(const std::string& varName,
                                            const fides::metadata::MetaData& selections,
                                            IsVector isit = IsVector::Auto,
                                            ReadMode mode = ReadMode::Deferred) override;

  // NOTE: Selections are currently ignored for data coming from Conduit.
  // \c mode matches the base signature but is ignored.
  std::vector<fides::RawArray> ReadMultiBlockVariable(const std::string& varName,
                                                      const fides::metadata::MetaData& selections,
                                                      ReadMode mode = ReadMode::Deferred) override;

  // NOTE: Selections are currently ignored for data coming from Conduit
  std::vector<fides::RawArray> GetScalarVariable(
    const std::string& varName,
    const fides::metadata::MetaData& selections) override;

  std::vector<fides::RawArray> GetTimeArray(const std::string& varName,
                                            const fides::metadata::MetaData& selections) override;

  // NOTE: Selections are currently ignored for data coming from Conduit
  std::vector<fides::RawArray> GetVariableDimensions(
    const std::string& varName,
    const fides::metadata::MetaData& selections,
    fides::FieldAssociation association = fides::FieldAssociation::Points) override;

  std::vector<size_t> GetVariableShape(std::string& varName) override;

  std::vector<size_t> GetVariableShape(std::string& varName, const std::string& group) override;

  /// Returns the child names of the Conduit node at \c prefix.
  std::set<std::string> GetAttributeNames(const std::string& prefix) override;

protected:
  /// Look up a metadata leaf at the given Conduit Node path. List
  /// nodes are returned one string per child; a string scalar yields a
  /// single-element vector. Missing paths or non-string types return
  /// an empty vector.
  std::vector<std::string> ReadStringAttribute(const std::string& name) override;

  /// Like ReadStringAttribute, but for integer scalars / lists.
  /// Accepts any int width (to_int32 converts on read).
  std::vector<std::int32_t> ReadInt32Attribute(const std::string& name) override;

private:
#if FIDES_USE_MPI
  MPI_Comm Comm;
#endif

  class InternalImpl;
  std::unique_ptr<InternalImpl> Internals;
};

}
}

#endif
