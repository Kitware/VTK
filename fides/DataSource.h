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
#include <fides/MetaData.h>

#include <adios2.h>
#include <vector>
#include <map>
#include <vtkm/cont/VariantArrayHandle.h>

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
/// VTK-m arrays. Only ADIOS2 is currently supported.
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

  DataSource() = default;
  DataSource& operator=(const DataSource& other)
  {
    if(this != &other)
    {
      this->Mode = other.Mode;
      this->FileName = other.FileName;
      this->AdiosEngineType = other.AdiosEngineType;
    }
    return *this;
  }

  DataSource(const DataSource &other)
  {
    if(this != &other)
    {
      this->Mode = other.Mode;
      this->FileName = other.FileName;
      this->AdiosEngineType = other.AdiosEngineType;
    }
  }

  /// Set parameters needed by ADIOS. The \c params argument is a map of
  /// ADIOS engine parameters to be used. Currently, only the inline
  /// engine requires this to be called, which must happen before attempting
  /// to read.
  void SetDataSourceParameters(const DataSourceParams& params);

  /// Set the IO object for this data source. The \c io argument is
  /// a pointer to an ADIOS::IO object. This call is only required when
  /// using the inline engine and must be called before attempting to read.
  void SetDataSourceIO(void* io);

  /// Prepare data source for reading. This needs to be called before
  /// any meta-data or heavy-data operations can be performed.
  void OpenSource(const std::string& fname);

  /// Returns the number of blocks (partitions) available from the
  /// data source for the given variable name.
  size_t GetNumberOfBlocks(const std::string& varName);

  /// Prepares for reading the requested variable.
  /// Applies the provided set of selections to potentially restricted
  /// what is loaded. Actual reading happens when \c DoAllReads() or
  /// \c EndStep() is called.
  std::vector<vtkm::cont::VariantArrayHandle> ReadVariable(
    const std::string& varName,
    const fides::metadata::MetaData& selections,
    IsVector isit=IsVector::Auto);

  /// Similar to\c ReadVariable() but for XGC 3D variables.
  /// fides::keys::PLANE_SELECTION to determine which ADIOS blocks to read and returns a
  /// map of plane ids to the associated ArrayHandle.
  /// As with \c ReadVariable(), actual reading happens when \c DoAllReads() or
  /// \c EndStep() is called.
  std::unordered_map<size_t, vtkm::cont::VariantArrayHandle> ReadXGC3DVariable(
    const std::string& varName,
    const fides::metadata::MetaData& selections);

  /// Reads a scalar variable and can be used when when an
  /// actual value is needed immediately.
  std::vector<vtkm::cont::VariantArrayHandle> GetScalarVariable(
      const std::string& varName,
      const fides::metadata::MetaData& selections);

  /// Returns the dimensions and start of an n-dimensional variable.
  /// The first n values are the dimensions and the last n the start.
  /// Unlike ReadVariable(), the values are accessible immediately.
  std::vector<vtkm::cont::VariantArrayHandle> GetVariableDimensions(
      const std::string& varName,
      const fides::metadata::MetaData& selections);

  /// Returns the total number of steps available to the data source.
  size_t GetNumberOfSteps();

  /// Get the shape (dimensions) of a variable
  std::vector<size_t> GetVariableShape(std::string& varName);

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

  enum class VarType
  {
    PointData,
    CellData
  };
  std::map<std::string, adios2::Params> AvailVars;
  std::map<std::string, adios2::Params> AvailAtts;

  void SetupEngine();
};

template <typename AttributeType>
std::vector<AttributeType> DataSource::ReadAttribute(const std::string& attrName)
{
  if(!this->AdiosIO)
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
