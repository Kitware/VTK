// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtk3DSImporter
 * @brief   imports 3D Studio files.
 *
 * vtk3DSImporter imports 3D Studio files into vtk.
 *
 * This importer doesn't support scene hierarchy API
 *
 * This importer supports the collection API
 *
 * @sa
 * vtkImporter
 */

#ifndef vtk3DSImporter_h
#define vtk3DSImporter_h

#include "vtk3DS.h"            // Needed for all the 3DS structures
#include "vtkIOImportModule.h" // For export macro
#include "vtkImporter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;

class VTKIOIMPORT_EXPORT vtk3DSImporter : public vtkImporter
{
public:
  static vtk3DSImporter* New();

  vtkTypeMacro(vtk3DSImporter, vtkImporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the name of the file to read.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Set/Get the computation of normals. If on, imported geometry will
   * be run through vtkPolyDataNormals.
   */
  vtkSetMacro(ComputeNormals, vtkTypeBool);
  vtkGetMacro(ComputeNormals, vtkTypeBool);
  vtkBooleanMacro(ComputeNormals, vtkTypeBool);
  ///@}

  /**
   * Get a printable string describing the outputs
   */
  std::string GetOutputsDescription() override;

  /**
   * Return the file pointer to the open file.
   */
  FILE* GetFileFD() { return this->FileFD; }

  vtk3DSOmniLight* OmniList;
  vtk3DSSpotLight* SpotLightList;
  vtk3DSCamera* CameraList;
  vtk3DSMesh* MeshList;
  vtk3DSMaterial* MaterialList;
  vtk3DSMatProp* MatPropList;

protected:
  vtk3DSImporter();
  ~vtk3DSImporter() override;

  int ImportBegin() override;
  void ImportEnd() override;
  void ImportActors(vtkRenderer* renderer) override;
  void ImportCameras(vtkRenderer* renderer) override;
  void ImportLights(vtkRenderer* renderer) override;
  void ImportProperties(vtkRenderer* renderer) override;
  vtkPolyData* GeneratePolyData(vtk3DSMesh* meshPtr);
  int Read3DS();

  char* FileName;
  FILE* FileFD;
  vtkTypeBool ComputeNormals;

private:
  vtk3DSImporter(const vtk3DSImporter&) = delete;
  void operator=(const vtk3DSImporter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
