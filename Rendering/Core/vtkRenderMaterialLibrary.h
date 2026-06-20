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

  /**
   * Read a material file from disk.
   * Supports JSON and OBJ/MTL file formats.
   * @param FileName Path to the material file to read
   * @return True if file was successfully parsed, false otherwise
   */
  bool ReadFile(const char* FileName);

  /**
   * Parse material data from a buffer in memory.
   * @param Buffer String containing material data (JSON or MTL format)
   * @return True if buffer was successfully parsed, false otherwise
   */
  bool ReadBuffer(const char* Buffer);

  /**
   * Get the set of all material nicknames currently in the library.
   * @return Set of material nickname strings
   */
  const std::set<std::string>& GetMaterialNames() const;

  /**
   * Look up the implementation name for a material nickname.
   * The implementation name indicates the underlying material type (e.g., "glass", "metal").
   * @param nickname The user-facing material name
   * @return The implementation/type name for the material
   */
  std::string LookupImplName(const std::string& nickname) const;

  /**
   * Get the list of double-valued shader variables for a material.
   * @param nickname The material to query
   * @return Vector of variable names
   */
  std::vector<std::string> GetDoubleShaderVariableList(const std::string& nickname) const;

  /**
   * Get the value of a double shader variable for a material.
   * @param nickname The material to query
   * @param varname The variable name to retrieve
   * @return Vector of double values for the variable
   */
  virtual std::vector<double> GetDoubleShaderVariable(
    const std::string& nickname, const std::string& varname) const;

  /**
   * Get the list of texture variables for a material.
   * @param nickname The material to query
   * @return Vector of texture variable names
   */
  std::vector<std::string> GetTextureList(const std::string& nickname) const;

  /**
   * Get a texture object for a material variable.
   * @param nickname The material to query
   * @param varname The texture variable name
   * @return Pointer to vtkTexture, or nullptr if not found
   */
  vtkTexture* GetTexture(const std::string& nickname, const std::string& varname) const;

  /**
   * Get texture information for a material variable.
   * @param nickname The material to query
   * @param varname The texture variable name
   * @return Pointer to TextureInfo structure, or nullptr if not found
   */
  virtual const TextureInfo* GetTextureInfo(
    const std::string& nickname, const std::string& varname) const;

  /**
   * Get the registered name of a texture variable.
   * @param nickname The material to query
   * @param varname The texture variable name
   * @return The texture name string
   */
  std::string GetTextureName(const std::string& nickname, const std::string& varname) const;

  /**
   * Get the filename path of a texture variable.
   * @param nickname The material to query
   * @param varname The texture variable name
   * @return The texture file path string
   */
  std::string GetTextureFilename(const std::string& nickname, const std::string& varname) const;

  /**
   * Add a new material to the library or replace an existing one.
   * Subclasses may override to add validation logic.
   * @param nickname User-facing name for the material
   * @param implname Implementation/type name of the material
   */
  virtual void AddMaterial(const std::string& nickname, const std::string& implname);

  /**
   * Remove a material from the library.
   * Also removes all associated textures and shader variables.
   * @param nickname The material to remove
   */
  void RemoveMaterial(const std::string& nickname);

  /**
   * Add a texture to a material.
   * Subclasses may override to add validation logic.
   * @param nickname The material to modify
   * @param varname The texture variable name
   * @param tex The texture object to add
   * @param texturename Optional name for the texture
   * @param filename Optional filename path for the texture
   */
  virtual void AddTexture(const std::string& nickname, const std::string& varname, vtkTexture* tex,
    const std::string& texturename = "unnamedTexture", const std::string& filename = "");

  /**
   * Add a shader variable to a material.
   * Subclasses may override to add validation logic.
   * @param nickname The material to modify
   * @param variablename The shader variable name
   * @param numVars Number of values in the variable
   * @param x Array of double values for the variable
   */
  virtual void AddShaderVariable(
    const std::string& nickname, const std::string& variablename, int numVars, const double* x);

  /**
   * Add a shader variable from an initializer list of doubles.
   * Convenience overload that forwards to AddShaderVariable(string, string, int, const double*).
   * @param nickname The material to modify
   * @param variablename The shader variable name
   * @param data Initializer list of double values
   */
  void AddShaderVariable(const std::string& nickname, const std::string& variablename,
    const std::initializer_list<double>& data)
  {
    this->AddShaderVariable(nickname, variablename, static_cast<int>(data.size()), data.begin());
  }

  /**
   * Remove a shader variable from a material.
   * @param nickname The material to modify
   * @param variablename The shader variable to remove
   */
  void RemoveShaderVariable(const std::string& nickname, const std::string& variablename);

  /**
   * Remove all shader variables from a material.
   * @param nickname The material to modify
   */
  void RemoveAllShaderVariables(const std::string& nickname);

  /**
   * Remove a texture from a material.
   * @param nickname The material to modify
   * @param varname The texture variable to remove
   */
  void RemoveTexture(const std::string& nickname, const std::string& varname);

  /**
   * Remove all textures from a material.
   * @param nickname The material to modify
   */
  void RemoveAllTextures(const std::string& nickname);

  /**
   * Write the material library to a string buffer.
   * @param writeImageInline If true, embed image data inline in the output; if false, use file
   * references
   * @return String buffer containing the serialized material library data
   */
  const char* WriteBuffer(bool writeImageInline = true);

  /**
   * Write the material library to a file on disk.
   * @param filename Path to write the material file to
   * @param writeImageInline If true, embed image data inline in the output; if false, use file
   * references
   */
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

  virtual const std::map<std::string, ParametersMap>& GetParametersDictionary() const;

  /**
   * Convert a file path to a texture name by extracting the filename without extension.
   * @param path File path to convert
   * @return Filename without extension
   */
  std::string FilePathToTextureName(const std::string& path);

  /**
   * Remove leading and trailing whitespace from a string.
   * @param s Input string
   * @return String with whitespace trimmed from both ends
   */
  std::string Trim(const std::string& s);

  vtkRenderMaterialLibrary();
  ~vtkRenderMaterialLibrary() override;

  virtual const char* GetFamilyName() const { return "Generic"; }
  virtual const char* GetAcceptedFamilyName() const { return nullptr; }
  std::string InternalGetImplName(const std::string& nickname) const;

private:
  vtkRenderMaterialLibrary(const vtkRenderMaterialLibrary&) = delete;
  void operator=(const vtkRenderMaterialLibrary&) = delete;

  vtkRenderMaterialLibraryInternals* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
