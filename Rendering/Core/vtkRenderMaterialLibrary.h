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
#include <iosfwd>
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

  enum class ParameterType
  {
    FLOAT,
    FLOAT_DATA,
    COLOR_RGB,
    COLOR_RGBA,
    VEC2,
    VEC3,
    VEC4,
    BOOLEAN,
    INT,
    TEXTURE,
    NORMALIZED_FLOAT
  };

  using ParametersMap = std::map<std::string, ParameterType>;

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
  std::string GetTextureName(const std::string& nickname, const std::string& varname);
  std::string GetTextureFilename(const std::string& nickname, const std::string& varname);

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

  void RemoveShaderVariable(const std::string& nickname, const std::string& variablename);
  void RemoveAllShaderVariables(const std::string& nickname);
  void RemoveTexture(const std::string& nickname, const std::string& varname);
  void RemoveAllTextures(const std::string& nickname);

  const char* WriteBuffer(bool writeImageInline = true);
  void WriteFile(const std::string& filename, bool writeImageInline = false);

protected:
  /**
   * Load texture from file or inline data. Subclasses may override for custom texture loading.
   * @param texFilenameOrData Path to file or inline XML/binary data
   * @param fromfile True if texFilenameOrData is a file path
   * @param parentDir Parent directory for relative paths
   * @param textr Texture object to populate
   * @param textureName Output: name of the texture
   * @param textureFilename Output: full path to texture file
   * @return True if loading succeeded
   */
  virtual bool ReadTextureFileOrData(const std::string& texFilenameOrData, bool fromfile,
    const std::string& parentDir, vtkTexture* textr, std::string& textureName,
    std::string& textureFilename);

  bool InternalParse(const char* filename, bool fromfile);
  bool InternalParseJSON(const char* filename, bool fromfile, std::istream* doc);
  bool InternalParseMTL(const char* filename, bool fromfile, std::istream* doc);

  virtual const std::map<std::string, ParametersMap>& GetParametersDictionary();

  // Helper utilities shared by subclasses. Made protected so subclasses
  // can use consistent behavior and avoid duplicate definitions.
  std::string FilePathToTextureName(const std::string& path);
  std::string Trim(const std::string& s);

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
