//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_FidesDataSetWriter_H_
#define fides_FidesDataSetWriter_H_

#include <fides/FidesTypes.h>

#if FIDES_USE_MPI
#include <fides/fides_mpi.h>
#endif

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <fides/fides_export.h>


namespace fides
{

class FidesWriter;

namespace io
{

/// \brief Unified writer for VTK and Viskores datasets.
///
/// FidesDataSetWriter extracts data from VTK or Viskores datasets into
/// PartitionInfo and delegates writing to FidesWriter. Both Write overloads
/// are always declared; calling an overload for an absent backend throws
/// std::runtime_error.
///
/// Usage:
///   writer.BeginStep();
///   writer.Write(dataSets);
///   writer.EndStep();
///   writer.Close();
class FIDES_EXPORT FidesDataSetWriter
{
public:
  FidesDataSetWriter(const std::string& outputFile, const std::string& outputMode = "BPFile");
#if FIDES_USE_MPI
  FidesDataSetWriter(const std::string& outputFile,
                     MPI_Comm comm,
                     const std::string& outputMode = "BPFile");
#endif
  ~FidesDataSetWriter();

  // --- configuration (must be called before BeginStep) ---

  /// Set a path to an ADIOS2 XML configuration file.
  void SetAdiosConfigFile(const std::string& adiosConfigPath);

  /// Set ADIOS2 engine parameters.
  void SetEngineParameters(const std::unordered_map<std::string, std::string>& params);

  /// Set which fields to write. An empty vector means "write no fields" (geometry only).
  /// Duplicate entries are silently deduplicated.
  void SetWriteFields(const std::vector<std::string>& fieldNames);

  /// Set whether to write all fields. When true (the default), all fields are
  /// written regardless of any previous SetWriteFields filter. When false,
  /// no fields are written (geometry only).
  void SetWriteAllFields(bool writeAll);

  // --- write (always explicit step lifecycle) ---

  /// Open a new step. Must be called before Write().
  void BeginStep();

  /// Set the current time value for this step.
  void SetCurrentTime(double t);

  /// Write VTK datasets for the current step.
  void Write(const fides::VTKCollection& dataSets);

  /// Write Viskores datasets for the current step.
  void Write(const fides::ViskoresCollection& dataSets);

  /// Write datasets from a DataContainer (e.g., output from a reader).
  /// The container must hold a VTK or Viskores collection.
  void Write(fides::DataContainer& container);

  /// Close the current step.
  void EndStep();

  /// Close the writer.
  void Close();

  /// Returns true if a step is currently open.
  bool IsStepOpen() const;

private:
  std::unique_ptr<FidesWriter> Writer;
  std::set<std::string> FieldsToWrite;
  bool WriteAll = true;
};

} // namespace io
} // namespace fides

#endif // fides_FidesDataSetWriter_H_
