// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkAnariTestUtilities_h
#define vtkAnariTestUtilities_h

class vtkRenderer;
class vtkRenderWindow;

namespace vtkAnariTestUtilities
{

void SetParameterDefaults(vtkRenderWindow* renderWindow, bool useDebugDevice, const char* testName);

}

#endif
