//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_ADIOSDataSource_H_
#define fides_datamodel_ADIOSDataSource_H_

#include <fides/DataSource.h>
#include <fides/Deprecated.h>
#include <fides/FidesTypes.h>
#include <fides/MetaData.h>

#include <adios2.h>
#include <map>
#include <set>
#include <vector>

namespace fides
{
namespace io
{

enum class FileNameMode
{
  Input,
  Relative
};

enum class EngineType
{
  BPFile,
  SST,
  Inline
};

class ADIOSDataSource : public DataSource
{
public:
  ADIOSDataSource();

#if FIDES_USE_MPI
  ADIOSDataSource(MPI_Comm comm);
#endif

  ~ADIOSDataSource() override;

  DataSourceBackend GetBackendType() override { return DataSourceBackend::ADIOS; }

  void SetDataSourceParameters(const DataSourceParams& params) override;

  void SetDataSourceIO(void* io);
  void SetDataSourceIO(const std::string& ioAddress);

  EngineType GetEngineType() { return this->AdiosEngineType; }
  void SetEngineType(const std::string engine_type);

  void OpenSource(const std::unordered_map<std::string, std::string>& paths,
                  const std::string& dataSourceName,
                  bool useMPI = true) override;

  void OpenSource(const std::string& fname, bool useMPI = true) override;

  size_t GetNumberOfBlocks(const std::string& varName) override;

  size_t GetNumberOfBlocks(const std::string& varName, const std::string& group) override;

  std::set<std::string> GetGroupNames(const std::string& name) const override;

  std::vector<fides::RawArray> ReadVariable(const std::string& varName,
                                            const fides::metadata::MetaData& selections,
                                            IsVector isit = IsVector::Auto) override;

  std::vector<fides::RawArray> ReadMultiBlockVariable(
    const std::string& varName,
    const fides::metadata::MetaData& selections) override;

  std::vector<fides::RawArray> GetScalarVariable(
    const std::string& varName,
    const fides::metadata::MetaData& selections) override;

  std::vector<fides::RawArray> GetTimeArray(const std::string& varName,
                                            const fides::metadata::MetaData& selections) override;

  std::vector<fides::RawArray> GetVariableDimensions(
    const std::string& varName,
    const fides::metadata::MetaData& selections,
    fides::FieldAssociation association = fides::FieldAssociation::Points) override;

  size_t GetNumberOfSteps() override;

  std::vector<size_t> GetVariableShape(std::string& varName) override;

  std::vector<size_t> GetVariableShape(std::string& varName, const std::string& group) override;

  void DoAllReads() override;

  StepStatus BeginStep() override;

  size_t CurrentStep() override;

  void EndStep() override;

  void Refresh() override;

  void Close() override;

  /// Returns the attribute type in a string. If attribute isn't found,
  /// returns an empty string.
  std::string GetAttributeType(const std::string& attrName);

  /// Returns the type of attribute inside the given group as a string.
  /// If attribute isn't found, returns an empty string.
  std::string GetAttributeType(const std::string& attrName, const std::string& group);

  /// Reads an attribute from ADIOS and returns in an std::vector.
  /// If an attribute is not found, returns an empty std::vector<AttributeType>.
  template <typename AttributeType>
  std::vector<AttributeType> ReadAttribute(const std::string& attrName);

  /// Returns the immediate child attribute names under the given prefix
  /// by scanning the IO's available attributes.
  std::set<std::string> GetAttributeNames(const std::string& prefix) override;

protected:
  /// Base-class hooks for polymorphic attribute reads. Forwarded to the
  /// existing concrete template so the existing call sites that hold a
  /// shared_ptr<ADIOSDataSource> directly continue to use the optimized
  /// direct path; callers that hold a DataSource& go through these.
  std::vector<std::string> ReadStringAttribute(const std::string& attrName) override
  {
    return this->ReadAttribute<std::string>(attrName);
  }
  std::vector<std::int32_t> ReadInt32Attribute(const std::string& attrName) override
  {
    return this->ReadAttribute<std::int32_t>(attrName);
  }

public:
  /// \c FileNameMode determines how full file paths are formed
  /// when loading data. When the \c FileNameMode is set to \c Input,
  /// the argument to \c OpenSource() is used directly and the FileName
  /// data member is ignored. When \c FileNameMode is set to \c Relative,
  /// the FileName is appended to the argument to \c OpenSource(). This
  /// enables the use of multiple files all residing in the same path.
  FileNameMode Mode;

  /// Used only when \c FileNameMode is set to \c Relative.
  std::string FileName = "";

  /// When \c FileNameMode is set to \c Relative, the \c FileName will
  /// be relative to this directory unless otherwise specified in the
  /// paths.
  std::string RelativePath = "";

  /// Determines whether to close gaps between uniform grid blocks
  /// with the use of shared points.
  bool CreateSharedPoints = false;

  /// Determines if we should call the next BeginStep() for this source.
  /// Set to true by the DataSetReader when the schema is contained in a BP file
  /// and it needs to get that from attributes. This is due to BP5 and SST engines
  /// requiring that BeginStep() is called before reading the attributes (unless
  /// we're using random access mode).
  bool DoNotCallNextBeginStep = false;

  bool StreamingMode = true;

private:
#if FIDES_USE_MPI
  MPI_Comm Comm;
#endif

  std::unique_ptr<adios2::ADIOS> Adios = nullptr;
  adios2::IO AdiosIO;
  std::shared_ptr<adios2::Engine> Reader;
  EngineType AdiosEngineType = EngineType::BPFile;
  DataSourceParams SourceParams;
  std::string ReaderID = "inline-reader"; // Only used for Inline Engine
  StepStatus MostRecentStepStatus = StepStatus::NotReady;

  enum class VarType
  {
    PointData,
    CellData
  };
  std::map<std::string, adios2::Params> AvailVars;
  std::map<std::string, adios2::Params> AvailAtts;
  std::map<std::string, std::set<std::string>> AvailGroups;

  void SetupEngine();
  /// Finds a variable with the given name.
  /// If a path selection is provided, then this function looks up a variable with name "groupSelection/name"
  /// Example: FindVariable(name="points", groupSelection="Right/Top/Mesh")
  ///          In this case, the iterator points to a variable named "Right/Top/Mesh/points" (if exists)
  std::map<std::string, adios2::Params>::iterator FindVariable(
    const std::string& name,
    const fides::metadata::MetaData& groupSelection);

  /// Finds an attribute with the given name.
  /// If a path selection is provided, then this function looks up an attribute with name "groupSelection/name"
  /// Example: FindAttribute(name="dims", groupSelection="Right/Top/Mesh")
  ///          In this case, the iterator points to an attribute named "Right/Top/Mesh/dims" (if exists)
  std::map<std::string, adios2::Params>::iterator FindAttribute(
    const std::string& name,
    const fides::metadata::MetaData& groupSelection);
};

template <typename AttributeType>
std::vector<AttributeType> ADIOSDataSource::ReadAttribute(const std::string& attrName)
{
  if (!this->AdiosIO)
  {
    throw std::runtime_error("Cannot read attribute without setting the adios engine.");
  }
  auto attr = this->AdiosIO.InquireAttribute<AttributeType>(attrName);
  if (!attr)
  {
    // just return empty vector here, since attributes
    // aren't always required for Fides to run correctly
    return std::vector<AttributeType>();
  }
  return attr.Data();
}

}
}

#endif
