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

#include <fides/Deprecated.h>
#include <fides/FidesTypes.h>
#include <fides/MetaData.h>

#include <adios2.h>
#include <map>
#include <set>
#include <vector>
#include <viskores/cont/UnknownArrayHandle.h>

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

enum class IsVector
{
  Yes,
  No,
  Auto
};

/// \brief Data producer for Fides data models.
///
/// \c fides::io::DataSource is responsible of performing the
/// actual IO operations to load arrays into memory. It produces
/// Viskores arrays. Only ADIOS2 is currently supported.
struct DataSource
{
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

  DataSource() = default;
  DataSource& operator=(const DataSource& other)
  {
    if (this != &other)
    {
      this->Mode = other.Mode;
      this->FileName = other.FileName;
      this->AdiosEngineType = other.AdiosEngineType;
    }
    return *this;
  }

  DataSource(const DataSource& other)
  {
    if (this != &other)
    {
      this->Mode = other.Mode;
      this->FileName = other.FileName;
      this->AdiosEngineType = other.AdiosEngineType;
    }
  }

  bool StreamingMode = true;

  /// Set parameters needed by ADIOS. The \c params argument is a map of
  /// ADIOS engine parameters to be used. Currently, only the inline
  /// engine requires this to be called, which must happen before attempting
  /// to read.
  void SetDataSourceParameters(const DataSourceParams& params);

  /// Set the IO object for this data source. The \c io argument is
  /// a pointer to an ADIOS::IO object. This call is only required when
  /// using the inline engine and must be called before attempting to read.
  void SetDataSourceIO(void* io);

  /// Set the IO object for this data source. The \c ioAddress argument is
  /// the pointer address to an ADIOS::IO object stored in a string.
  /// This call is only required when
  /// using the inline engine and must be called before attempting to read.
  void SetDataSourceIO(const std::string& ioAddress);

  /// Prepare data source for reading. This needs to be called before
  /// any meta-data or heavy-data operations can be performed.
  /// A map of paths to find data sources and the name of the data source
  /// is provided to find the pathname of the source file.
  /// In most cases, useMPI should be true (the default value), but in some
  /// cases it is useful to open a source without using MPI
  /// (See DataSetReader::CheckForDataModelAttribute for details).
  /// useMPI is ignored if Fides is built without MPI support.
  void OpenSource(const std::unordered_map<std::string, std::string>& paths,
                  const std::string& dataSourceName,
                  bool useMPI = true);

  /// Prepare data source for reading. This needs to be called before
  /// any meta-data or heavy-data operations can be performed.
  /// In most cases, useMPI should be true (the default value), but in some
  /// cases it is useful to open a source without using MPI
  /// (See DataSetReader::CheckForDataModelAttribute for details).
  /// useMPI is ignored if Fides is built without MPI support.
  void OpenSource(const std::string& fname, bool useMPI = true);

  /// Returns the number of blocks (partitions) available from the
  /// data source for the given variable name.
  size_t GetNumberOfBlocks(const std::string& varName);

  /// Returns the number of blocks (partitions) available from the
  /// data source for the given variable name inside the given path.
  size_t GetNumberOfBlocks(const std::string& varName, const std::string& group);

  /// Returns all paths that contain a variable or attribute with the given name.
  std::set<std::string> GetGroupNames(const std::string& name) const;

  /// Prepares for reading the requested variable.
  /// Applies the provided set of selections to potentially restricted
  /// what is loaded. Actual reading happens when \c DoAllReads() or
  /// \c EndStep() is called.
  std::vector<viskores::cont::UnknownArrayHandle> ReadVariable(
    const std::string& varName,
    const fides::metadata::MetaData& selections,
    IsVector isit = IsVector::Auto);

  /// Similar to\c ReadVariable() but for variables where multiple blocks
  /// should be written to a single ArrayHandle (useful for XGC, GTC).
  /// As with \c ReadVariable(), actual reading happens when \c DoAllReads() or
  /// \c EndStep() is called.
  /// Inline engine is not supported with this type of read
  std::vector<viskores::cont::UnknownArrayHandle> ReadMultiBlockVariable(
    const std::string& varName,
    const fides::metadata::MetaData& selections);

  /// Reads a scalar variable and can be used when when an
  /// actual value is needed immediately.
  std::vector<viskores::cont::UnknownArrayHandle> GetScalarVariable(
    const std::string& varName,
    const fides::metadata::MetaData& selections);

  std::vector<viskores::cont::UnknownArrayHandle> GetTimeArray(
    const std::string& varName,
    const fides::metadata::MetaData& selections);

  /// Returns the dimensions and start of an n-dimensional variable.
  /// The first n values are the dimensions and the last n the start.
  /// Unlike ReadVariable(), the values are accessible immediately.
  std::vector<viskores::cont::UnknownArrayHandle> GetVariableDimensions(
    const std::string& varName,
    const fides::metadata::MetaData& selections);

  /// Returns the total number of steps available to the data source.
  size_t GetNumberOfSteps();

  /// Get the shape (dimensions) of a variable
  std::vector<size_t> GetVariableShape(std::string& varName);

  /// Get the shape (dimensions) of a variable inside the given group.
  std::vector<size_t> GetVariableShape(std::string& varName, const std::string& group);

  /// Perform all scheduled reads for this data source. This can be one
  /// or more variables.
  void DoAllReads();

  /// Start the next time step.
  StepStatus BeginStep();

  /// get CurrentStep number
  size_t CurrentStep();

  /// Finish the time step. This performs all scheduled reads for this
  /// data source.
  void EndStep();

  /// Update the available variables for current time step
  void Refresh();

  /// Returns the engine type being used for this data source
  EngineType GetEngineType() { return this->AdiosEngineType; }

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

private:
  std::unique_ptr<adios2::ADIOS> Adios = nullptr;
  adios2::IO AdiosIO;
  adios2::Engine Reader;
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
std::vector<AttributeType> DataSource::ReadAttribute(const std::string& attrName)
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
