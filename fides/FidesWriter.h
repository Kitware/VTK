//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_FidesWriter_H_
#define fides_FidesWriter_H_

#include <fides/FidesTypes.h>

#if FIDES_USE_MPI
#include <fides/fides_mpi.h>
#endif

#include <fides/PartitionInfo.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <fides/fides_export.h>

namespace fides
{

/// \brief Framework-independent writer for Fides data.
///
/// FidesWriter writes data described by PartitionInfo to ADIOS2 files.
/// It generates the Fides schema automatically from PartitionInfo metadata,
/// handles MPI coordination, and supports multi-step append mode.
///
/// Usage:
///   writer.BeginStep();
///   writer.Write(partitions);
///   writer.EndStep();
///   writer.Close();
///
/// This is the core writer that VTK and Viskores wrappers delegate to.
class FIDES_EXPORT FidesWriter
{
public:
  FidesWriter(const std::string& outputFile, const std::string& outputMode = "BPFile");
#if FIDES_USE_MPI
  FidesWriter(const std::string& outputFile,
              MPI_Comm comm,
              const std::string& outputMode = "BPFile");
#endif
  ~FidesWriter();

  /// Open a new step. Must be called before Write().
  void BeginStep();

  /// Write partition data for the current step.
  /// BeginStep() must have been called first.
  void Write(const std::vector<PartitionInfo>& partitions);

  /// Write cellgrid partition data for the current step. A FidesWriter
  /// is single-kind: once one of the two Write overloads has been
  /// called, the other will throw to keep the schema consistent across
  /// steps. WriteCollection (below) further extends this to a third
  /// "collection" mode that cannot be mixed with either Write overload.
  void Write(const std::vector<CellGridPartitionInfo>& partitions);

  /// Write a multi-dataset (PDC) collection for the current step. Each
  /// item's partitions are written under the item name as an ADIOS2
  /// variable group prefix, and a single datasets[] schema (with the
  /// optional assembly tree) is emitted. \c assembly may be null. The
  /// collection path is single-kind: it cannot be mixed with either of
  /// the Write overloads on the same engine.
  ///
  /// File-size note: write-side array deduplication is not implemented.
  /// Each item's coordinates and cells are serialized to their own
  /// per-item ADIOS2 variables, so two items that share an identical
  /// mesh in memory still produce two copies on disk. If the items in
  /// a collection naturally share a mesh, expressing them as a single
  /// item carrying multiple fields will avoid the duplication; the
  /// PDC-of-mesh shape is intended for items whose coordinates and
  /// cells differ.
  void WriteCollection(const std::vector<CollectionItem>& items,
                       const AssemblyNode* assembly = nullptr);

  /// Close the current step. Must be called after Write() and before the next
  /// BeginStep() or Close().
  void EndStep();

  /// Close the engine. EndStep() must have been called before Close() if a
  /// step is still open.
  void Close();

  /// Returns true if a step is currently open (BeginStep has been called but
  /// EndStep has not yet been called).
  bool IsStepOpen() const;

  /// Set a path to an ADIOS2 XML configuration file.
  /// Must be called before the first BeginStep().
  void SetAdiosConfigFile(const std::string& configFile);

  /// Set ADIOS2 engine parameters.
  /// Must be called before the first BeginStep().
  void SetEngineParameters(const std::map<std::string, std::string>& params);

  /// Set the current time value. If called before Write(), the time is written
  /// as a step-scoped ADIOS2 scalar variable named "time".
  void SetCurrentTime(double t);

  /// Set the field filter. Only fields whose names are in this set will be
  /// written. An empty set means "write no fields" (geometry only).
  /// Must be called before the first BeginStep().
  void SetWriteFields(const std::set<std::string>& fields);

  /// Set whether to write all fields. When true (the default), all fields are
  /// written regardless of any previous SetWriteFields filter. When false,
  /// no fields are written (geometry only).
  /// Must be called before the first BeginStep().
  void SetWriteAllFields(bool writeAll);

  /// Generate a Fides JSON schema string from PartitionInfo metadata,
  /// without writing any file. Useful for inspecting the data layout.
  static std::string GenerateSchema(const std::vector<PartitionInfo>& partitions);

private:
  struct Impl;
  std::unique_ptr<Impl> PImpl;
};

} // namespace fides

#endif // fides_FidesWriter_H_
