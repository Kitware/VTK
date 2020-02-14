/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFImporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGLTFImporter
 * @brief   Import a GLTF file.
 *
 * vtkGLTFImporter is a concrete subclass of vtkImporter that reads glTF 2.0
 * files.
 *
 * The GL Transmission Format (glTF) is an API-neutral runtime asset delivery format.
 * A glTF asset is represented by:
 * - A JSON-formatted file (.gltf) containing a full scene description: node hierarchy, materials,
 *   cameras, as well as descriptor information for meshes, animations, and other constructs
 * - Binary files (.bin) containing geometry and animation data, and other buffer-based data
 * - Image files (.jpg, .png) for textures
 *
 * This importer supports all physically-based rendering material features, with the exception of
 * alpha masking and mirrored texture wrapping, which are not supported.
 *
 *
 * This importer does not support materials that use multiple
 * sets of texture coordinates. Only the first set will be used in this case.
 *
 * This importer does not support animations, morphing and skinning. If you would like to use
 * animations, morphing or skinning, please consider using vtkGLTFReader.
 *
 * This importer only supports assets that use the 2.x version of the glTF specification.
 *
 * For the full glTF specification, see:
 * https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * Note: array sizes should not exceed INT_MAX
 *
 * Supported extensions:
 * - KHR_lights_punctual :
 *   The importer supports the KHR_lights_punctual extension except for this feature:
 *   - VTK does not support changing the falloff of the cone with innerConeAngle and outerConeAngle.
 *     The importer uses outerConeAngle and ignores innerConeAngle as specified for this situation.
 *
 * @sa
 * vtkImporter
 * vtkGLTFReader
 */

#ifndef vtkGLTFImporter_h
#define vtkGLTFImporter_h

#include "vtkIOImportModule.h" // For export macro
#include "vtkImporter.h"
#include "vtkSmartPointer.h" // For SmartPointer

#include <map>    // For map
#include <vector> // For vector

class vtkCamera;
class vtkGLTFDocumentLoader;
class vtkTexture;

class VTKIOIMPORT_EXPORT vtkGLTFImporter : public vtkImporter
{
public:
  static vtkGLTFImporter* New();

  vtkTypeMacro(vtkGLTFImporter, vtkImporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the name of the file to read.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  /**
   * glTF defines multiple camera objects, but no default behavior for which camera should be
   * used. The importer will by default apply the asset's first camera. This accessor lets you use
   * the asset's other cameras.
   */
  vtkSmartPointer<vtkCamera> GetCamera(unsigned int id);

  /**
   * Get the total number of cameras
   */
  size_t GetNumberOfCameras();

  /**
   * Get a printable string describing all outputs
   */
  std::string GetOutputsDescription() override { return this->OutputsDescription; };

protected:
  vtkGLTFImporter() = default;
  ~vtkGLTFImporter() override;

  int ImportBegin() override;
  void ImportActors(vtkRenderer* renderer) override;
  void ImportCameras(vtkRenderer* renderer) override;
  void ImportLights(vtkRenderer* renderer) override;

  char* FileName = nullptr;

  std::vector<vtkSmartPointer<vtkCamera> > Cameras;
  std::map<int, vtkSmartPointer<vtkTexture> > Textures;
  vtkSmartPointer<vtkGLTFDocumentLoader> Loader;
  std::string OutputsDescription;

private:
  vtkGLTFImporter(const vtkGLTFImporter&) = delete;
  void operator=(const vtkGLTFImporter&) = delete;
};

#endif
