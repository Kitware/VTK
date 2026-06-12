// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSPRayMaterialLibrary
 * @brief   a collection of materials for OSPRay-based rendering
 *
 * A singleton instance of this class manages a collection of materials
 * for use with the OSPRay renderer. The materials can be read in from
 * disk or created programmatically.
 *
 * This class extends vtkRenderMaterialLibrary with OSPRay-specific
 * material type validation and the OSPRay material parameter dictionary.
 * It accepts material files with the "OSPRay" family designation.
 *
 * @sa vtkRenderMaterialLibrary, vtkOSPRayMaterialHelpers
 */

#ifndef vtkOSPRayMaterialLibrary_h
#define vtkOSPRayMaterialLibrary_h

#include "vtkRenderMaterialLibrary.h"
#include "vtkRenderingRayTracingModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayMaterialLibrary : public vtkRenderMaterialLibrary
{
public:
  static vtkOSPRayMaterialLibrary* New();
  vtkTypeMacro(vtkOSPRayMaterialLibrary, vtkRenderMaterialLibrary);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add Material
   * Adds a new material nickname to the set of known materials.
   * The implementation name is validated against the OSPRay material dictionary.
   * If the name is a repeat, we replace the old one.
   **/
  void AddMaterial(const std::string& nickname, const std::string& implname) override;

  /**
   * Add Texture
   * Given a material @c nickname and a shader variable @c varname, set its data
   * to a specific texture @c tex named @c texturename.
   * The variable name is validated against the OSPRay material parameter dictionary.
   *
   * Replaces any previous content.
   **/
  void AddTexture(const std::string& nickname, const std::string& varname, vtkTexture* tex,
    const std::string& texturename = "unnamedTexture", const std::string& filename = "") override;

  /**
   * Add control variable
   * Adds a new control variable validated against the OSPRay parameter dictionary.
   * Replaces any previous content.
   **/
  void AddShaderVariable(const std::string& nickname, const std::string& variablename, int numVars,
    const double* x) override;
  using Superclass::AddShaderVariable;

  /**
   * Returns a uniform variable, resolving OSPRay parameter name aliases.
   */
  std::vector<double> GetDoubleShaderVariable(
    const std::string& nickname, const std::string& varname);

  /**
   * Returns the texture information, resolving OSPRay parameter name aliases.
   */
  const TextureInfo* GetTextureInfo(const std::string& nickname, const std::string& varname);

  /**
   * Get the dictionary of all possible materials based on OSPRay documentation.
   */
  const std::map<std::string, ParametersMap>& GetParametersDictionary() override;

protected:
  /**
   * Load texture from file or inline XML data.
   * Implements image loading for OSPRay materials.
   */
  bool ReadTextureFileOrData(const std::string& texFilenameOrData, bool fromfile,
    const std::string& parentDir, vtkTexture* textr, std::string& textureName,
    std::string& textureFilename) override;
  vtkOSPRayMaterialLibrary() = default;
  ~vtkOSPRayMaterialLibrary() override = default;

  const char* GetFamilyName() override { return "OSPRay"; }
  const char* GetAcceptedFamilyName() override { return "OSPRay"; }

private:
  vtkOSPRayMaterialLibrary(const vtkOSPRayMaterialLibrary&) = delete;
  void operator=(const vtkOSPRayMaterialLibrary&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
