// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFWriterDistributedTestUtilities.h"

#include <utility>

using namespace HDFWriterDistributedTestUtilities;

int TestHDFWriterDistributedUGConfigs(int argc, char* argv[])
{
  std::vector<Configuration> allConfigs;
  // Test all configs with and without a polyhedron part
  for (auto config : GenerateConfigurations(MakeBaseConfigurations(DataType::UnstructuredGrid)))
  {
    allConfigs.push_back(config);
    config.Polyhedrons = PolyhedronsType::Polyhedrons;
    config.Name += "_polyhedrons";
    allConfigs.push_back(std::move(config));
  }

  return RunDistributedConfigurations(argc, argv, std::move(allConfigs)) ? EXIT_SUCCESS
                                                                         : EXIT_FAILURE;
}
