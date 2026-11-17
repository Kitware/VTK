// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFWriterDistributedTestUtilities.h"

#include <cstdlib>
#include <utility>

using namespace HDFWriterDistributedTestUtilities;

int TestHDFWriterDistributedCompositeConfigs(int argc, char* argv[])
{
  // Test composite writing with UG
  std::vector<Configuration> bases = MakeBaseConfigurations(DataType::UnstructuredGrid);
  std::vector<Configuration> configs;
  for (const auto& base : bases)
  {
    for (const auto composite : { CompositeType::Multiblock, CompositeType::PDC })
    {
      for (const auto distributed : { ExternalType::NoExternal, ExternalType::Distributed })
      {
        if (composite == CompositeType::Multiblock && !base.PartitionCounts.empty())
        {
          continue;
        }
        auto layout = base;
        layout.Composite = composite;
        layout.External = distributed;
        if (layout.Types.size() == 1)
        {
          layout.Types.push_back(layout.Types.front());
        }
        if (composite == CompositeType::PDC && layout.PartitionCounts.empty())
        {
          layout.PartitionCounts = { 1, 1, 1 };
        }
        layout.Name += composite == CompositeType::PDC ? "_pdc" : "_mb";
        layout.Name += distributed == ExternalType::Distributed ? "_extrdistr" : "";
        configs.push_back(std::move(layout));
      }
    }
  }
  return RunDistributedConfigurations(argc, argv, std::move(configs)) ? EXIT_SUCCESS : EXIT_FAILURE;
}
