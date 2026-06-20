// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkANARIMaterialLibrary
 * @brief   a collection of materials for ANARI-based rendering
 *
 * A singleton instance of this class manages a collection of materials
 * for use with the ANARI renderer. The materials can be read in from
 * disk or created programmatically.
 *
 * This class extends vtkRenderMaterialLibrary with ANARI-specific
 * material type validation and the ANARI material parameter dictionary.
 * It accepts material files with the "ANARI" family designation.
 *
 * @sa vtkRenderMaterialLibrary
 */

#ifndef vtkANARIMaterialLibrary_h
#define vtkANARIMaterialLibrary_h

#include "vtkRenderMaterialLibrary.h"
#include "vtkRenderingAnariModule.h" // For export macro
#include "vtkWrappingHints.h"        // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGANARI_EXPORT VTK_MARSHALAUTO vtkANARIMaterialLibrary
  : public vtkRenderMaterialLibrary
{
public:
  static vtkANARIMaterialLibrary* New();
  vtkTypeMacro(vtkANARIMaterialLibrary, vtkRenderMaterialLibrary);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add Material
   * Adds a new material nickname to the set of known materials.
   * The implementation name is validated against the ANARI material dictionary.
   * If the name is a repeat, we replace the old one.
   **/
  void AddMaterial(const std::string& nickname, const std::string& implname) override;

  /**
   * Add Texture
   * Given a material @c nickname and a shader variable @c varname, set its data
   * to a specific texture @c tex named @c texturename.
   * The variable name is validated against the ANARI material parameter dictionary.
   *
   * Replaces any previous content.
   **/
  void AddTexture(const std::string& nickname, const std::string& varname, vtkTexture* tex,
    const std::string& texturename = "unnamedTexture", const std::string& filename = "") override;

  /**
   * Add control variable
   * Adds a new control variable validated against the ANARI parameter dictionary.
   * Replaces any previous content.
   **/
  void AddShaderVariable(const std::string& nickname, const std::string& variablename, int numVars,
    const double* x) override;

  /**
   * Get the dictionary of all possible materials based on ANARI documentation.
   */
  const std::map<std::string, ParametersMap>& GetParametersDictionary() const override;

protected:
  /**
   * Load texture from file or inline XML data.
   * Implements image loading for ANARI materials.
   */
  bool ReadTextureFileOrData(const std::string& texFilenameOrData, bool fromfile,
    const std::string& parentDir, vtkTexture* textr, std::string& textureName,
    std::string& textureFilename) override;
  vtkANARIMaterialLibrary();
  ~vtkANARIMaterialLibrary() override;

  const char* GetFamilyName() const override { return "ANARI"; }
  const char* GetAcceptedFamilyName() const override { return "ANARI"; }

private:
  vtkANARIMaterialLibrary(const vtkANARIMaterialLibrary&) = delete;
  void operator=(const vtkANARIMaterialLibrary&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
