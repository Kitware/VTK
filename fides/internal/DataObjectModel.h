//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_DataObjectModel_H_
#define fides_datamodel_DataObjectModel_H_

#include <fides/DataContainer.h>
#include <fides/MetaData.h>
#include <fides/internal/DataModel.h>

#include <fides_rapidjson.h>
// clang-format off
#include FIDES_RAPIDJSON(rapidjson/document.h)
// clang-format on

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace fides
{
class OutputBuilder;

namespace predefined
{
class InternalMetadataSource;
}

namespace datamodel
{

/// \brief Abstract base for top-level data models read by \c DataSetReader.
///
/// A \c DataObjectModel represents the entire logical model of a dataset.
/// Concrete subclasses (e.g. \c DataSetModel for traditional dataset-shaped
/// data) own the schema-driven processing, reading, and post-processing
/// logic for one variant of the Fides data model. The naming mirrors VTK's
/// \c vtkDataObject base class. Note this is distinct from
/// \c DataModelBase, which is the per-component base class for
/// \c CoordinateSystem, \c CellSet, \c Field, etc.
struct DataObjectModel
{
  virtual ~DataObjectModel() = default;

  /// Process the model-specific portion of the Fides JSON schema. \c root
  /// is the inner object under the schema's top-level key (e.g. the value
  /// of \c "FidesDataModel"). The implementation extracts coordinate
  /// systems, cell sets, fields, etc. from that object using \c sources to
  /// resolve named data sources.
  virtual void ProcessJSON(const rapidjson::Value& root, DataSourcesType& sources) = 0;

  /// Returns the number of blocks/partitions in the dataset.
  virtual size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                   DataSourcesType& sources,
                                   const std::string& groupName) = 0;

  /// Returns the set of available group names. May be empty.
  virtual std::set<std::string> GetGroupNames(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources) = 0;

  /// Collects field information for metadata reporting. Expanding any
  /// wildcard fields uses \c metadataSource.
  virtual std::vector<fides::metadata::FieldInformation> CollectFieldInformation(
    std::shared_ptr<fides::predefined::InternalMetadataSource>& metadataSource,
    DataSourcesType& sources) = 0;

  /// Reads heavy data into \c builder.
  virtual void Read(const std::unordered_map<std::string, std::string>& paths,
                    DataSourcesType& sources,
                    const fides::metadata::MetaData& selections,
                    fides::OutputBuilder& builder) = 0;

  /// Post-processes the data wrapped in \c container. Called after the
  /// \c OutputBuilder has been finalized.
  virtual void PostRead(fides::DataContainer& container,
                        const fides::metadata::MetaData& selections) = 0;

  /// Returns true if this model can only produce output through the VTK
  /// backend. The reader uses this to reject mismatched
  /// \c fides::DataSetType selections at \c CreateBuilder time.
  virtual bool RequiresVTK() const { return false; }
};

}
}

#endif
