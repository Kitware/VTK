#include "vtkOpenGLUniforms.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkShaderProgram.h"
#include <cmath>
#include <cstring>
#include <vector>

vtkStandardNewMacro(vtkOpenGLUniforms);

// temporary patch: Some Android builds don't have std::to_string
#include <sstream>
namespace patch
{
template <typename T>
std::string to_string(const T& n)
{
  std::ostringstream stm;
  stm << n;
  return stm.str();
}
}

class Uniform
{
public:
  virtual ~Uniform() {}
  virtual int GetScalarType() = 0;
  virtual vtkIdType GetNumberOfTuples() = 0;
  virtual vtkUniforms::TupleType GetTupleType() = 0;
  virtual int GetNumberOfComponents() = 0;
  virtual std::string GetGlslDeclaration(const char* name) = 0;
  virtual bool SetUniform(const char* name, vtkShaderProgram*) = 0;
  virtual bool GetGenericValue(std::vector<int>& value) = 0;
  virtual bool GetGenericValue(std::vector<float>& value) = 0;
  virtual void PrintSelf(const char* name, ostream&, vtkIndent) = 0;
};

template <typename scalarType, vtkUniforms::TupleType tupleType, int nbComponents>
class UniformT : public Uniform
{
public:
  UniformT() {}
  vtkIdType GetNumberOfTuples() override
  {
    return static_cast<vtkIdType>(values.size() / nbComponents);
  }
  vtkUniforms::TupleType GetTupleType() override { return tupleType; }
  int GetNumberOfComponents() override { return nbComponents; }
  void SetValue(scalarType value)
  {
    values.resize(1);
    values[0] = value;
  }
  void SetValue(const std::vector<scalarType>& value) { values = value; }
  const std::vector<scalarType>& GetValue() { return values; }
  void PrintSelf(const char* name, ostream& os, vtkIndent indent) override
  {
    os << indent << name << ":";
    vtkIndent inNext = indent.GetNextIndent();
    for (int i = 0; i < GetNumberOfTuples(); ++i)
    {
      PrintOne(i, os, inNext);
      if (i < GetNumberOfTuples() - 1)
      {
        os << std::endl << inNext;
      }
    }
  }

protected:
  void PrintOne(vtkIdType tupleIndex, ostream& os, vtkIndent indent)
  {
    int nbComp = GetNumberOfComponents();
    if (GetTupleType() == vtkUniforms::TupleTypeScalar)
    {
      os << values[tupleIndex] << std::endl;
    }
    else if (GetTupleType() == vtkUniforms::TupleTypeVector)
    {
      PrintVec(tupleIndex * nbComp, nbComp, os);
    }
    else
    {
      int w = static_cast<int>(sqrt(nbComp));
      for (int i = 0; i < w; ++i)
      {
        PrintVec((tupleIndex + i) * nbComp, nbComp, os);
        if (i < w - 1)
        {
          os << std::endl << indent;
        }
      }
    }
  }
  void PrintVec(vtkIdType index, int nbComp, ostream& os)
  {
    os << "[ ";
    for (int i = 0; i < nbComp - 1; ++i)
    {
      os << values[index + i] << ", ";
    }
    os << values[index + nbComp - 1] << " ]";
  }
  std::vector<scalarType> values;
};

template <vtkUniforms::TupleType tupleType, int nbComponents>
class Uniformi : public UniformT<int, tupleType, nbComponents>
{
public:
  int GetScalarType() override { return VTK_INT; }
  bool GetGenericValue(std::vector<int>& value) override
  {
    value = this->values;
    return true;
  }
  bool GetGenericValue(std::vector<float>&) override { return false; }
};

template <vtkUniforms::TupleType tupleType, int nbComponents>
class Uniformf : public UniformT<float, tupleType, nbComponents>
{
public:
  int GetScalarType() override { return VTK_FLOAT; }
  bool GetGenericValue(std::vector<int>&) override { return false; }
  bool GetGenericValue(std::vector<float>& value) override
  {
    value = this->values;
    return true;
  }
};

