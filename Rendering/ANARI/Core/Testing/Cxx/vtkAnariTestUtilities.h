// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkAnariTestUtilities_h
#define vtkAnariTestUtilities_h

class vtkAnariPass;
class vtkAnariRenderWindow;
class vtkRenderer;

namespace vtkAnariTestUtilities
{

void SetParameterDefaults(
  vtkAnariRenderWindow* renderWindow, bool useDebugDevice, const char* testName);

void SetParameterDefaults(
  vtkAnariPass*, vtkRenderer* renderer, bool useDebugDevice, const char* testName);

}

#endif
