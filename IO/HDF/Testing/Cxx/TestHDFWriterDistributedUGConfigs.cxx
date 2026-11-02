// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFWriterDistributedTestUtilities.h"

#include <utility>

using namespace HDFWriterDistributedTestUtilities;

int TestHDFWriterDistributedUGConfigs(int argc, char* argv[])
{
  std::vector<Configuration> configs(2);
  configs[0].Name = "ug";
  configs[1].Name = "ug";
  configs[1].External = ExternalType::Distributed;
  return RunDistributedConfigurations(argc, argv, std::move(configs)) ? EXIT_SUCCESS : EXIT_FAILURE;
}
