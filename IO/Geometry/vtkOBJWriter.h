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
class vtkDataSetAttributes;
class vtkImageData;
class vtkPolyData;
class vtkUnsignedCharArray;

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

  ///@{
  /**
   * These methods enable the user to control how to add color into the OBJ
   * output file. The default behavior is as follows. The user provides the
   * name of an array. If the type of the array is three components, unsigned
   * char, then the data is written as three separate "red", "green" and "blue"
   * properties. If the type of the array is four components, unsigned char,
   * then the data is written as three separate "red", "green", "blue" and "alpha"
   * properties. If the array is of any other type, then no color information is
   * written.
   * If WriteColorArray is off, no color information will be written into the OBJ file.
   * Default is WriteColorArray off.
   */
  vtkSetMacro(WriteColorArray, bool);
  vtkGetMacro(WriteColorArray, bool);
  vtkBooleanMacro(WriteColorArray, bool);
  ///@}

  ///@{
  /**
   * Specify the array name to use to color the data.
   */
  vtkSetMacro(ColorArrayName, std::string);
  vtkGetMacro(ColorArrayName, std::string);
  ///@}

  ///@{
  /**
   * Get/Set whether to use relative path for texture file in the .mtl file.
   * Default is false.
   */
  vtkSetMacro(UseRelativeTexturePath, bool);
  vtkGetMacro(UseRelativeTexturePath, bool);
  vtkBooleanMacro(UseRelativeTexturePath, bool);
  ///@}

protected:
  vtkOBJWriter();
  ~vtkOBJWriter() override;

  bool WriteDataAndReturn() override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkOBJWriter(const vtkOBJWriter&) = delete;
  void operator=(const vtkOBJWriter&) = delete;

  char* FileName{ nullptr };
  char* TextureFileName{ nullptr };

  std::string ColorArrayName;
  bool WriteColorArray{ false };

  bool UseRelativeTexturePath{ false };
};

VTK_ABI_NAMESPACE_END
#endif
