// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCesium3DTilesWriter
 * @brief   Writes a dataset into 3D Tiles format.
 *
 *
 * Valid inputs include the vtkMultiBlockDataSet (as created by
 * vtkCityGMLReader) storing 3D buildings, vtkPointSet storing a point
 * cloud or vtkPolyData for storing a mesh.
 *
 * @sa
 * vtkCityGMLReader
 * vtkMultiBlockDataSet
 * vtkPolyData
 */

#ifndef vtkCesium3DTilesWriter_h
#define vtkCesium3DTilesWriter_h

#include "vtkIOCesium3DTilesModule.h" // For export macro
#include "vtkWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOCESIUM3DTILES_EXPORT vtkCesium3DTilesWriter : public vtkWriter
{
public:
  static vtkCesium3DTilesWriter* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkCesium3DTilesWriter, vtkWriter);

  enum InputType
  {
    Buildings,
    Points,
    Mesh
  };

  ///@{
  /**
   * Accessor for name of the directory where Cesium3DTiles data is written
   */
  vtkSetFilePathMacro(DirectoryName);
  vtkGetFilePathMacro(DirectoryName);
  ///@}

  ///@{
  /**
   * Path used to prefix all texture paths stored as fields in the input data.
   * @see vtkCityGMLReader
   */
  vtkSetFilePathMacro(TextureBaseDirectory);
  vtkGetFilePathMacro(TextureBaseDirectory);
  ///@}

  ///@{
  /**
   * Optional property texture mapping for the whole dataset.
   * This is a json file described in <a
   href="https://github.com/CesiumGS/3d-tiles/tree/main/specification/Metadata">3D Metadata</a> and
    <a
   href="https://github.com/CesiumGS/glTF/tree/3d-tiles-next/extensions/2.0/Vendor/EXT_structural_metadata">EXT_structural_metadata</a>
   * @see vtkCityGMLReader
   */
  vtkSetFilePathMacro(PropertyTextureFile);
  vtkGetFilePathMacro(PropertyTextureFile);
  ///@}

  ///@{
  /**
   * Data coordinates are relative to this origin. To get the actual
   * coordinates data has to be translated with the Offset.
   */
  vtkSetVector3Macro(Offset, double);
  vtkGetVector3Macro(Offset, double);
  ///@}

  ///@{
  /**
   * Save textures as part of the 3D Tiles dataset. Default true.
   * Otherwise save only the mesh.
   */
  vtkSetMacro(SaveTextures, bool);
  vtkGetMacro(SaveTextures, bool);
  vtkBooleanMacro(SaveTextures, bool);
  ///@}

  ///@{
  /**
   * Save the tiles (B3DMs) as part of the 3D Tiles dataset. Default true.
   * Otherwise save only the tileset (JSON) file. This is mainly used for
   * debugging. Default true.
   */
  vtkSetMacro(SaveTiles, bool);
  vtkGetMacro(SaveTiles, bool);
  vtkBooleanMacro(SaveTiles, bool);
  ///@}

  ///@{
  /**
   * Merge all meshes in each tile so we end up with one mesh per tile.
   * If polydata has textures we merged textures as well such that
   * the width of the resulting texture is less then MergedTextureWidth (this is
   * measured in number of input textures). If MergedTextureWidth is not specified
   * it is computed as sqrt of the number of input textures.
   * Default is false which means that we expect an external program to merge
   * the meshes in each tile to improve performance (such as meshoptimizer).
   * otherwise we merge the polydata in VTK.
   * @see
   * https://meshoptimizer.org/
   */
  vtkSetMacro(MergeTilePolyData, bool);
  vtkGetMacro(MergeTilePolyData, bool);
  vtkBooleanMacro(MergeTilePolyData, bool);
  vtkSetMacro(MergedTextureWidth, int);
  vtkGetMacro(MergedTextureWidth, int);
  ///@}

  ///@{
  /**
   * What is the file type used to save tiles. If ContentGLTF is false
   * (the default) we use B3DM for Buildings or Mesh and PNTS for
   * PointCloud otherwise  we use GLB or GLTF (3DTILES_content_gltf
   * extension, use GLB if ContentGLTFSaveGLB is true (default is true)).
   * If the file type is B3DM, external programs are
   * needed to convert GLB -> B3DM.
   *
   */
  vtkSetMacro(ContentGLTF, bool);
  vtkGetMacro(ContentGLTF, bool);
  vtkBooleanMacro(ContentGLTF, bool);
  vtkSetMacro(ContentGLTFSaveGLB, bool);
  vtkGetMacro(ContentGLTFSaveGLB, bool);
  vtkBooleanMacro(ContentGLTFSaveGLB, bool);
  ///@}

  ///@{
  /**
   * Input is Buildings, Points or Mesh.
   */
  vtkSetMacro(InputType, int);
  vtkGetMacro(InputType, int);
  ///@}

  ///@{
  /**
   * Maximum number of buildings per tile in case of buildings input or
   * the number of points per tile in case of point cloud input. Default is 100.
   */
  vtkSetMacro(NumberOfFeaturesPerTile, int);
  vtkGetMacro(NumberOfFeaturesPerTile, int);
  ///@}

  ///@{
  /**
   * Set the coordinate reference system (CRS) also known as spatial reference system (SRC),
   * such as EPSG:2263. This string can also be a proj string such as
   * "+proj=utm +zone=17 +datum=WGS84"
   */
  vtkSetStringMacro(CRS);
  vtkGetStringMacro(CRS);
  ///@}

protected:
  vtkCesium3DTilesWriter();
  ~vtkCesium3DTilesWriter() override;

  // Only accepts vtkMultiBlockData
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Implementation of Write()
  void WriteData() override;

  char* DirectoryName;
  char* TextureBaseDirectory;
  char* PropertyTextureFile;
  double Offset[3];
  bool SaveTextures;
  int InputType;
  bool ContentGLTF;
  bool ContentGLTFSaveGLB;
  bool SaveTiles;
  bool MergeTilePolyData;
  int MergedTextureWidth;
  int NumberOfFeaturesPerTile;
  char* CRS;

private:
  vtkCesium3DTilesWriter(const vtkCesium3DTilesWriter&) = delete;
  void operator=(const vtkCesium3DTilesWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCesium3DTilesWriter_h
