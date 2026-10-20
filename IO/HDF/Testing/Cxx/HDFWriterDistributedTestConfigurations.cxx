// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFWriterDistributedTestUtilities.h"

#include "vtkStringFormatter.h"

#include <utility>

namespace HDFWriterDistributedTestUtilities
{
std::vector<Configuration> MakeBaseConfigurations(DataType type)
{
  const std::string name = type == DataType::PolyData ? "pd" : "ug";
  // 2 base configurations: not partitioned and partitioned
  Configuration base;
  base.Name = name;
  base.Types = { type };
  std::vector<Configuration> configs{ base };
  auto partitioned = base;
  partitioned.Name = "partitioned_" + name;
  partitioned.PartitionCounts = { 0, 5, 1 };
  configs.emplace_back(std::move(partitioned));

  // Make one rank without any point and cells, the first of the last
  for (int emptyRank : { 0, 2 })
  {
    auto empty = base;
    empty.Name = "empty_" + name + "_" + vtk::to_string(emptyRank);
    empty.EmptyRank = emptyRank;
    configs.emplace_back(std::move(empty));
  }
  return configs;
}

std::vector<Configuration> GenerateConfigurations(const std::vector<Configuration>& bases)
{
  std::vector<Configuration> configurations;
  for (const auto& base : bases)
  {
    for (const auto externalPartitions : { ExternalType::NoExternal, ExternalType::Partitions })
    {
      if (externalPartitions == ExternalType::Partitions && base.PartitionCounts.empty())
      {
        continue;
      }
      for (const auto distributed : { ExternalType::NoExternal, ExternalType::Distributed })
      {
        for (const auto temporal : { TemporalType::NoTemporal, TemporalType::Temporal })
        {
          for (const auto externalTime : { ExternalType::NoExternal, ExternalType::Time })
          {
            for (const auto staticMesh : { StaticMeshType::NoStatic, StaticMeshType::Static })
            {
              if (temporal == TemporalType::NoTemporal &&
                (externalTime == ExternalType::Time || staticMesh == StaticMeshType::Static))
              {
                continue;
              }
              const auto external =
                static_cast<ExternalType>(base.External | externalPartitions | externalTime);
              // Currently external distributed is forced by other external types
              if (distributed == ExternalType::Distributed && external != ExternalType::NoExternal)
              {
                continue;
              }
              auto config = base;
              config.Temporal = temporal;
              config.StaticMesh = staticMesh;
              config.External = static_cast<ExternalType>(external | distributed);
              config.Name += externalPartitions == ExternalType::Partitions ? "_extpart" : "";
              config.Name += distributed == ExternalType::Distributed ? "_extdistr" : "";
              config.Name += temporal == TemporalType::Temporal ? "_temp" : "";
              config.Name += externalTime == ExternalType::Time ? "_exttemp" : "";
              config.Name += staticMesh == StaticMeshType::Static ? "_static" : "";
              configurations.push_back(std::move(config));
            }
          }
        }
      }
    }
  }
  return configurations;
}
}
