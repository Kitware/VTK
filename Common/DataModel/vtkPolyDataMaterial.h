// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataMaterial
 * @brief   Information about materials stored as fields in a polydata
 *
 */

#ifndef vtkPolyDataMaterial_h
#define vtkPolyDataMaterial_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"                // For vtkObject
#include <vector>                     // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;

/**
 * If the polydata has a texture, it should contain a TCOORDS point
 * array of type float/double (with two components (u,v)) and a field
 * array of type string: "texture_uri" containing one tuple per texture
 * file with the absolute/relative path to the file.  Texture paths
 * can be relative to the location of the original input file.  If
 * the dataset has a material we store two field arrays of
 * type double, with 3 components and 1 tuple: "diffuse_color" and
 * "specular_color" and two field arrays of type double with 1
 * component and 1 tuple: "transparency", "shininess"
 * (Note: the naming was adopted from app::X3DMaterial)
 * @sa
 * vtkCityGMLReader,
 * vtkGLTFWriter
 * vtkIFCReader
 */
class VTKCOMMONDATAMODEL_EXPORT vtkPolyDataMaterial : public vtkObject
{
public:
  static vtkPolyDataMaterial* New();

  static inline const char* TEXTURE_URI = "texture_uri";
  static inline const char* DIFFUSE_COLOR = "diffuse_color";
  static inline const char* SPECULAR_COLOR = "specular_color";
  static inline const char* TRANSPARENCY = "transparency";
  static inline const char* SHININESS = "shininess";

  ///@{
  /**
   * Helper functions for setting field arrays. These are used to save
   * texture paths or colors for polydata.
   *
   */
  static void SetField(vtkDataObject* obj, const char* name, const char* value);
  static std::vector<std::string> GetField(vtkDataObject* obj, const char* name);
  static void SetField(
    vtkDataObject* obj, const char* name, double* value, vtkIdType numberOfComponents);
  static std::vector<double> GetField(
    vtkDataObject* obj, const char* name, const std::vector<double>& defaultResult);
  static std::vector<float> GetField(
    vtkDataObject* obj, const char* name, const std::vector<float>& defaultRes);
  ///@}

protected:
  vtkPolyDataMaterial() = default;

private:
  vtkPolyDataMaterial(const vtkPolyDataMaterial&) = delete;
  void operator=(const vtkPolyDataMaterial&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
