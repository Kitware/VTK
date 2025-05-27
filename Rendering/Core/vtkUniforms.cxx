// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUniforms.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkUniforms);

//------------------------------------------------------------------------------
void vtkUniforms::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkMTimeType vtkUniforms::GetUniformListMTime()
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return 0;
}

//------------------------------------------------------------------------------
std::string vtkUniforms::TupleTypeToString(TupleType tt)
{
  std::string str;
  switch (tt)
  {
    case vtkUniforms::TupleTypeScalar:
      str = "TupleTypeScalar";
      break;
    case vtkUniforms::TupleTypeVector:
      str = "TupleTypeVector";
      break;
    case vtkUniforms::TupleTypeMatrix:
      str = "TupleTypeMatrix";
      break;
    default:
      str = "TupleTypeInvalid";
      break;
  }
  return str;
}

//------------------------------------------------------------------------------
vtkUniforms::TupleType vtkUniforms::StringToTupleType(const std::string& s)
{
  if (s == "TupleTypeScalar")
  {
    return vtkUniforms::TupleTypeScalar;
  }
  else if (s == "TupleTypeVector")
  {
    return vtkUniforms::TupleTypeVector;
  }
  else if (s == "TupleTypeMatrix")
  {
    return vtkUniforms::TupleTypeMatrix;
  }
  return vtkUniforms::TupleTypeInvalid;
}

//------------------------------------------------------------------------------
std::string vtkUniforms::ScalarTypeToString(int scalarType)
{
  if (scalarType == VTK_INT)
  {
    return "int";
  }
  else if (scalarType == VTK_FLOAT)
  {
    return "float";
  }
  return "invalid";
}

//------------------------------------------------------------------------------
int vtkUniforms::StringToScalarType(const std::string& s)
{
  if (s == "int")
  {
    return VTK_INT;
  }
  else if (s == "float")
  {
    return VTK_FLOAT;
  }
  else
  {
    return VTK_VOID;
  }
}

//------------------------------------------------------------------------------
void vtkUniforms::RemoveUniform(const char*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::RemoveAllUniforms()
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform(const char*, vtkUniforms::TupleType, int, const std::vector<int>&)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform(const char*, vtkUniforms::TupleType, int, const std::vector<float>&)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform(const char*, std::vector<int>&)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform(const char*, std::vector<float>&)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniformi(const char*, int)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniformf(const char*, float)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform2i(const char*, const int[2])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform2f(const char*, const float[2])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform3f(const char*, const float[3])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform4f(const char*, const float[4])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniformMatrix3x3(const char*, float*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniformMatrix4x4(const char*, float*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform1iv(const char*, int, const int*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform1fv(const char*, int, const float*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform2fv(const char*, int, const float (*)[2])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform3fv(const char*, int, const float (*)[3])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform4fv(const char*, int, const float (*)[4])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniformMatrix4x4v(const char*, int, float*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform3f(const char*, const double[3])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform3uc(const char*, const unsigned char[3])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
} // maybe remove

//------------------------------------------------------------------------------
void vtkUniforms::SetUniform4uc(const char*, const unsigned char[4])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
} // maybe remove

//------------------------------------------------------------------------------
void vtkUniforms::SetUniformMatrix(const char*, vtkMatrix3x3*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
void vtkUniforms::SetUniformMatrix(const char*, vtkMatrix4x4*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniformi(const char*, int&)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniformf(const char*, float&)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform2i(const char*, int[2])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform2f(const char*, float[2])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform3f(const char*, float[3])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform4f(const char*, float[4])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniformMatrix3x3(const char*, float*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniformMatrix4x4(const char*, float*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform3f(const char*, double[3])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform3uc(const char*, unsigned char[3])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform4uc(const char*, unsigned char[4])
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniformMatrix(const char*, vtkMatrix3x3*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniformMatrix(const char*, vtkMatrix4x4*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform1iv(const char*, std::vector<int>&)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform1fv(const char*, std::vector<float>&)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform2fv(const char*, std::vector<float>&)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform3fv(const char*, std::vector<float>&)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniform4fv(const char*, std::vector<float>&)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
bool vtkUniforms::GetUniformMatrix4x4v(const char*, std::vector<float>&)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return false;
}

//------------------------------------------------------------------------------
int vtkUniforms::GetNumberOfUniforms()
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return 0;
}

//------------------------------------------------------------------------------
const char* vtkUniforms::GetNthUniformName(vtkIdType)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return "";
}

//------------------------------------------------------------------------------
int vtkUniforms::GetUniformScalarType(const char*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return 0;
}

//------------------------------------------------------------------------------
vtkUniforms::TupleType vtkUniforms::GetUniformTupleType(const char*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return vtkUniforms::TupleTypeInvalid;
}

//------------------------------------------------------------------------------
int vtkUniforms::GetUniformNumberOfComponents(const char*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return 0;
}

//------------------------------------------------------------------------------
int vtkUniforms::GetUniformNumberOfTuples(const char*)
{
  vtkWarningMacro(
    "vtkUniforms is not overriden by any derived class in the currently used rendering factory.");
  return 0;
}

VTK_ABI_NAMESPACE_END