class UniformScalari : public Uniformi<vtkUniforms::TupleTypeScalar, 1>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform int ") + name + ";\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniformi(name, values[0]);
  }
  void PrintSelf(const char* name, ostream& os, vtkIndent indent) override
  {
    os << indent << name << ": " << values[0] << std::endl;
  }
};

class UniformScalarf : public Uniformf<vtkUniforms::TupleTypeScalar, 1>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform float ") + name + ";\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniformf(name, values[0]);
  }
};

class UniformVec2i : public Uniformi<vtkUniforms::TupleTypeVector, 2>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform ivec2 ") + name + ";\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniform2i(name, values.data());
  }
};

class UniformVec2f : public Uniformf<vtkUniforms::TupleTypeVector, 2>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform vec2 ") + name + ";\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniform2f(name, values.data());
  }
};

class UniformVec3f : public Uniformf<vtkUniforms::TupleTypeVector, 3>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform vec3 ") + name + ";\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniform3f(name, values.data());
  }
};

class UniformVec4f : public Uniformf<vtkUniforms::TupleTypeVector, 4>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform vec4 ") + name + ";\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniform4f(name, values.data());
  }
};

class UniformMat3f : public Uniformf<vtkUniforms::TupleTypeMatrix, 9>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform mat3 ") + name + ";\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniformMatrix3x3(name, values.data());
  }
};

class UniformMat4f : public Uniformf<vtkUniforms::TupleTypeMatrix, 16>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform mat4 ") + name + ";\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniformMatrix4x4(name, values.data());
  }
};

class Uniform1iv : public Uniformi<vtkUniforms::TupleTypeScalar, 1>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform int ") + name + "[" + patch::to_string(GetNumberOfTuples()) +
      "];\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniform1iv(name, GetNumberOfTuples(), values.data());
  }
};

class Uniform1fv : public Uniformf<vtkUniforms::TupleTypeScalar, 1>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform float ") + name + "[" + patch::to_string(GetNumberOfTuples()) +
      "];\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniform1fv(name, GetNumberOfTuples(), values.data());
  }
};

class Uniform2fv : public Uniformf<vtkUniforms::TupleTypeVector, 2>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform vec2 ") + name + "[" + patch::to_string(GetNumberOfTuples()) +
      "];\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniform2fv(
      name, GetNumberOfTuples(), reinterpret_cast<const float(*)[2]>(values.data()));
  }
};

class Uniform3fv : public Uniformf<vtkUniforms::TupleTypeVector, 3>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform vec3 ") + name + "[" + patch::to_string(GetNumberOfTuples()) +
      "];\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniform3fv(
      name, GetNumberOfTuples(), reinterpret_cast<const float(*)[3]>(values.data()));
  }
};

class Uniform4fv : public Uniformf<vtkUniforms::TupleTypeVector, 4>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform vec4 ") + name + "[" + patch::to_string(GetNumberOfTuples()) +
      "];\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniform4fv(
      name, GetNumberOfTuples(), reinterpret_cast<const float(*)[4]>(values.data()));
  }
};

class UniformMat4fv : public Uniformf<vtkUniforms::TupleTypeMatrix, 16>
{
public:
  std::string GetGlslDeclaration(const char* name) override
  {
    return std::string("uniform mat4 ") + name + "[" + patch::to_string(GetNumberOfTuples()) +
      "];\n";
  }
  bool SetUniform(const char* name, vtkShaderProgram* p) override
  {
    return p->SetUniformMatrix4x4v(name, GetNumberOfTuples(), values.data());
  }
};

class vtkUniformInternals : public vtkObject
{

public:
  static vtkUniformInternals* New();
  vtkTypeMacro(vtkUniformInternals, vtkObject);

  void SetParent(vtkOpenGLUniforms* uni) { this->Parent = uni; }

