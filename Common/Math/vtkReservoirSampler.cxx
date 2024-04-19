// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkReservoirSampler.h"

VTK_ABI_NAMESPACE_BEGIN
vtkReservoirSamplerBase::SeedType vtkReservoirSamplerBase::RandomSeed()
{
  VTK_THREAD_LOCAL std::random_device device;
  return device();
}
VTK_ABI_NAMESPACE_END
