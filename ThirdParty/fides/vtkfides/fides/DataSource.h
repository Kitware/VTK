//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_DataSource_H_
#define fides_datamodel_DataSource_H_

#include <fides/FidesTypes.h>

#if FIDES_USE_MPI
#include <fides/fides_mpi.h>
#endif

#include <fides/MetaData.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <fides_rapidjson.h>
// clang-format off
#include FIDES_RAPIDJSON(rapidjson/document.h)
// clang-format on

namespace fides
{
namespace io
{

enum class DataSourceBackend
{
  ADIOS,
  Conduit
};

enum class IsVector
{
  Yes,
  No,
  Auto
};

/// \brief Abstract data producer for Fides data models.
///
/// \c fides::io::DataSource is responsible of performing the
/// actual operations to load arrays into memory. It produces
/// RawArray objects. Both ADIOS2 and Conduit are currently supported.
class DataSource
{
public:
  virtual ~DataSource() = default;

  virtual DataSourceBackend GetBackendType() = 0;

  /// Set parameters needed by ADIOS. The \c params argument is a map of
  /// ADIOS engine parameters to be used. Currently, only the inline
  /// engine requires this to be called, which must happen before attempting
  /// to read.
  virtual void SetDataSourceParameters(const DataSourceParams& fidesNotUsed(params)) {}

  /// Prepare data source for reading. This needs to be called before
  /// any meta-data or heavy-data operations can be performed.
  /// A map of paths to find data sources and the name of the data source
  /// is provided to find the pathname of the source file.
  /// In most cases, useMPI should be true (the default value), but in some
  /// cases it is useful to open a source without using MPI
  /// (See DataSetReader::CheckForDataModelAttribute for details).
  /// useMPI is ignored if Fides is built without MPI support.
  virtual void OpenSource(const std::unordered_map<std::string, std::string>& paths,
                          const std::string& dataSourceName,
                          bool useMPI = true) = 0;

  /// Prepare data source for reading. This needs to be called before
  /// any meta-data or heavy-data operations can be performed.
  /// In most cases, useMPI should be true (the default value), but in some
  /// cases it is useful to open a source without using MPI
  /// (See DataSetReader::CheckForDataModelAttribute for details).
  /// useMPI is ignored if Fides is built without MPI support.
  virtual void OpenSource(const std::string& fname, bool useMPI = true) = 0;

  /// Returns the number of blocks (partitions) available from the
  /// data source for the given variable name.
  virtual size_t GetNumberOfBlocks(const std::string& fidesNotUsed(varName)) { return 1; }

  /// Returns the number of blocks (partitions) available from the
  /// data source for the given variable name inside the given path.
  virtual size_t GetNumberOfBlocks(const std::string& fidesNotUsed(varName),
                                   const std::string& fidesNotUsed(group))
  {
    return 1;
  }

  /// Returns all paths that contain a variable or attribute with the given name.
  virtual std::set<std::string> GetGroupNames(const std::string& fidesNotUsed(name)) const
  {
    return std::set<std::string>{};
  }

  /// Prepares for reading the requested variable.
  /// Applies the provided set of selections to potentially restricted
  /// what is loaded. Actual reading happens when \c DoAllReads() or
  /// \c EndStep() is called.
  virtual std::vector<fides::RawArray> ReadVariable(const std::string& varName,
                                                    const fides::metadata::MetaData& selections,
                                                    IsVector isit = IsVector::Auto) = 0;

  /// Similar to\c ReadVariable() but for variables where multiple blocks
  /// should be written to a single array (useful for XGC, GTC).
  /// As with \c ReadVariable(), actual reading happens when \c DoAllReads() or
  /// \c EndStep() is called.
  /// Inline engine is not supported with this type of read
  virtual std::vector<fides::RawArray> ReadMultiBlockVariable(
    const std::string& varName,
    const fides::metadata::MetaData& selections) = 0;

