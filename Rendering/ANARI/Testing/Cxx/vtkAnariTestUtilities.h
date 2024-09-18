// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkAnariTestUtilities_h
#define vtkAnariTestUtilities_h

#include "vtkAnariPass.h"
#include "vtkAnariRendererNode.h"

void SetAnariRendererParameterDefaults(
  vtkAnariPass* pass, vtkRenderer* renderer, bool useDebugDevice, const char* testName);

#endif
