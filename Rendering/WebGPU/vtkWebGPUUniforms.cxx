// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUUniforms.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"

#include <map>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
struct Kind
{
  int ScalarType = VTK_VOID;
  vtkUniforms::TupleType Tuple = vtkUniforms::TupleTypeInvalid;
  int Components = 0;
  bool Array = false;

  bool operator==(const Kind& o) const
  {
    return this->ScalarType == o.ScalarType && this->Tuple == o.Tuple &&
      this->Components == o.Components && this->Array == o.Array;
  }
};

struct Uniform
{
  Kind Type;
  std::vector<int> Ints;
  std::vector<float> Floats;

  size_t Size() const
  {
    return this->Type.ScalarType == VTK_INT ? this->Ints.size() : this->Floats.size();
  }
};

const Kind kScalari{ VTK_INT, vtkUniforms::TupleTypeScalar, 1, false };
const Kind kScalarf{ VTK_FLOAT, vtkUniforms::TupleTypeScalar, 1, false };
const Kind kVec2i{ VTK_INT, vtkUniforms::TupleTypeVector, 2, false };
const Kind kVec2f{ VTK_FLOAT, vtkUniforms::TupleTypeVector, 2, false };
const Kind kVec3f{ VTK_FLOAT, vtkUniforms::TupleTypeVector, 3, false };
const Kind kVec4f{ VTK_FLOAT, vtkUniforms::TupleTypeVector, 4, false };
const Kind kMat3f{ VTK_FLOAT, vtkUniforms::TupleTypeMatrix, 9, false };
const Kind kMat4f{ VTK_FLOAT, vtkUniforms::TupleTypeMatrix, 16, false };
const Kind k1iv{ VTK_INT, vtkUniforms::TupleTypeScalar, 1, true };
const Kind k1fv{ VTK_FLOAT, vtkUniforms::TupleTypeScalar, 1, true };
const Kind k2fv{ VTK_FLOAT, vtkUniforms::TupleTypeVector, 2, true };
const Kind k3fv{ VTK_FLOAT, vtkUniforms::TupleTypeVector, 3, true };
const Kind k4fv{ VTK_FLOAT, vtkUniforms::TupleTypeVector, 4, true };
const Kind kMat4fv{ VTK_FLOAT, vtkUniforms::TupleTypeMatrix, 16, true };
}

struct vtkWebGPUUniforms::Internals
{
  std::map<std::string, Uniform> Uniforms;
  vtkTimeStamp UniformListMTime;

  const Uniform* Find(const char* name, const Kind& kind) const
  {
    if (!name)
    {
      return nullptr;
    }
    auto it = this->Uniforms.find(name);
    return (it != this->Uniforms.end() && it->second.Type == kind) ? &it->second : nullptr;
  }

  const Uniform* Find(const char* name) const
  {
    if (!name)
    {
      return nullptr;
    }
    auto it = this->Uniforms.find(name);
    return it != this->Uniforms.end() ? &it->second : nullptr;
  }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUUniforms);

//------------------------------------------------------------------------------
vtkWebGPUUniforms::vtkWebGPUUniforms()
  : Storage(std::make_unique<Internals>())
{
}

//------------------------------------------------------------------------------
vtkWebGPUUniforms::~vtkWebGPUUniforms() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebGPUUniforms::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
  return renderingBackendAttribute;
}

namespace
{
template <typename T>
std::vector<T>& Values(Uniform& u);

//------------------------------------------------------------------------------
template <>
std::vector<int>& Values<int>(Uniform& u)
{
  return u.Ints;
}

//------------------------------------------------------------------------------
template <>
std::vector<float>& Values<float>(Uniform& u)
{
  return u.Floats;
}

//------------------------------------------------------------------------------
template <typename T>
const std::vector<T>& Values(const Uniform& u)
{
  return Values<T>(const_cast<Uniform&>(u));
}
}

