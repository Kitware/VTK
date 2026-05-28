// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRenderMaterialLibrary
 * @brief   a renderer-agnostic collection of materials for VTK apps to draw from
 */

#ifndef vtkRenderMaterialLibrary_h
#define vtkRenderMaterialLibrary_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

#include <initializer_list>
#include <map>
#include <set>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderMaterialLibraryInternals;
class vtkTexture;
struct TextureInfo;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkRenderMaterialLibrary : public vtkObject
{
public:
  static vtkRenderMaterialLibrary* New();
  vtkTypeMacro(vtkRenderMaterialLibrary, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Fire();

  bool ReadFile(const char* FileName);
  bool ReadBuffer(const char* Buffer);

  std::set<std::string> GetMaterialNames();
  std::string LookupImplName(const std::string& nickname);

  std::vector<std::string> GetDoubleShaderVariableList(const std::string& nickname);
  virtual std::vector<double> GetDoubleShaderVariable(
    const std::string& nickname, const std::string& varname);

  std::vector<std::string> GetTextureList(const std::string& nickname);
  vtkTexture* GetTexture(const std::string& nickname, const std::string& varname);
  virtual const TextureInfo* GetTextureInfo(
    const std::string& nickname, const std::string& varname);

  virtual void AddMaterial(const std::string& nickname, const std::string& implname);
  void RemoveMaterial(const std::string& nickname);

  virtual void AddTexture(const std::string& nickname, const std::string& varname, vtkTexture* tex,
    const std::string& texturename = "unnamedTexture", const std::string& filename = "");

  virtual void AddShaderVariable(
    const std::string& nickname, const std::string& variablename, int numVars, const double* x);
  void AddShaderVariable(const std::string& nickname, const std::string& variablename,
    const std::initializer_list<double>& data)
  {
    this->AddShaderVariable(nickname, variablename, static_cast<int>(data.size()), data.begin());
  }

protected:
  vtkRenderMaterialLibrary();
  ~vtkRenderMaterialLibrary() override;

  virtual const char* GetFamilyName() { return "Generic"; }
  virtual const char* GetAcceptedFamilyName() { return nullptr; }
  std::string InternalGetImplName(const std::string& nickname) const;

private:
  vtkRenderMaterialLibrary(const vtkRenderMaterialLibrary&) = delete;
  void operator=(const vtkRenderMaterialLibrary&) = delete;

  vtkRenderMaterialLibraryInternals* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
