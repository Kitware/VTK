// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOBJWriter
 * @brief   write wavefront obj file
 *
 * vtkOBJWriter writes wavefront obj (.obj) files in ASCII form.
 * OBJ files contain the geometry including lines, triangles and polygons.
 * Normals and texture coordinates on points are also written if they exist.
 * One can specify a texture passing a vtkImageData on port 1.
 * If a texture is set, additional .mtl and .png files are generated. Those files have the same
 * name without obj extension.
 * Alternatively, one can specify a TextureFileName pointing to an existing texture.
 * In this case a .mtl file is generated pointing to the specified file.
 *
 */

#ifndef vtkOBJWriter_h
#define vtkOBJWriter_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkImageData;
class vtkPolyData;

class VTKIOGEOMETRY_EXPORT vtkOBJWriter : public vtkWriter
{
public:
  static vtkOBJWriter* New();
  vtkTypeMacro(vtkOBJWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the inputs to this writer.
   */
  vtkPolyData* GetInputGeometry();
  vtkImageData* GetInputTexture();
  vtkDataSet* GetInput(int port);
  ///@}

  ///@{
  /**
   * Get/Set the path to an existing texture file for the OBJ.
   * If this is set, the writer will generate mtllib, usemtl lines
   * and a .mtl file that points to the existing texture file.
   */
  vtkSetFilePathMacro(TextureFileName);
  vtkGetFilePathMacro(TextureFileName);
  ///@}

  ///@{
  /**
   * Get/Set the file name of the OBJ file.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

protected:
  vtkOBJWriter();
  ~vtkOBJWriter() override;

  void WriteData() override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  char* FileName;
  char* TextureFileName;

private:
  vtkOBJWriter(const vtkOBJWriter&) = delete;
  void operator=(const vtkOBJWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