  /// Reads a scalar variable and can be used when when an
  /// actual value is needed immediately.
  virtual std::vector<fides::RawArray> GetScalarVariable(
    const std::string& varName,
    const fides::metadata::MetaData& selections) = 0;

  virtual std::vector<fides::RawArray> GetTimeArray(
    const std::string& varName,
    const fides::metadata::MetaData& selections) = 0;

  /// Returns the dimensions and start of an n-dimensional variable.
  /// The first n values are the dimensions and the last n the start.
  /// Unlike ReadVariable(), the values are accessible immediately.
  virtual std::vector<fides::RawArray> GetVariableDimensions(
    const std::string& varName,
    const fides::metadata::MetaData& selections,
    fides::FieldAssociation association = fides::FieldAssociation::Points) = 0;

  /// Returns the total number of steps available to the data source.
  virtual size_t GetNumberOfSteps() { return 1; }

  /// Get the shape (dimensions) of a variable
  virtual std::vector<size_t> GetVariableShape(std::string& varName) = 0;

  /// Get the shape (dimensions) of a variable inside the given group.
  virtual std::vector<size_t> GetVariableShape(std::string& varName, const std::string& group) = 0;

  /// Perform all scheduled reads for this data source. This can be one
  /// or more variables.
  virtual void DoAllReads() {}

  /// Start the next time step.
  virtual StepStatus BeginStep() { return StepStatus::OK; }

  /// get CurrentStep number
  virtual size_t CurrentStep() { return 0; }

  /// Finish the time step. This performs all scheduled reads for this
  /// data source.
  virtual void EndStep() {}

  /// Update the available variables for current time step
  virtual void Refresh() {}

  /// Closes the engine.
  virtual void Close() {}

  void SetSchemaDocument(const std::shared_ptr<rapidjson::Document>& docptr)
  {
    this->SchemaDocument = docptr;
  }

  /// Read a metadata attribute (string-keyed leaf) from the data
  /// source. The string key is interpreted by each backend: ADIOS2 maps
  /// it to a same-named global attribute; Conduit maps it to a Node
  /// path. Missing attributes return an empty vector — callers handle
  /// that as "not present".
  ///
  /// Implemented for the two types the data models currently need
  /// (string, std::int32_t) via per-type virtual hooks below; add a
  /// specialization + matching virtual when a new type is needed.
  template <typename T>
  std::vector<T> ReadAttribute(const std::string& name);

  /// Return the set of immediate child attribute names under the given
  /// path prefix — i.e. the distinct first path components that appear
  /// after "prefix/" among the data source's attribute names. For
  /// ADIOS2 this scans the global attributes; for Conduit it lists the
  /// children of the node at \c prefix. Used to discover the set of
  /// array roles a cell attribute carries without hardcoding them.
  /// Default returns an empty set so a backend without metadata
  /// enumeration degrades gracefully.
  virtual std::set<std::string> GetAttributeNames(const std::string& fidesNotUsed(prefix))
  {
    return {};
  }

protected:
  /// Per-type backend hooks behind the public \c ReadAttribute<T>
  /// template. Not meant to be called directly — go through
  /// \c ReadAttribute<T>. Default returns an empty vector so a backend
  /// without metadata support degrades gracefully.
  virtual std::vector<std::string> ReadStringAttribute(const std::string& fidesNotUsed(name))
  {
    return {};
  }
  virtual std::vector<std::int32_t> ReadInt32Attribute(const std::string& fidesNotUsed(name))
  {
    return {};
  }

  std::shared_ptr<rapidjson::Document> SchemaDocument;
};

template <>
inline std::vector<std::string> DataSource::ReadAttribute<std::string>(const std::string& name)
{
  return this->ReadStringAttribute(name);
}

template <>
inline std::vector<std::int32_t> DataSource::ReadAttribute<std::int32_t>(const std::string& name)
{
  return this->ReadInt32Attribute(name);
}

}
}

#endif
