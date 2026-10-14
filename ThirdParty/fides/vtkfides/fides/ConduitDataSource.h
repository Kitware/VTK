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

  /// A variable has one block, or, if it has a \c blocks child
  /// (\c variables/<name>/blocks by default, or the schema's \c blocks_path),
  /// one block per child of it. Each such child holds the block's \c data and
  /// \c shape, and optionally \c start (used by GetVariableDimensions). The
  /// names of these children are fixed; the schema's \c data_path does not
  /// apply to a variable with blocks. The variable's \c dtype applies to all
  /// of its blocks; without one, all blocks must store the same type. An empty
  /// \c blocks child is a variable with no blocks on this rank, for which
  /// reads return no arrays. Returns 0 for such a variable, and for a variable
  /// that does not exist.
  size_t GetNumberOfBlocks(const std::string& varName) override;

  size_t GetNumberOfBlocks(const std::string& varName, const std::string& group) override;

  /// Returns one array per block in the block selection, or per block of the
  /// variable if there is no block selection. Other selections are ignored.
  /// \c mode matches the base signature but is ignored.
  std::vector<fides::RawArray> ReadVariable(const std::string& varName,
                                            const fides::metadata::MetaData& selections,
                                            IsVector isit = IsVector::Auto,
                                            ReadMode mode = ReadMode::Deferred) override;

  /// Like ReadVariable, but returns the selected blocks as a single contiguous
  /// array. \c mode matches the base signature but is ignored.
  std::vector<fides::RawArray> ReadMultiBlockVariable(const std::string& varName,
                                                      const fides::metadata::MetaData& selections,
                                                      ReadMode mode = ReadMode::Deferred) override;

  // NOTE: Selections are currently ignored for data coming from Conduit
  std::vector<fides::RawArray> GetScalarVariable(
    const std::string& varName,
    const fides::metadata::MetaData& selections) override;

  std::vector<fides::RawArray> GetTimeArray(const std::string& varName,
                                            const fides::metadata::MetaData& selections) override;

  /// Returns one [dims..., starts...] array per selected block (see
  /// ReadVariable). Starts are 0 unless the block has a \c start child.
  /// \c shape and \c start are used in the order they are stored, which for
  /// structured data must be fastest-varying first (x, y, z). This is the
  /// reverse of an ADIOS2 block's \c Count and \c Start, which the ADIOS2
  /// data source reverses, so a producer that copies ADIOS2 block metadata into
  /// a Conduit node must reverse both. \c shape and \c start may have any
  /// integer dtype.
  std::vector<fides::RawArray> GetVariableDimensions(
    const std::string& varName,
    const fides::metadata::MetaData& selections,
    fides::FieldAssociation association = fides::FieldAssociation::Points) override;

  /// Returns the variable's shape, or for a multi-block variable without a
  /// variable-level shape, the shape of its first block (empty if it has no
  /// blocks).
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
