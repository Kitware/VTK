// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDGOperatorEntry.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

std::string vtkDGOperatorEntry::GetShaderString(
  const std::string& functionName, const std::string& parameterName, int order) const
{
  std::ostringstream wrapper;
  int parameterSize = this->NumberOfFunctions * this->OperatorSize;
  wrapper << "void " << functionName << "(in vec3 param, "
          << "out float " << parameterName << "[" << parameterSize << "])\n"
          << "{\n"
          << "  RealT eps = 1.19209e-07;\n"
          << "  RealT rr = param.x;\n"
          << "  RealT ss = param.y;\n"
          << "  RealT tt = param.z;\n"
          << "  const int order = " << order << ";\n"
          << this->ShaderOp << "\n"
          << "}\n";
  return wrapper.str();
}

VTK_ABI_NAMESPACE_END
