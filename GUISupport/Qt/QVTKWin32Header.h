// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#ifndef QVTKWin32Header_h
#define QVTKWin32Header_h

#include "vtkABI.h"
#include "vtkBuild.h"
#include "vtkSystemIncludes.h"

#if defined(VTK_BUILD_SHARED_LIBS)
#if defined(QVTK_EXPORTS)
#define QVTK_EXPORT VTK_ABI_EXPORT
#else
#define QVTK_EXPORT VTK_ABI_IMPORT
#endif
#else
#define QVTK_EXPORT
#endif

#endif /*QVTKWin32Header_h*/
