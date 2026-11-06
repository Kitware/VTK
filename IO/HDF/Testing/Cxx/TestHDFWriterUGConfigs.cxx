// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFWriterDistributedTestUtilities.h"

#include <utility>

using namespace HDFWriterDistributedTestUtilities;

int TestHDFWriterUGConfigs(int argc, char* argv[])
{
  std::vector<Configuration> allConfigs =
    GenerateConfigurations(MakeBaseConfigurations(DataType::UnstructuredGrid));
  return RunDistributedConfigurations(argc, argv, std::move(allConfigs)) ? EXIT_SUCCESS
                                                                         : EXIT_FAILURE;
}
