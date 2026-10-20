// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HDFWriterDistributedTestUtilities_h
#define HDFWriterDistributedTestUtilities_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkSmartPointer.h"

#include <cstdint>
#include <string>
#include <vector>

/**
 * Utility header providing the functions to test the vtkHDFWriter using a custom customizable
 * source, that can generate UG, PD, composite data types.
 * Includes: Test runner, Configuration generator, and this source
 */
namespace HDFWriterDistributedTestUtilities
{
constexpr int NumberOfSteps = 4;

enum class DataType
{
  UnstructuredGrid,
  PolyData,
};
enum class TemporalType
{
  NoTemporal,
  Temporal,
};
enum class StaticMeshType
{
  NoStatic,
  Static,
};
enum class PolyhedronsType
{
  NoPolyhedrons,
  Polyhedrons
};
enum class CompositeType
{
  NoComposite,
  Multiblock,
  PDC,
};
enum ExternalType
{
  NoExternal = 0,
  Time = 1,
  Distributed = 2,
  Composite = 4,
  Partitions = 8,
};

struct Configuration
{
  std::string Name;
  std::vector<DataType> Types{ DataType::UnstructuredGrid };
  std::vector<uint8_t> PartitionCounts;
  CompositeType Composite = CompositeType::NoComposite;
  ExternalType External = ExternalType::NoExternal;
  TemporalType Temporal = TemporalType::NoTemporal;
  PolyhedronsType Polyhedrons = PolyhedronsType::NoPolyhedrons;
  StaticMeshType StaticMesh = StaticMeshType::NoStatic;
  int EmptyRank = -1;
};

/**
 * Build the custom source with given config
 */
vtkSmartPointer<vtkDataObjectAlgorithm> MakeConfigurationSource(const Configuration& config);

/**
 * Generate base configurations from the provided data type
 */
std::vector<Configuration> MakeBaseConfigurations(DataType type);

/**
 * From the base configurations, generate possible option combinations
 * using external options, time, static mesh.
 * We don't want a full cartesian product because some configurations don't actually make sense,
 * and we want to keep the time to run the test relatively low.
 */
std::vector<Configuration> GenerateConfigurations(const std::vector<Configuration>& bases);

/**
 * Run all configurations, log progress, return true if all passed.
 */
bool RunDistributedConfigurations(int argc, char* argv[], std::vector<Configuration> configs);

}

#endif