  vtkMTimeType GetUniformListMTime() { return this->UniformListMTime; }

  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    for (auto& uni : this->Uniforms)
    {
      uni.second->PrintSelf(uni.first.c_str(), os, indent);
    }
  }

  void RemoveUniform(const char* name)
  {
    UniformMap::iterator it = this->Uniforms.find(name);
    if (it != this->Uniforms.end())
    {
      delete (*it).second;
      this->Uniforms.erase(it);
    }
    this->UniformListMTime.Modified();
    this->Parent->Modified();
  }

  void RemoveAllUniforms()
  {
    for (auto& uni : this->Uniforms)
    {
      delete uni.second;
    }
    this->Uniforms.clear();
    this->UniformListMTime.Modified();
    this->Parent->Modified();
  }

  template <class dataT, class uniformT>
  void SetUniformValue(const char* name, const dataT& value)
  {
    UniformMap::iterator it = this->Uniforms.find(name);
    uniformT* uni = nullptr;
    if (it != this->Uniforms.end())
    {
      uni = dynamic_cast<uniformT*>(it->second);
      if (uni)
      {
        uni->SetValue(value);
        Parent->Modified();
      }
      else
      {
        vtkErrorMacro(<< "Trying to set the value of uniform variable of a different type: " << name
                      << endl);
      }
    }
    else
    {
      uni = new uniformT;
      uni->SetValue(value);
      this->Uniforms[std::string(name)] = uni;
      this->UniformListMTime.Modified();
      this->Parent->Modified();
    }
  }

  template <class uniformT>
  uniformT* GetUniform(const char* name)
  {
    UniformMap::iterator it = this->Uniforms.find(name);
    if (it != this->Uniforms.end())
    {
      return dynamic_cast<uniformT*>(it->second);
    }
    return nullptr;
  }

  template <typename scalarT, typename uniformT>
  bool GetUniformValue(const char* name, scalarT* v)
  {
    uniformT* uni = GetUniform<uniformT>(name);
    if (uni)
    {
      std::copy(uni->GetValue().begin(), uni->GetValue().end(), v);
      return true;
    }
    return false;
  }

  template <typename scalarT, typename uniformT>
  bool GetUniformValue(const char* name, std::vector<scalarT>& v)
  {
    uniformT* uni = GetUniform<uniformT>(name);
    if (uni)
    {
      v = uni->GetValue();
      return true;
    }
    return false;
  }

  // In this case, get the uniform value based only on the scalar type
  // without any guaranty on the tuple type and number of components
  // This function is used by generic getters.
  template <typename scalarT>
  bool GetGenericUniformValue(const char* name, std::vector<scalarT>& v)
  {
    UniformMap::iterator it = this->Uniforms.find(name);
    if (it != this->Uniforms.end())
    {
      Uniform* uni = it->second;
      return uni->GetGenericValue(v);
    }
    return false;
  }

  std::string GetDeclarations()
  {
    std::string res;
    for (auto& uni : this->Uniforms)
    {
      res += uni.second->GetGlslDeclaration(uni.first.c_str());
    }
    return res;
  }

  bool SetUniforms(vtkShaderProgram* p)
  {
    bool res = true;
    for (auto& uni : this->Uniforms)
    {
      bool r = uni.second->SetUniform(uni.first.c_str(), p);
      if (!r)
      {
        vtkErrorMacro(<< "vtkOpenGLUniforms: couldn't set custom uniform variable " << uni.first
                      << endl);
      }
      res &= r;
    }
    return res;
  }

  int GetNumberOfUniforms() { return static_cast<int>(this->Uniforms.size()); }

  const char* GetNthUniformName(vtkIdType uniformIndex)
  {
    if (uniformIndex >= this->GetNumberOfUniforms())
    {
      return nullptr;
    }

    UniformMap::iterator it = this->Uniforms.begin();
    std::advance(it, uniformIndex);
    return it->first.c_str();
  }

  Uniform* GetUniform(const char* name)
  {
    UniformMap::iterator it = this->Uniforms.find(name);
    if (it != this->Uniforms.end())
    {
      return it->second;
    }
    return nullptr;
  }

  int GetUniformScalarType(const char* name)
  {
    Uniform* uniform = this->GetUniform(name);
    if (uniform == nullptr)
    {
      return VTK_VOID;
    }
    return uniform->GetScalarType();
  }

  vtkUniforms::TupleType GetUniformTupleType(const char* name)
  {
    Uniform* uniform = this->GetUniform(name);
    if (uniform == nullptr)
    {
      return vtkUniforms::TupleTypeInvalid;
    }
    return uniform->GetTupleType();
  }

  int GetNumberOfTuples(const char* name)
  {
    Uniform* uniform = this->GetUniform(name);
    if (uniform == nullptr)
    {
      return 0;
    }
    return uniform->GetNumberOfTuples();
  }

  int GetUniformNumberOfComponents(const char* name)
  {
    Uniform* uniform = this->GetUniform(name);
    if (uniform == nullptr)
    {
      return 0;
    }
    return uniform->GetNumberOfComponents();
  }