#define vtkWebGPUUniformsSet(name, kind, T, first, count)                                          \
  do                                                                                               \
  {                                                                                                \
    if (!(name))                                                                                   \
    {                                                                                              \
      break;                                                                                       \
    }                                                                                              \
    auto it = this->Storage->Uniforms.find(name);                                                  \
    if (it == this->Storage->Uniforms.end())                                                       \
    {                                                                                              \
      Uniform u;                                                                                   \
      u.Type = (kind);                                                                             \
      Values<T>(u).assign((first), (first) + (count));                                             \
      this->Storage->Uniforms.emplace(name, std::move(u));                                         \
      this->Storage->UniformListMTime.Modified();                                                  \
      this->Modified();                                                                            \
    }                                                                                              \
    else if (it->second.Type == (kind))                                                            \
    {                                                                                              \
      Values<T>(it->second).assign((first), (first) + (count));                                    \
      this->Modified();                                                                            \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      vtkErrorMacro(<< "Trying to set the value of uniform variable of a different type: "         \
                    << (name));                                                                    \
    }                                                                                              \
  } while (false)

//------------------------------------------------------------------------------
vtkMTimeType vtkWebGPUUniforms::GetUniformListMTime()
{
  return this->Storage->UniformListMTime;
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::RemoveUniform(const char* name)
{
  if (name && this->Storage->Uniforms.erase(name))
  {
    this->Storage->UniformListMTime.Modified();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::RemoveAllUniforms()
{
  if (!this->Storage->Uniforms.empty())
  {
    this->Storage->Uniforms.clear();
    this->Storage->UniformListMTime.Modified();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform(
  const char* name, vtkUniforms::TupleType tt, int nbComponents, const std::vector<int>& value)
{
  if (tt != TupleTypeScalar || nbComponents != 1)
  {
    if (tt == TupleTypeVector && nbComponents == 2 && value.size() == 2)
    {
      vtkWebGPUUniformsSet(name, kVec2i, int, value.data(), value.size());
      return;
    }
    vtkErrorMacro(<< "Uniform type doesn't match input value.");
    return;
  }
  if (value.size() == 1)
  {
    vtkWebGPUUniformsSet(name, kScalari, int, value.data(), value.size());
  }
  else if (!value.empty())
  {
    vtkWebGPUUniformsSet(name, k1iv, int, value.data(), value.size());
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform(
  const char* name, vtkUniforms::TupleType tt, int nbComponents, const std::vector<float>& value)
{
  if (tt == TupleTypeInvalid || nbComponents <= 0 || value.empty() ||
    value.size() % static_cast<size_t>(nbComponents) != 0)
  {
    vtkErrorMacro(<< "Uniform type doesn't match input value.");
    return;
  }
  const bool array = value.size() > static_cast<size_t>(nbComponents);
  Kind kind{ VTK_FLOAT, tt, nbComponents, array };
  const bool supported = (tt == TupleTypeScalar && nbComponents == 1) ||
    (tt == TupleTypeVector && nbComponents >= 2 && nbComponents <= 4) ||
    (tt == TupleTypeMatrix && nbComponents == 9 && !array) ||
    (tt == TupleTypeMatrix && nbComponents == 16);
  if (!supported)
  {
    vtkErrorMacro(<< "Uniform type doesn't match input value.");
    return;
  }
  vtkWebGPUUniformsSet(name, kind, float, value.data(), value.size());
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform(const char* name, std::vector<int>& value)
{
  const Uniform* u = this->Storage->Find(name);
  if (!u || u->Type.ScalarType != VTK_INT)
  {
    return false;
  }
  value = u->Ints;
  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform(const char* name, std::vector<float>& value)
{
  const Uniform* u = this->Storage->Find(name);
  if (!u || u->Type.ScalarType != VTK_FLOAT)
  {
    return false;
  }
  value = u->Floats;
  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniformi(const char* name, int v)
{
  vtkWebGPUUniformsSet(name, kScalari, int, &v, 1);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniformf(const char* name, float v)
{
  vtkWebGPUUniformsSet(name, kScalarf, float, &v, 1);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform2i(const char* name, const int v[2])
{
  vtkWebGPUUniformsSet(name, kVec2i, int, v, 2);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform2f(const char* name, const float v[2])
{
  vtkWebGPUUniformsSet(name, kVec2f, float, v, 2);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform3f(const char* name, const float v[3])
{
  vtkWebGPUUniformsSet(name, kVec3f, float, v, 3);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform4f(const char* name, const float v[4])
{
  vtkWebGPUUniformsSet(name, kVec4f, float, v, 4);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniformMatrix3x3(const char* name, float* v)
{
  vtkWebGPUUniformsSet(name, kMat3f, float, v, 9);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniformMatrix4x4(const char* name, float* v)
{
  vtkWebGPUUniformsSet(name, kMat4f, float, v, 16);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform1iv(const char* name, int count, const int* f)
{
  vtkWebGPUUniformsSet(name, k1iv, int, f, count);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform1fv(const char* name, int count, const float* f)
{
  vtkWebGPUUniformsSet(name, k1fv, float, f, count);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform2fv(const char* name, int count, const float (*f)[2])
{
  vtkWebGPUUniformsSet(name, k2fv, float, f[0], 2 * count);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform3fv(const char* name, int count, const float (*f)[3])
{
  vtkWebGPUUniformsSet(name, k3fv, float, f[0], 3 * count);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform4fv(const char* name, int count, const float (*f)[4])
{
  vtkWebGPUUniformsSet(name, k4fv, float, f[0], 4 * count);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniformMatrix4x4v(const char* name, int count, float* v)
{
  vtkWebGPUUniformsSet(name, kMat4fv, float, v, 16 * count);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform3f(const char* name, const double v[3])
{
  const float f[3] = { static_cast<float>(v[0]), static_cast<float>(v[1]),
    static_cast<float>(v[2]) };
  this->SetUniform3f(name, f);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform3uc(const char* name, const unsigned char v[3])
{
  const float f[3] = { v[0] / 255.0f, v[1] / 255.0f, v[2] / 255.0f };
  this->SetUniform3f(name, f);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniform4uc(const char* name, const unsigned char v[4])
{
  const float f[4] = { v[0] / 255.0f, v[1] / 255.0f, v[2] / 255.0f, v[3] / 255.0f };
  this->SetUniform4f(name, f);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniformMatrix(const char* name, vtkMatrix3x3* v)
{
  float f[9];
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      f[i * 3 + j] = static_cast<float>(v->GetElement(i, j));
    }
  }
  this->SetUniformMatrix3x3(name, f);
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::SetUniformMatrix(const char* name, vtkMatrix4x4* v)
{
  float f[16];
  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      f[i * 4 + j] = static_cast<float>(v->GetElement(i, j));
    }
  }
  this->SetUniformMatrix4x4(name, f);
}

namespace
{
//------------------------------------------------------------------------------
template <typename T>
bool CopyOut(const Uniform* u, T* v)
{
  if (!u)
  {
    return false;
  }
  const std::vector<T>& values = Values<T>(*u);
  std::copy(values.begin(), values.end(), v);
  return true;
}

//------------------------------------------------------------------------------
template <typename T>
bool CopyOut(const Uniform* u, std::vector<T>& v)
{
  if (!u)
  {
    return false;
  }
  v = Values<T>(*u);
  return true;
}
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniformi(const char* name, int& v)
{
  return CopyOut(this->Storage->Find(name, kScalari), &v);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniformf(const char* name, float& v)
{
  return CopyOut(this->Storage->Find(name, kScalarf), &v);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform2i(const char* name, int v[2])
{
  return CopyOut(this->Storage->Find(name, kVec2i), v);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform2f(const char* name, float v[2])
{
  return CopyOut(this->Storage->Find(name, kVec2f), v);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform3f(const char* name, float v[3])
{
  return CopyOut(this->Storage->Find(name, kVec3f), v);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform4f(const char* name, float v[4])
{
  return CopyOut(this->Storage->Find(name, kVec4f), v);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniformMatrix3x3(const char* name, float* v)
{
  return CopyOut(this->Storage->Find(name, kMat3f), v);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniformMatrix4x4(const char* name, float* v)
{
  return CopyOut(this->Storage->Find(name, kMat4f), v);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform3f(const char* name, double v[3])
{
  float f[3];
  if (!this->GetUniform3f(name, f))
  {
    return false;
  }
  std::copy(f, f + 3, v);
  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform3uc(const char* name, unsigned char v[3])
{
  float f[3];
  if (!this->GetUniform3f(name, f))
  {
    return false;
  }
  for (int i = 0; i < 3; ++i)
  {
    v[i] = static_cast<unsigned char>(f[i] * 255.0f);
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform4uc(const char* name, unsigned char v[4])
{
  float f[4];
  if (!this->GetUniform4f(name, f))
  {
    return false;
  }
  for (int i = 0; i < 4; ++i)
  {
    v[i] = static_cast<unsigned char>(f[i] * 255.0f);
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniformMatrix(const char* name, vtkMatrix3x3* v)
{
  float f[9];
  if (!this->GetUniformMatrix3x3(name, f))
  {
    return false;
  }
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      v->SetElement(i, j, f[i * 3 + j]);
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniformMatrix(const char* name, vtkMatrix4x4* v)
{
  float f[16];
  if (!this->GetUniformMatrix4x4(name, f))
  {
    return false;
  }
  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      v->SetElement(i, j, f[i * 4 + j]);
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform1iv(const char* name, std::vector<int>& f)
{
  return CopyOut(this->Storage->Find(name, k1iv), f);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform1fv(const char* name, std::vector<float>& f)
{
  return CopyOut(this->Storage->Find(name, k1fv), f);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform2fv(const char* name, std::vector<float>& f)
{
  return CopyOut(this->Storage->Find(name, k2fv), f);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform3fv(const char* name, std::vector<float>& f)
{
  return CopyOut(this->Storage->Find(name, k3fv), f);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniform4fv(const char* name, std::vector<float>& f)
{
  return CopyOut(this->Storage->Find(name, k4fv), f);
}

//------------------------------------------------------------------------------
bool vtkWebGPUUniforms::GetUniformMatrix4x4v(const char* name, std::vector<float>& f)
{
  return CopyOut(this->Storage->Find(name, kMat4fv), f);
}

//------------------------------------------------------------------------------
int vtkWebGPUUniforms::GetNumberOfUniforms()
{
  return static_cast<int>(this->Storage->Uniforms.size());
}

//------------------------------------------------------------------------------
const char* vtkWebGPUUniforms::GetNthUniformName(vtkIdType uniformIndex)
{
  if (uniformIndex < 0 || uniformIndex >= this->GetNumberOfUniforms())
  {
    return nullptr;
  }
  auto it = this->Storage->Uniforms.begin();
  std::advance(it, uniformIndex);
  return it->first.c_str();
}

//------------------------------------------------------------------------------
int vtkWebGPUUniforms::GetUniformScalarType(const char* name)
{
  const Uniform* u = this->Storage->Find(name);
  return u ? u->Type.ScalarType : VTK_VOID;
}

//------------------------------------------------------------------------------
vtkUniforms::TupleType vtkWebGPUUniforms::GetUniformTupleType(const char* name)
{
  const Uniform* u = this->Storage->Find(name);
  return u ? u->Type.Tuple : TupleTypeInvalid;
}

//------------------------------------------------------------------------------
int vtkWebGPUUniforms::GetUniformNumberOfComponents(const char* name)
{
  const Uniform* u = this->Storage->Find(name);
  return u ? u->Type.Components : 0;
}

//------------------------------------------------------------------------------
int vtkWebGPUUniforms::GetUniformNumberOfTuples(const char* name)
{
  const Uniform* u = this->Storage->Find(name);
  return u ? static_cast<int>(u->Size() / static_cast<size_t>(u->Type.Components)) : 0;
}

//------------------------------------------------------------------------------
void vtkWebGPUUniforms::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for (const auto& it : this->Storage->Uniforms)
  {
    os << indent << it.first << ":";
    if (it.second.Type.ScalarType == VTK_INT)
    {
      for (int v : it.second.Ints)
      {
        os << " " << v;
      }
    }
    else
    {
      for (float v : it.second.Floats)
      {
        os << " " << v;
      }
    }
    os << "\n";
  }
}

VTK_ABI_NAMESPACE_END
