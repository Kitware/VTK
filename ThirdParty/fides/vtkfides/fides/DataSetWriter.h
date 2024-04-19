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

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/PartitionedDataSet.h>

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "fides_export.h"

namespace fides
{
namespace io
{

/// \brief General purpose writer for data described by an Fides data model.
///
/// \c fides::io::DataSetWriter writes a vtkm dataset to a file.
///

class FIDES_EXPORT DataSetWriter
{
public:
  DataSetWriter(const std::string& outputFile);
  ~DataSetWriter() = default;

  void Write(const vtkm::cont::PartitionedDataSet& dataSets, const std::string& outputMode);

  void SetWriteFields(const std::vector<std::string>& writeFields)
  {
    this->FieldsToWrite.clear();

    for (const auto& v : writeFields)
      this->FieldsToWrite.insert(v);
    this->WriteFieldSet = true;
  }

protected:
  class GenericWriter;
  class UniformDataSetWriter;
  class RectilinearDataSetWriter;
  class UnstructuredSingleTypeDataSetWriter;
  class UnstructuredExplicitDataSetWriter;

  const unsigned char DATASET_TYPE_NONE = 0x00;
  const unsigned char DATASET_TYPE_UNIFORM = 0x01;
  const unsigned char DATASET_TYPE_RECTILINEAR = 0x02;
  const unsigned char DATASET_TYPE_UNSTRUCTURED_SINGLE = 0x08;
  const unsigned char DATASET_TYPE_UNSTRUCTURED = 0x10;
  const unsigned char DATASET_TYPE_ERROR = 0xFF;

  void SetDataSetType(const vtkm::cont::PartitionedDataSet& dataSets);
  unsigned char GetDataSetType(const vtkm::cont::DataSet& ds);

  std::string OutputFile;
  unsigned char DataSetType;
  std::set<std::string> FieldsToWrite;
  bool WriteFieldSet;
};


class FIDES_EXPORT DataSetAppendWriter : public DataSetWriter
{
public:
  DataSetAppendWriter(const std::string& outputFile);
  ~DataSetAppendWriter() = default;

  void Write(const vtkm::cont::PartitionedDataSet& dataSets, const std::string& outputMode);
  void Close();

private:
  void Initialize(const vtkm::cont::PartitionedDataSet& dataSets, const std::string& outputMode);
  bool IsInitialized;
  std::shared_ptr<DataSetWriter::GenericWriter> Writer;
};

} // end namespace io
} // end namespace fides

#endif // fides_io_DataSetWriter_h