protected:
  vtkUniformInternals() {}
  ~vtkUniformInternals() override { RemoveAllUniforms(); }

private:
  vtkUniformInternals(const vtkUniformInternals&) = delete;
  void operator=(const vtkUniformInternals&) = delete;

  vtkTimeStamp UniformListMTime;

  vtkOpenGLUniforms* Parent;

  typedef std::map<std::string, Uniform*> UniformMap;
  UniformMap Uniforms;
};

vtkStandardNewMacro(vtkUniformInternals);

vtkOpenGLUniforms::vtkOpenGLUniforms()
{
  this->Internals = vtkUniformInternals::New();
  this->Internals->SetParent(this);
}

vtkOpenGLUniforms::~vtkOpenGLUniforms()
{
  this->Internals->Delete();
}

void vtkOpenGLUniforms::RemoveUniform(const char* name)
{
  this->Internals->RemoveUniform(name);
}

void vtkOpenGLUniforms::RemoveAllUniforms()
{
  this->Internals->RemoveAllUniforms();
}

// Utility function to convert fixed size array to std::vector
template <typename T>
std::vector<T> arrayToVec(const T* in, int count)
{
  return std::vector<T>(in, in + count);
}

//---------------------------------------------------------------------------------------
// Generic Setters and Getters (useful for IO)
//---------------------------------------------------------------------------------------

void vtkOpenGLUniforms::SetUniform(
  const char* name, vtkUniforms::TupleType tt, int nbComponents, const std::vector<int>& value)
{
  if (tt == vtkUniforms::TupleTypeScalar)
  {
    if (value.size() == 1)
    {
      this->Internals->SetUniformValue<int, UniformScalari>(name, value[0]);
    }
    else if (value.size() > 1)
    {
      this->Internals->SetUniformValue<std::vector<int>, Uniform1iv>(name, value);
    }
    else
    {
      vtkErrorMacro(<< "Uniform type doesn't match input value.");
    }
  }
  else if (tt == vtkUniforms::TupleTypeVector)
  {
    if (nbComponents == 2)
    {
      if (value.size() == 2)
      {
        this->Internals->SetUniformValue<std::vector<int>, UniformVec2i>(name, value);
      }
      else
      {
        vtkErrorMacro(<< "Uniform type doesn't match input value.");
      }
    }
  }
  else
  {
    vtkErrorMacro(<< "Invalid tuple type");
  }
}

