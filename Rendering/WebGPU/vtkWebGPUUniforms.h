// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUUniforms_h
#define vtkWebGPUUniforms_h

#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkUniforms.h"

#include <memory> // For std::unique_ptr
#include <string> // For std::string
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUUniforms : public vtkUniforms
{
public:
  static vtkWebGPUUniforms* New();
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkWebGPUUniforms, vtkUniforms);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkMTimeType GetUniformListMTime() override;

  void RemoveUniform(const char* name) override;
  void RemoveAllUniforms() override;

  void SetUniform(const char* name, vtkUniforms::TupleType tt, int nbComponents,
    const std::vector<int>& value) override;
  void SetUniform(const char* name, vtkUniforms::TupleType tt, int nbComponents,
    const std::vector<float>& value) override;
  bool GetUniform(const char* name, std::vector<int>& value) override;
  bool GetUniform(const char* name, std::vector<float>& value) override;

  void SetUniformi(const char* name, int v) override;
  void SetUniformf(const char* name, float v) override;
  void SetUniform2i(const char* name, const int v[2]) override;
  void SetUniform2f(const char* name, const float v[2]) override;
  void SetUniform3f(const char* name, const float v[3]) override;
  void SetUniform4f(const char* name, const float v[4]) override;
  void SetUniformMatrix3x3(const char* name, float* v) override;
  void SetUniformMatrix4x4(const char* name, float* v) override;

  void SetUniform1iv(const char* name, int count, const int* f) override;
  void SetUniform1fv(const char* name, int count, const float* f) override;
  void SetUniform2fv(const char* name, int count, const float (*f)[2]) override;
  void SetUniform3fv(const char* name, int count, const float (*f)[3]) override;
  void SetUniform4fv(const char* name, int count, const float (*f)[4]) override;
  void SetUniformMatrix4x4v(const char* name, int count, float* v) override;

  void SetUniform3f(const char* name, const double v[3]) override;
  void SetUniform3uc(const char* name, const unsigned char v[3]) override;
  void SetUniform4uc(const char* name, const unsigned char v[4]) override;
  void SetUniformMatrix(const char* name, vtkMatrix3x3* v) override;
  void SetUniformMatrix(const char* name, vtkMatrix4x4* v) override;

  bool GetUniformi(const char* name, int& v) override;
  bool GetUniformf(const char* name, float& v) override;
  bool GetUniform2i(const char* name, int v[2]) override;
  bool GetUniform2f(const char* name, float v[2]) override;
  bool GetUniform3f(const char* name, float v[3]) override;
  bool GetUniform4f(const char* name, float v[4]) override;
  bool GetUniformMatrix3x3(const char* name, float* v) override;
  bool GetUniformMatrix4x4(const char* name, float* v) override;

  bool GetUniform3f(const char* name, double v[3]) override;
  bool GetUniform3uc(const char* name, unsigned char v[3]) override;
  bool GetUniform4uc(const char* name, unsigned char v[4]) override;
  bool GetUniformMatrix(const char* name, vtkMatrix3x3* v) override;
  bool GetUniformMatrix(const char* name, vtkMatrix4x4* v) override;

  bool GetUniform1iv(const char* name, std::vector<int>& f) override;
  bool GetUniform1fv(const char* name, std::vector<float>& f) override;
  bool GetUniform2fv(const char* name, std::vector<float>& f) override;
  bool GetUniform3fv(const char* name, std::vector<float>& f) override;
  bool GetUniform4fv(const char* name, std::vector<float>& f) override;
  bool GetUniformMatrix4x4v(const char* name, std::vector<float>& f) override;

  int GetNumberOfUniforms() override;
  const char* GetNthUniformName(vtkIdType uniformIndex) override;
  int GetUniformScalarType(const char* name) override;
  TupleType GetUniformTupleType(const char* name) override;
  int GetUniformNumberOfComponents(const char* name) override;
  int GetUniformNumberOfTuples(const char* name) override;

protected:
  vtkWebGPUUniforms();
  ~vtkWebGPUUniforms() override;

private:
  vtkWebGPUUniforms(const vtkWebGPUUniforms&) = delete;
  void operator=(const vtkWebGPUUniforms&) = delete;

  struct Internals;
  std::unique_ptr<Internals> Storage;
};

#define vtkWebGPUUniforms_OVERRIDE_ATTRIBUTES vtkWebGPUUniforms::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
