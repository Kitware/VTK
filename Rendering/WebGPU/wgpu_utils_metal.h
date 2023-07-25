// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"
#include <memory>

static VTKRENDERINGWEBGPU_EXPORT std::unique_ptr<wgpu::ChainedStruct> VTKRENDERINGWEBGPU_EXPORT
SetupWindowAndGetSurfaceDescriptorCocoa(void* window);