void vtkOpenGLUniforms::SetUniform(
  const char* name, vtkUniforms::TupleType tt, int nbComponents, const std::vector<float>& value)
{
  if (tt == vtkUniforms::TupleTypeScalar)
  {
    if (value.size() == 1)
    {
      this->Internals->SetUniformValue<float, UniformScalarf>(name, value[0]);
    }
    else if (value.size() > 1)
    {
      this->Internals->SetUniformValue<std::vector<float>, Uniform1fv>(name, value);
    }
    else
    {
      vtkErrorMacro(<< "Uniform type doesn't match input value.");
    }
  }
  else if (tt == vtkUniforms::TupleTypeVector)
  {
    if (nbComponents == 2)
    {
      if (value.size() == 2)
      {
        this->Internals->SetUniformValue<std::vector<float>, UniformVec2f>(name, value);
      }
      else if (value.size() > 2 && value.size() % 2 == 0)
      {
        this->Internals->SetUniformValue<std::vector<float>, Uniform2fv>(name, value);
      }
      else
      {
        vtkErrorMacro(<< "Uniform type doesn't match input value.");
      }
    }
    if (nbComponents == 3)
    {
      if (value.size() == 3)
      {
        this->Internals->SetUniformValue<std::vector<float>, UniformVec3f>(name, value);
      }
      else if (value.size() > 3 && value.size() % 3 == 0)
      {
        this->Internals->SetUniformValue<std::vector<float>, Uniform3fv>(name, value);
      }
      else
      {
        vtkErrorMacro(<< "Uniform type doesn't match input value.");
      }
    }
    if (nbComponents == 4)
    {
      if (value.size() == 4)
      {
        this->Internals->SetUniformValue<std::vector<float>, UniformVec4f>(name, value);
      }
      else if (value.size() > 4 && value.size() % 4 == 0)
      {
        this->Internals->SetUniformValue<std::vector<float>, Uniform4fv>(name, value);
      }
      else
      {
        vtkErrorMacro(<< "Uniform type doesn't match input value.");
      }
    }
  }
  else if (tt == vtkUniforms::TupleTypeMatrix)
  {
    if (nbComponents == 9)
    {
      if (value.size() == 9)
      {
        this->Internals->SetUniformValue<std::vector<float>, UniformMat3f>(name, value);
      }
      else
      {
        vtkErrorMacro(<< "Uniform type doesn't match input value.");
      }
    }
    else if (nbComponents == 16)
    {
      if (value.size() == 16)
      {
        this->Internals->SetUniformValue<std::vector<float>, UniformMat4f>(name, value);
      }
      else if (value.size() > 16 && value.size() % 16 == 0)
      {
        this->Internals->SetUniformValue<std::vector<float>, UniformMat4fv>(name, value);
      }
      else
      {
        vtkErrorMacro(<< "Uniform type doesn't match input value.");
      }
    }
    else
    {
      vtkErrorMacro(<< "Uniform type doesn't match input value.");
    }
  }
  else
  {
    vtkErrorMacro(<< "Invalid tuple type");
  }
}

bool vtkOpenGLUniforms::GetUniform(const char* name, std::vector<int>& value)
{
  return this->Internals->GetGenericUniformValue(name, value);
}

bool vtkOpenGLUniforms::GetUniform(const char* name, std::vector<float>& value)
{
  return this->Internals->GetGenericUniformValue(name, value);
}

//---------------------------------------------------------------------------------------
// Basic setters
//---------------------------------------------------------------------------------------

void vtkOpenGLUniforms::SetUniformi(const char* name, int v)
{
  this->Internals->SetUniformValue<int, UniformScalari>(name, v);
}

void vtkOpenGLUniforms::SetUniformf(const char* name, float v)
{
  this->Internals->SetUniformValue<float, UniformScalarf>(name, v);
}

void vtkOpenGLUniforms::SetUniform2i(const char* name, const int v[2])
{
  std::vector<int> sv = arrayToVec(v, 2);
  this->Internals->SetUniformValue<std::vector<int>, UniformVec2i>(name, sv);
}

void vtkOpenGLUniforms::SetUniform2f(const char* name, const float v[2])
{
  std::vector<float> sv = arrayToVec(v, 2);
  this->Internals->SetUniformValue<std::vector<float>, UniformVec2f>(name, sv);
}

