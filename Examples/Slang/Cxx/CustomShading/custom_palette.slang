// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Constants that custom_shading.slang imports. VTK finds this file through the search path that
// CustomShading.cxx adds with vtkWebGPUShaderProperty::AddSlangSearchPath().
module custom_palette;

public static const float3 kStripeColor    = float3(0.08f, 0.65f, 0.9f);
public static const float  kStripesPerUnit = 6.f;
public static const float  kToonBands      = 4.f;
