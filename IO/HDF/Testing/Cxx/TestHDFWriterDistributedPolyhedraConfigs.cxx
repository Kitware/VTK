// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFWriterDistributedTestUtilities.h"

#include <utility>

using namespace HDFWriterDistributedTestUtilities;

int TestHDFWriterDistributedPolyhedraConfigs(int argc, char* argv[])
{
  std::vector<Configuration> allConfigs =
    GenerateConfigurations(MakeBaseConfigurations(DataType::UnstructuredGrid));
  for (auto& config : allConfigs)
  {
    config.Polyhedrons = PolyhedronsType::Polyhedrons;
    config.Name += "_polyhedrons";
  }

  return RunDistributedConfigurations(argc, argv, std::move(allConfigs)) ? EXIT_SUCCESS
                                                                         : EXIT_FAILURE;
}