void vtkOpenGLUniforms::SetUniform3f(const char* name, const float v[3])
{
  std::vector<float> sv = arrayToVec(v, 3);
  this->Internals->SetUniformValue<std::vector<float>, UniformVec3f>(name, sv);
}

void vtkOpenGLUniforms::SetUniform4f(const char* name, const float v[4])
{
  std::vector<float> sv = arrayToVec(v, 4);
  this->Internals->SetUniformValue<std::vector<float>, UniformVec4f>(name, sv);
}

void vtkOpenGLUniforms::SetUniformMatrix3x3(const char* name, float* v)
{
  std::vector<float> sv = arrayToVec(v, 9);
  this->Internals->SetUniformValue<std::vector<float>, UniformMat3f>(name, sv);
}

void vtkOpenGLUniforms::SetUniformMatrix4x4(const char* name, float* v)
{
  std::vector<float> sv = arrayToVec(v, 16);
  this->Internals->SetUniformValue<std::vector<float>, UniformMat4f>(name, sv);
}

void vtkOpenGLUniforms::SetUniform1iv(const char* name, const int count, const int* v)
{
  std::vector<int> sv = arrayToVec(v, count);
  this->Internals->SetUniformValue<std::vector<int>, Uniform1iv>(name, sv);
}

void vtkOpenGLUniforms::SetUniform1fv(const char* name, const int count, const float* v)
{
  std::vector<float> sv = arrayToVec(v, count);
  this->Internals->SetUniformValue<std::vector<float>, Uniform1fv>(name, sv);
}

void vtkOpenGLUniforms::SetUniform2fv(const char* name, const int count, const float (*v)[2])
{
  std::vector<float> sv = arrayToVec(reinterpret_cast<const float*>(v), count * 2);
  this->Internals->SetUniformValue<std::vector<float>, Uniform2fv>(name, sv);
}

void vtkOpenGLUniforms::SetUniform3fv(const char* name, const int count, const float (*v)[3])
{
  std::vector<float> sv = arrayToVec(reinterpret_cast<const float*>(v), count * 3);
  this->Internals->SetUniformValue<std::vector<float>, Uniform3fv>(name, sv);
}

void vtkOpenGLUniforms::SetUniform4fv(const char* name, const int count, const float (*v)[4])
{
  std::vector<float> sv = arrayToVec(reinterpret_cast<const float*>(v), count * 4);
  this->Internals->SetUniformValue<std::vector<float>, Uniform4fv>(name, sv);
}

void vtkOpenGLUniforms::SetUniformMatrix4x4v(const char* name, const int count, float* v)
{
  std::vector<float> sv = arrayToVec(v, count * 16);
  this->Internals->SetUniformValue<std::vector<float>, UniformMat4fv>(name, sv);
}

//---------------------------------------------------------------------------------------
// Convenience setters (data undergoes conversion)
//---------------------------------------------------------------------------------------

void vtkOpenGLUniforms::SetUniform3f(const char* name, const double v[3])
{
  std::vector<float> sv;
  sv.reserve(3);
  for (int i = 0; i < 3; ++i)
  {
    sv.push_back(static_cast<float>(v[i]));
  }
  this->Internals->SetUniformValue<std::vector<float>, UniformVec3f>(name, sv);
}

void vtkOpenGLUniforms::SetUniform3uc(const char* name, const unsigned char v[3])
{
  std::vector<float> sv({ v[0] / 255.0f, v[1] / 255.0f, v[2] / 255.0f });
  this->Internals->SetUniformValue<std::vector<float>, UniformVec3f>(name, sv);
}

void vtkOpenGLUniforms::SetUniform4uc(const char* name, const unsigned char v[4])
{
  std::vector<float> sv({ v[0] / 255.0f, v[1] / 255.0f, v[2] / 255.0f, v[3] / 255.0f });
  this->Internals->SetUniformValue<std::vector<float>, UniformVec4f>(name, sv);
}

