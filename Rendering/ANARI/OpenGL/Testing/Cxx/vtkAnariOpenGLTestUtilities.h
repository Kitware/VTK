// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkAnariTestUtilities_h
#define vtkAnariTestUtilities_h

#include <anari/anari_cpp.hpp>

class vtkRenderer;
class vtkAnariPass;

namespace vtkAnariOpenGLTestUtilities
{

void SetParameterDefaults(
  vtkAnariPass* anariPass, vtkRenderer* renderer, bool useDebugDevice, const char* testName);

const anari::Extensions& GetDeviceExtensions(vtkAnariPass* anariPass);

}

#endif
