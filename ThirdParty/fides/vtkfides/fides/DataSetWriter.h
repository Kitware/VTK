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
///

class FIDES_EXPORT DataSetWriter
{
public:
  DataSetWriter(const std::string& outputFile);
#ifdef FIDES_USE_MPI
  DataSetWriter(const std::string& outputFile, MPI_Comm comm);
#endif
  ~DataSetWriter() = default;

  void Write(const viskores::cont::PartitionedDataSet& dataSets, const std::string& outputMode);

  void SetWriteFields(const std::vector<std::string>& writeFields)
  {
    this->FieldsToWrite.clear();

    for (const auto& v : writeFields)
      this->FieldsToWrite.insert(v);
    this->WriteFieldSet = true;
  }

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

  void SetDataSetType(const viskores::cont::PartitionedDataSet& dataSets);
  unsigned char GetDataSetType(const viskores::cont::DataSet& ds);

  std::string OutputFile;
  unsigned char DataSetType;
  std::set<std::string> FieldsToWrite;
  bool WriteFieldSet;
#ifdef FIDES_USE_MPI
  MPI_Comm Comm = MPI_COMM_WORLD;
#endif
};

class FIDES_EXPORT DataSetAppendWriter : public DataSetWriter
{
public:
  DataSetAppendWriter(const std::string& outputFile);
#ifdef FIDES_USE_MPI
  DataSetAppendWriter(const std::string& outputFile, MPI_Comm comm);
#endif
  ~DataSetAppendWriter() = default;

  void Write(const viskores::cont::PartitionedDataSet& dataSets, const std::string& outputMode);
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