void vtkOpenGLUniforms::SetUniformMatrix(const char* name, vtkMatrix3x3* v)
{
  std::vector<float> sv;
  sv.reserve(9);
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      sv.push_back(static_cast<float>(v->GetElement(i, j)));
    }
  }
  this->Internals->SetUniformValue<std::vector<float>, UniformMat3f>(name, sv);
}

void vtkOpenGLUniforms::SetUniformMatrix(const char* name, vtkMatrix4x4* v)
{
  std::vector<float> sv;
  sv.reserve(16);
  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      sv.push_back(static_cast<float>(v->GetElement(i, j)));
    }
  }
  this->Internals->SetUniformValue<std::vector<float>, UniformMat4f>(name, sv);
}

//---------------------------------------------------------------------------------------
// Type specific getters
//---------------------------------------------------------------------------------------

bool vtkOpenGLUniforms::GetUniformi(const char* name, int& v)
{
  return this->Internals->GetUniformValue<int, UniformScalari>(name, reinterpret_cast<int*>(&v));
}

bool vtkOpenGLUniforms::GetUniformf(const char* name, float& v)
{
  return this->Internals->GetUniformValue<float, UniformScalarf>(
    name, reinterpret_cast<float*>(&v));
}

bool vtkOpenGLUniforms::GetUniform2i(const char* name, int v[2])
{
  return this->Internals->GetUniformValue<int, UniformVec2i>(name, reinterpret_cast<int*>(v));
}

bool vtkOpenGLUniforms::GetUniform2f(const char* name, float v[2])
{
  return this->Internals->GetUniformValue<float, UniformVec2f>(name, reinterpret_cast<float*>(v));
}

bool vtkOpenGLUniforms::GetUniform3f(const char* name, float v[3])
{
  return this->Internals->GetUniformValue<float, UniformVec3f>(name, reinterpret_cast<float*>(v));
}

bool vtkOpenGLUniforms::GetUniform4f(const char* name, float v[4])
{
  return this->Internals->GetUniformValue<float, UniformVec4f>(name, reinterpret_cast<float*>(v));
}

bool vtkOpenGLUniforms::GetUniformMatrix3x3(const char* name, float* v)
{
  return this->Internals->GetUniformValue<float, UniformMat3f>(name, v);
}

bool vtkOpenGLUniforms::GetUniformMatrix4x4(const char* name, float* v)
{
  return this->Internals->GetUniformValue<float, UniformMat4f>(name, v);
}

bool vtkOpenGLUniforms::GetUniform1iv(const char* name, std::vector<int>& v)
{
  return this->Internals->GetUniformValue<int, Uniform1iv>(name, v);
}

bool vtkOpenGLUniforms::GetUniform1fv(const char* name, std::vector<float>& v)
{
  return this->Internals->GetUniformValue<float, Uniform1fv>(name, v);
}

bool vtkOpenGLUniforms::GetUniform2fv(const char* name, std::vector<float>& v)
{
  return this->Internals->GetUniformValue<float, Uniform2fv>(name, v);
}

bool vtkOpenGLUniforms::GetUniform3fv(const char* name, std::vector<float>& v)
{
  return this->Internals->GetUniformValue<float, Uniform3fv>(name, v);
}

bool vtkOpenGLUniforms::GetUniform4fv(const char* name, std::vector<float>& v)
{
  return this->Internals->GetUniformValue<float, Uniform4fv>(name, v);
}

bool vtkOpenGLUniforms::GetUniformMatrix4x4v(const char* name, std::vector<float>& v)
{
  return this->Internals->GetUniformValue<float, UniformMat4fv>(name, v);
}

//---------------------------------------------------------------------------------------
// Convenience getters (with type conversion)
//---------------------------------------------------------------------------------------

