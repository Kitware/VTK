// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGLTFWriter
 * @brief   export a scene into GLTF 2.0 format.
 *
 * vtkGLTFWriter is a concrete subclass of vtkWriter that writes GLTF
 * 2.0 files. Its input is a multiblock dataset as it is produced by
 * the CityGML reader. The dataset contains a list of buildings, a mesh
 * or a point cloud.
 *
 * For buildings, each building is made of pieces (polydata), each
 * piece could potentially have several textures. The mesh input
 * is the same as one building. The point cloud input, is the same as
 * mesh input but with Verts cells instead of Polys.

 * Materials, including textures, are described as fields in the
 * polydata. If InlineData is false, we only refer to textures files
 * referred in the data, otherwise we read the textures and save them
 * encoded in the file.
 *
 * @sa
 * vtkCityGMLReader
 * vtkPolyData
 */

#ifndef vtkGLTFWriter_h
#define vtkGLTFWriter_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkWriter.h"

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiBlockDataSet;
class VTKIOGEOMETRY_EXPORT vtkGLTFWriter : public vtkWriter
{
public:
  static vtkGLTFWriter* New();
  vtkTypeMacro(vtkGLTFWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the name of the GLTF file to write.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify the base directory for texture files.
   */
  vtkSetStringMacro(TextureBaseDirectory);
  vtkGetStringMacro(TextureBaseDirectory);
  ///@}

  ///@{
  /**
   * Specify the property texture file.
   * This is a json file described by
   https://github.com/CesiumGS/3d-tiles/tree/main/specification/Metadata
   and
   https://github.com/CesiumGS/glTF/tree/3d-tiles-next/extensions/2.0/Vendor/EXT_structural_metadata
   */
  vtkSetStringMacro(PropertyTextureFile);
  vtkGetStringMacro(PropertyTextureFile);
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
   * It looks for the normals point attribute and saves it in the
   * GLTF file if found with the name NORMAL
   * Cesium needs this to render buildings correctly
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
   * If true (default) we save textures. We only include a reference
   * to the texture file unless CopyTextures is true or you want to
   * include the binary data in the json file using InlineData in
   * which case we have to load the texture in memory and save it
   * encoded in the json file.
   * @sa TextureBaseDirectory
   */
  vtkGetMacro(SaveTextures, bool);
  vtkSetMacro(SaveTextures, bool);
  vtkBooleanMacro(SaveTextures, bool);
  ///@}

  ///@{
  /**
   * If true we copy the textures the the same directory where FileName is saved.
   * Default is false.
   * @sa TextureBaseDirectory
   */
  vtkGetMacro(CopyTextures, bool);
  vtkSetMacro(CopyTextures, bool);
  vtkBooleanMacro(CopyTextures, bool);
  ///@}

  ///@{
  /**
   * If true, the writer looks at the active scalar and if it finds a
   * 3 or 4 component, float, unsigned char or unsigned short it
   * stores a RGB or RGBA attribute called COLOR_0 in the GLTF
   * file. The default is false.  Note a float component has to be
   * bewtween [0, 1] while the unsigned chars and unsigned short are
   * OpenGL normalized integers (are either between [0, 255] for
   * unsigned char, they are between [0, 65536] for unsigned short - they
   * are used to quantize a float between [0, 1]).
   */
  vtkGetMacro(SaveActivePointColor, bool);
  vtkSetMacro(SaveActivePointColor, bool);
  vtkBooleanMacro(SaveActivePointColor, bool);
  ///@}

  ///@{
  /**
   * Save mesh point coordinates relative to the bounding box origin
   * and add the corresponding translation to the root node.  This is
   * especially important for 3D Tiles as points are stored as
   * cartesian coordinates relative to the earth center so they are
   * stored as doubles. As GLTF can only store floats not setting this
   * variable on results in a loss of precision of about a meter.
   * Note that the translation information is stored in json which can
   * store doubles.
   */
  vtkGetMacro(RelativeCoordinates, bool);
  vtkSetMacro(RelativeCoordinates, bool);
  vtkBooleanMacro(RelativeCoordinates, bool);
  ///@}

  ///@{
  /**
   * If true, save as GLB (Binary GLTF).
   * This is set by using .glb extension for FileName
   * and unset for any other extension (usually .gltf)
   */
  vtkGetMacro(Binary, bool);
  ///@}

  /**
   * Write the result to a string instead of a file
   */
  std::string WriteToString();

  /**
   * Write the result to a provided ostream
   */
  void WriteToStream(ostream& out, vtkDataObject* in);
  /**
   * This is used to read texture_uri fields that contain
   * a list of texture paths
   * @see vtkCityGMLReader
   */
  static std::vector<std::string> GetFieldAsStringVector(vtkDataObject* obj, const char* name);

protected:
  vtkGLTFWriter();
  ~vtkGLTFWriter() override;

  void WriteData() override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  void WriteToStreamMultiBlock(ostream& out, vtkMultiBlockDataSet* in);

  char* FileName;
  char* TextureBaseDirectory;
  char* PropertyTextureFile;
  bool InlineData;
  bool SaveNormal;
  bool SaveBatchId;
  bool SaveTextures;
  bool RelativeCoordinates;
  bool CopyTextures;
  bool SaveActivePointColor;
  bool Binary;

private:
  vtkGLTFWriter(const vtkGLTFWriter&) = delete;
  void operator=(const vtkGLTFWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
