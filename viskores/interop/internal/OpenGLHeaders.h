//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_interop_internal_OpenGLHeaders_h
#define viskores_interop_internal_OpenGLHeaders_h

#include <viskores/internal/ExportMacros.h>

#if defined(__APPLE__)

#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifdef VISKORES_CUDA
#include <cuda_runtime.h>

#include <cuda_gl_interop.h>
#endif

#endif //viskores_interop_internal_OpenGLHeaders_h