bool vtkOpenGLUniforms::GetUniform3f(const char* name, double v[3])
{
  std::vector<float> val;
  bool res = this->Internals->GetUniformValue<float, UniformVec3f>(name, val);
  if (res)
  {
    v[0] = static_cast<double>(val[0]);
    v[1] = static_cast<double>(val[1]);
    v[2] = static_cast<double>(val[2]);
    return true;
  }
  return false;
}

bool vtkOpenGLUniforms::GetUniform3uc(const char* name, unsigned char v[3])
{
  std::vector<float> val;
  bool res = this->Internals->GetUniformValue<float, UniformVec3f>(name, val);
  if (res)
  {
    v[0] = static_cast<unsigned char>(std::round(val[0] * 255.0f));
    v[1] = static_cast<unsigned char>(std::round(val[1] * 255.0f));
    v[2] = static_cast<unsigned char>(std::round(val[2] * 255.0f));
    return true;
  }
  return false;
}

bool vtkOpenGLUniforms::GetUniform4uc(const char* name, unsigned char v[4])
{
  std::vector<float> val;
  bool res = this->Internals->GetUniformValue<float, UniformVec4f>(name, val);
  if (res)
  {
    v[0] = static_cast<unsigned char>(std::round(val[0] * 255.0f));
    v[1] = static_cast<unsigned char>(std::round(val[1] * 255.0f));
    v[2] = static_cast<unsigned char>(std::round(val[2] * 255.0f));
    v[3] = static_cast<unsigned char>(std::round(val[3] * 255.0f));
    return true;
  }
  return false;
}

bool vtkOpenGLUniforms::GetUniformMatrix(const char* name, vtkMatrix3x3* v)
{
  std::vector<float> val;
  bool res = this->Internals->GetUniformValue<float, UniformMat3f>(name, val);
  if (res)
  {
    for (unsigned i = 0; i < 3; ++i)
    {
      for (unsigned j = 0; j < 3; ++j)
      {
        v->SetElement(
          static_cast<int>(i), static_cast<int>(j), static_cast<double>(val[3 * i + j]));
      }
    }
    return true;
  }
  return false;
}

bool vtkOpenGLUniforms::GetUniformMatrix(const char* name, vtkMatrix4x4* v)
{
  std::vector<float> val;
  bool res = this->Internals->GetUniformValue<float, UniformMat4f>(name, val);
  if (res)
  {
    for (unsigned i = 0; i < 4; ++i)
    {
      for (unsigned j = 0; j < 4; ++j)
      {
        v->SetElement(
          static_cast<int>(i), static_cast<int>(j), static_cast<double>(val[4 * i + j]));
      }
    }
    return true;
  }
  return false;
}

std::string vtkOpenGLUniforms::GetDeclarations()
{
  return this->Internals->GetDeclarations();
}

bool vtkOpenGLUniforms::SetUniforms(vtkShaderProgram* p)
{
  return this->Internals->SetUniforms(p);
}

vtkMTimeType vtkOpenGLUniforms::GetUniformListMTime()
{
  return this->Internals->GetUniformListMTime();
}

int vtkOpenGLUniforms::GetNumberOfUniforms()
{
  return this->Internals->GetNumberOfUniforms();
}

const char* vtkOpenGLUniforms::GetNthUniformName(vtkIdType uniformIndex)
{
  return this->Internals->GetNthUniformName(uniformIndex);
}

int vtkOpenGLUniforms::GetUniformScalarType(const char* name)
{
  return this->Internals->GetUniformScalarType(name);
}

vtkUniforms::TupleType vtkOpenGLUniforms::GetUniformTupleType(const char* name)
{
  return this->Internals->GetUniformTupleType(name);
}

int vtkOpenGLUniforms::GetUniformNumberOfComponents(const char* name)
{
  return this->Internals->GetUniformNumberOfComponents(name);
}

int vtkOpenGLUniforms::GetUniformNumberOfTuples(const char* name)
{
  return this->Internals->GetNumberOfTuples(name);
}

void vtkOpenGLUniforms::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Internals->PrintSelf(os, indent);
}
