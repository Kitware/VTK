// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGLTFExporter
 * @brief   export a scene into GLTF 2.0 format.
 *
 * vtkGLTFExporter is a concrete subclass of vtkExporter that writes GLTF 2.0
 * files. It currently only supports a very small subset of what VTK can do
 * including polygonal meshes with optional vertex colors. Over time the class
 * can be expanded to support more and more of what VTK renders.
 *
 * It should be noted that gltf is a format for rendering data. As such
 * it stores what the VTK scene renders as, not the underlying data. For
 * example it currently does not support quads or higher sided polygons
 * although VTK does. As such taking an exported gltf file and then selecting
 * wireframe in a viewer will give all triangles where VTK's rendering
 * would correctly draw the original polygons. etc.
 *
 * @sa
 * vtkExporter
 */

#ifndef vtkGLTFExporter_h
#define vtkGLTFExporter_h

#include "vtkExporter.h"
#include "vtkIOExportModule.h" // For export macro

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class VTKIOEXPORT_EXPORT vtkGLTFExporter : public vtkExporter
{
public:
  static vtkGLTFExporter* New();
  vtkTypeMacro(vtkGLTFExporter, vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the name of the GLTF file to write.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Should the binary data be included in the json file as a base64
   * string.
   */
  vtkGetMacro(InlineData, bool);
  vtkSetMacro(InlineData, bool);
  vtkBooleanMacro(InlineData, bool);
  ///@}

  ///@{
  /**
   * It looks for a point array called
   * NORMAL in the data and it saves it in the
   * GLTF file if found.
   * NORMAL is the vertex normal. Cesium needs this to render buildings correctly
   * if there is no texture.
   */
  vtkGetMacro(SaveNormal, bool);
  vtkSetMacro(SaveNormal, bool);
  vtkBooleanMacro(SaveNormal, bool);
  ///@}

  ///@{
  /**
   * It looks for point arrays called
   * _BATCHID in the data and it saves it in the
   * GLTF file if found.
   * _BATCHID is an index used in 3D Tiles b3dm format. This format stores
   * a binary gltf with a mesh that has several objects (buildings).
   * Objects are indexed from 0 to number of objects - 1, all points
   * of an objects have the same index. These index values are stored
   * in _BATCHID
   */
  vtkGetMacro(SaveBatchId, bool);
  vtkSetMacro(SaveBatchId, bool);
  vtkBooleanMacro(SaveBatchId, bool);
  ///@}

  ///@{
  /**
   * Set/Get weither NaN color is saved in the texture associated
   * to the mesh. Certain viewers do not support well the texture generated
   * with NaN colors, so consider disabling this unless NaN data is present.
   * Default value is true.
   */
  vtkGetMacro(SaveNaNValues, bool);
  vtkSetMacro(SaveNaNValues, bool);
  vtkBooleanMacro(SaveNaNValues, bool);
  ///@}

  /**
   * Write the result to a string instead of a file
   */
  std::string WriteToString();

  /**
   * Write the result to a provided ostream
   */
  void WriteToStream(ostream& out);

protected:
  vtkGLTFExporter();
  ~vtkGLTFExporter() override;

  void WriteData() override;

  char* FileName;
  bool InlineData;
  bool SaveNormal;
  bool SaveBatchId;

private:
  vtkGLTFExporter(const vtkGLTFExporter&) = delete;
  void operator=(const vtkGLTFExporter&) = delete;

  bool SaveNaNValues = true;
};

VTK_ABI_NAMESPACE_END
#endif
