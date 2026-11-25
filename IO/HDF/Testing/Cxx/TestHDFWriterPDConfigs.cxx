// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFWriterDistributedTestUtilities.h"

#include <utility>

using namespace HDFWriterDistributedTestUtilities;

int TestHDFWriterPDConfigs(int argc, char* argv[])
{
  auto configs = GenerateConfigurations(MakeBaseConfigurations(DataType::PolyData));
  return RunDistributedConfigurations(argc, argv, std::move(configs)) ? EXIT_SUCCESS : EXIT_FAILURE;
}
