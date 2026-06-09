//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/FidesDataSetWriter.h>
#include <fides/FidesWriter.h>

#if FIDES_USE_VTK
#include <fides/vtk/FidesDataSetWriterVTK.h>
#include <vtkPartitionedDataSet.h>
#include <vtkSmartPointer.h>
#endif

#if FIDES_USE_VISKORES
#include <fides/viskores/FidesDataSetWriterViskores.h>
#include <viskores/cont/PartitionedDataSet.h>
#endif

#include <map>
#include <set>
#include <stdexcept>

namespace fides
{
namespace io
{

FidesDataSetWriter::FidesDataSetWriter(const std::string& outputFile, const std::string& outputMode)
  : Writer(new FidesWriter(outputFile, outputMode))
{
}

#if FIDES_USE_MPI
FidesDataSetWriter::FidesDataSetWriter(const std::string& outputFile,
                                       MPI_Comm comm,
                                       const std::string& outputMode)
  : Writer(new FidesWriter(outputFile, comm, outputMode))
{
}
#endif

FidesDataSetWriter::~FidesDataSetWriter() = default;

void FidesDataSetWriter::SetAdiosConfigFile(const std::string& adiosConfigPath)
{
  this->Writer->SetAdiosConfigFile(adiosConfigPath);
}

void FidesDataSetWriter::SetEngineParameters(
  const std::unordered_map<std::string, std::string>& params)
{
  // FidesWriter::SetEngineParameters takes std::map; convert.
  std::map<std::string, std::string> m(params.begin(), params.end());
  this->Writer->SetEngineParameters(m);
}

void FidesDataSetWriter::SetWriteFields(const std::vector<std::string>& fieldNames)
{
  std::set<std::string> s(fieldNames.begin(), fieldNames.end());
  this->FieldsToWrite = s;
  this->WriteAll = false;
  this->Writer->SetWriteFields(s);
}

void FidesDataSetWriter::SetWriteAllFields(bool writeAll)
{
  this->WriteAll = writeAll;
  if (writeAll)
  {
    this->FieldsToWrite.clear();
  }
  this->Writer->SetWriteAllFields(writeAll);
}

void FidesDataSetWriter::BeginStep()
{
  this->Writer->BeginStep();
}

void FidesDataSetWriter::SetCurrentTime(double t)
{
  this->Writer->SetCurrentTime(t);
}

void FidesDataSetWriter::Write(const fides::VTKCollection& dataSets)
{
#if FIDES_USE_VTK
  auto extraction = fides::ExtractVTKPartitions(
    dataSets.Get(), this->WriteAll ? std::set<std::string>() : this->FieldsToWrite);
  // ExtractVTKPartitions throws if both kinds are present in one step,
  // so exactly one of these vectors is non-empty here (or both are
  // empty, which also means "nothing to do").
  if (!extraction.CellGrids.empty())
  {
    this->Writer->Write(extraction.CellGrids);
  }
  else
  {
    this->Writer->Write(extraction.DataSets);
  }
#else
  (void)dataSets;
  throw std::runtime_error("Fides was not built with VTK support");
#endif
}

void FidesDataSetWriter::Write(const fides::ViskoresCollection& dataSets)
{
#if FIDES_USE_VISKORES
  auto partitions = fides::ExtractViskoresPartitions(
    dataSets, this->WriteAll ? std::set<std::string>() : this->FieldsToWrite);
  this->Writer->Write(partitions);
#else
  (void)dataSets;
  throw std::runtime_error("Fides was not built with Viskores support");
#endif
}

void FidesDataSetWriter::Write(fides::DataContainer& container)
{
#if FIDES_USE_VTK
  if (auto* vtk = fides::GetDataAs<vtkSmartPointer<vtkPartitionedDataSet>>(container))
  {
    this->Write(*vtk);
    return;
  }
#endif
#if FIDES_USE_VISKORES
  if (auto* vec = fides::GetDataAs<std::vector<viskores::cont::DataSet>>(container))
  {
    this->Write(viskores::cont::PartitionedDataSet(*vec));
    return;
  }
#endif
  throw std::runtime_error("DataContainer does not hold a supported VTK or Viskores collection");
}

void FidesDataSetWriter::EndStep()
{
  this->Writer->EndStep();
}

void FidesDataSetWriter::Close()
{
  this->Writer->Close();
}

bool FidesDataSetWriter::IsStepOpen() const
{
  return this->Writer->IsStepOpen();
}

} // namespace io
} // namespace fides
