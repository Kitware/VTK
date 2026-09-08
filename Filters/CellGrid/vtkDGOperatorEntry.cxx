// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDGOperatorEntry.h"
#include "vtkStringFormatter.h"

#include <vtksys/SystemTools.hxx>

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

namespace
{

/// Replace every occurrence of `order[axis]` in \a source with a literal integer.
///
/// GLSL requires array sizes to be constant expressions, so the order an
/// arbitrary-order basis is bound to must be baked into the shader source
/// rather than passed in as a variable. Substituting the literal is enough:
/// a workspace declared as `WORKSPACE(RealT, rtmp, order[0] + 1)` becomes
/// `RealT rtmp[3 + 1];`, whose size is a constant expression.
std::string SubstituteOrder(const std::string& source, const std::vector<int>& order)
{
  std::string result = source;
  for (std::size_t axis = 0; axis < order.size(); ++axis)
  {
    vtksys::SystemTools::ReplaceString(
      result, "order[" + vtk::to_string(axis) + "]", vtk::to_string(order[axis]));
  }
  return result;
}

} // anonymous namespace

int vtkDGOperatorEntry::TensorProductFunctionCount(const std::vector<int>& order)
{
  int count = 1;
  for (int axisOrder : order)
  {
    if (axisOrder < 0)
    {
      return 0;
    }
    count *= axisOrder + 1;
  }
  return count;
}

int vtkDGOperatorEntry::SimplexFunctionCount(const std::vector<int>& order)
{
  if (order.empty() || order[0] < 0)
  {
    return 0;
  }
  int count = 1;
  for (std::size_t axis = 0; axis < order.size(); ++axis)
  {
    if (order[axis] != order[0])
    {
      return 0;
    }
    count = count * (order[0] + static_cast<int>(axis) + 1) / (static_cast<int>(axis) + 1);
  }
  return count;
}

int vtkDGOperatorEntry::WedgeFunctionCount(const std::vector<int>& order)
{
  if (order.size() != 3 || order[0] < 0 || order[2] < 0 || order[1] != order[0])
  {
    return 0;
  }
  return (order[0] + 1) * (order[0] + 2) / 2 * (order[2] + 1);
}

bool vtkDGOperatorEntry::SetOrder(const std::vector<int>& order)
{
  if (!this->FunctionCount)
  {
    // Operators implementing a single fixed order always know their own size.
    return this->NumberOfFunctions > 0;
  }
  this->Order = order;
  this->NumberOfFunctions = this->FunctionCount(this->Order);
  return this->NumberOfFunctions > 0;
}

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
          << "  const int order = " << order << ";\n";
  if (this->IsOrderDependent())
  {
    wrapper << "#define WORKSPACE(type, name, size) type name[size]\n"
            << SubstituteOrder(this->ShaderOp, this->Order) << "\n"
            << "#undef WORKSPACE\n";
  }
  else
  {
    wrapper << this->ShaderOp << "\n";
  }
  wrapper << "}\n";
  return wrapper.str();
}

VTK_ABI_NAMESPACE_END
