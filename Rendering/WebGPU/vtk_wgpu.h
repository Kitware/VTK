// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtk_wgpu_h
#define vtk_wgpu_h

#ifdef __EMSCRIPTEN__
/*
 * We need an up-to-date version of Emscripten for the API support.
 */
#include "webgpu/webgpu_cpp.h"
#include <emscripten/html5_webgpu.h>
#if __EMSCRIPTEN_major__ == 1 &&                                                                   \
  (__EMSCRIPTEN_minor__ < 40 || (__EMSCRIPTEN_minor__ == 40 && __EMSCRIPTEN_tiny__ < 1))
#error "Emscripten 1.40.1 or higher required"
#endif
#elif defined(VTK_USE_DAWN_NATIVE)
#include "dawn/webgpu_cpp.h"
#endif // VTK_USE_DAWN_NATIVE

#endif // vtk_wgpu_h
