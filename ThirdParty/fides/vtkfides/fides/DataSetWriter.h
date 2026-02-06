//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_io_DataSetWriter_h
#define fides_io_DataSetWriter_h

#include <fides/FidesTypes.h>
#include <fides/MetaData.h>

#include <viskores/cont/DataSet.h>
#include <viskores/cont/PartitionedDataSet.h>

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "fides_export.h"

#ifdef FIDES_USE_MPI
#include <vtk_mpi.h>
#endif

namespace fides
{
namespace io
{

/// \brief General purpose writer for data described by an Fides data model.
///
/// \c fides::io::DataSetWriter writes a viskores dataset to a file.
/// This class provides functionality to configure the writing process,
/// including setting the ADIOS configuration file and specifying which fields
/// to write. When MPI is enabled, it also supports passing a communicator, with
/// the default being MPI_COMM_WORLD.
class FIDES_EXPORT DataSetWriter
{
public:
  DataSetWriter(const std::string& outputFile);
#ifdef FIDES_USE_MPI
  DataSetWriter(const std::string& outputFile, MPI_Comm comm);
#endif
  ~DataSetWriter() = default;

  /// Sets the ADIOS configuration file for the writer. Note: If you use an ADIOS config file,
  /// outputMode will be ignored in the Write() method if used. When using the config file, it is
  /// expected that the IO object will be named "fides-write-io" in the config file.
  /// \param adiosConfig The path to the ADIOS configuration file.
  void SetAdiosConfigFile(const std::string& adiosConfig) { this->AdiosConfigFile = adiosConfig; }

  /// Writes the given partitioned dataset to the output file.
  /// \param dataSets The partitioned dataset to write.
  /// \param outputMode The mode in which to write the dataset (e.g., synchronous, asynchronous).
  void Write(const viskores::cont::PartitionedDataSet& dataSets, const std::string& outputMode);

  /// Writes the given partitioned dataset to the output file using default output mode.
  /// \param dataSets The partitioned dataset to write.
  void Write(const viskores::cont::PartitionedDataSet& dataSets);

  /// Specifies which fields to write in the dataset.
  /// \param writeFields A vector of field names to be included in the write operation.
  void SetWriteFields(const std::vector<std::string>& writeFields)
  {
    this->FieldsToWrite.clear();

    for (const auto& v : writeFields)
      this->FieldsToWrite.insert(v);
    this->WriteFieldSet = true;
  }

  // Enumeration of dataset types supported by the writer.
  const unsigned char DATASET_TYPE_NONE = 0x00;
  const unsigned char DATASET_TYPE_UNIFORM = 0x01;
  const unsigned char DATASET_TYPE_RECTILINEAR = 0x02;
  const unsigned char DATASET_TYPE_UNSTRUCTURED_SINGLE = 0x08;
  const unsigned char DATASET_TYPE_UNSTRUCTURED = 0x10;
  const unsigned char DATASET_TYPE_ERROR = 0xFF;

protected:
  class GenericWriter;
  class UniformDataSetWriter;
  class RectilinearDataSetWriter;
  class UnstructuredSingleTypeDataSetWriter;
  class UnstructuredExplicitDataSetWriter;

  /// Determines the type of dataset based on the provided partitioned dataset.
  /// \param dataSets The partitioned dataset to analyze.
  void SetDataSetType(const viskores::cont::PartitionedDataSet& dataSets);

  /// Retrieves the dataset type for a given dataset.
  /// \param ds The dataset for which to retrieve the type.
  /// \return returns one of DATASET_TYPE_*
  unsigned char GetDataSetType(const viskores::cont::DataSet& ds);

  /// Implements the actual writing logic for the dataset.
  /// \param dataSets The partitioned dataset to write.
  /// \param outputMode The mode in which to write the dataset.
  void WriteImpl(const viskores::cont::PartitionedDataSet& dataSets, const std::string& outputMode);

  std::string OutputFile;
  unsigned char DataSetType;
  std::set<std::string> FieldsToWrite;
  bool WriteFieldSet;
#ifdef FIDES_USE_MPI
  MPI_Comm Comm = MPI_COMM_WORLD;
#endif
  std::string AdiosConfigFile;
};

/// \brief General purpose writer for data described by an Fides data model.
///
/// \c fides::io::DataSetAppendWriter writes a viskores dataset to a file using
/// ADIOS's append mode.
/// This class provides functionality to configure the writing process,
/// including setting the ADIOS configuration file and specifying which fields
/// to write. When MPI is enabled, it also supports passing a communicator, with
/// the default being MPI_COMM_WORLD.
class FIDES_EXPORT DataSetAppendWriter : public DataSetWriter
{
public:
  DataSetAppendWriter(const std::string& outputFile);
#ifdef FIDES_USE_MPI
  DataSetAppendWriter(const std::string& outputFile, MPI_Comm comm);
#endif
  ~DataSetAppendWriter() = default;

  /// Writes the given partitioned dataset to the output file.
  /// \param dataSets The partitioned dataset to write.
  /// \param outputMode The mode in which to write the dataset (e.g., synchronous, asynchronous).
  void Write(const viskores::cont::PartitionedDataSet& dataSets, const std::string& outputMode);

  /// Writes the given partitioned dataset to the output file using default output mode.
  /// \param dataSets The partitioned dataset to write.
  void Write(const viskores::cont::PartitionedDataSet& dataSets);

  /// Closes the ADIOS engine
  void Close();

private:
  void Initialize(const viskores::cont::PartitionedDataSet& dataSets,
                  const std::string& outputMode);
  bool IsInitialized;
  std::shared_ptr<DataSetWriter::GenericWriter> Writer;
};

} // end namespace io
} // end namespace fides

#endif // fides_io_DataSetWriter_h
