// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file vtk_wgpu_impl.h
 * @brief Internal WebGPU implementation header with C++ wrapper support
 *
 * This header is for INTERNAL implementation use only and should NOT be
 * included by public headers or user code. It combines the standard WebGPU
 * C API with the C++ convenience wrapper (wgpu:: namespace).
 *
 * Public headers should NOT include this file. If a public header needs
 * wgpu:: types, it should explicitly include the two headers below so that the
 * C++ dependency stays visible:
 *
 * @code{.cpp}
 * #include "vtk_wgpu.h"
 * #include "webgpu/webgpu_cpp.h"
 * @endcode
 *
 * Implementation pattern:
 *
 * @code{.cpp}
 * // Private implementation header
 * #include "vtk_wgpu_impl.h" // C + C++ API
 *
 * // Public header (uses C++ API, but documents it clearly)
 * #include "vtk_wgpu.h"          // C API
 * #include "webgpu/webgpu_cpp.h" // C++ wrapper (explicit dependency)
 * @endcode
 */

#ifndef vtk_wgpu_impl_h
#define vtk_wgpu_impl_h

// C API foundation (required by both C and C++ code)
#include "vtk_wgpu.h"

// C++ wrapper (INTERNAL USE ONLY)
// This header pulls in ~10k LOC of convenience C++ types and methods.
// It is only used internally by VTK's WebGPU implementation, not by the public API.
#include "webgpu/webgpu_cpp.h"

#endif // vtk_wgpu_impl_h
