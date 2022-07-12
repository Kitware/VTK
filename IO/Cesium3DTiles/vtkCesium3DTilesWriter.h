/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCesium3DTilesWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCesium3DTilesWriter
 * @brief   Writes a dataset into 3D Tiles format.
 *
 *
 * Valid inputs include the vtkMultiBlockDataSet (as created by
 * vtkCityGMLReader) storing 3D buidlings, vtkPointSet storing a point
 * cloud or vtkPolyData for storing a mesh.
 *
 */

#ifndef vtkCesium3DTilesWriter_h
#define vtkCesium3DTilesWriter_h

#include "vtkIOCesium3DTilesModule.h" // For export macro
#include "vtkWriter.h"

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
  //@

  ///@{
  /**
   * Save the tiles (B3DMs) as part of the 3D Tiles dataset. Default true.
   * Otherwise save only the tileset (JSON) file. This is mainly used for
   * debugging. Default true.
   */
  vtkSetMacro(SaveTiles, bool);
  vtkGetMacro(SaveTiles, bool);
  vtkBooleanMacro(SaveTiles, bool);
  //@

  ///@{
  /**
   * Merge all meshes in each tile to end up with one mesh per tile.
   * Default is false which means that we expect an external program to merge
   * the meshes in each tile to improve performance (such as meshoptimizer).
   * otherwise we merge the polydata in VTK.
   * @see
   * https://meshoptimizer.org/
   */
  vtkSetMacro(MergeTilePolyData, bool);
  vtkGetMacro(MergeTilePolyData, bool);
  vtkBooleanMacro(MergeTilePolyData, bool);
  //@

  ///@{
  /**
   * What is the file type used to save tiles. If ContentGLTF is false
   * (the default) we use B3DM for Buildings or Mesh and PNTS for
   * PointCloud otherwise  we use GLB (3DTILES_content_gltf
   * extension).  If the file type is B3DM or GLB, external programs are
   * needed to convert GLTF -> GLB -> B3DM.
   *
   */
  vtkSetMacro(ContentGLTF, bool);
  vtkGetMacro(ContentGLTF, bool);
  vtkBooleanMacro(ContentGLTF, bool);
  //@

  ///@{
  /**
   * Input is Buildings, Points or Mesh.
   */
  vtkSetMacro(InputType, int);
  vtkGetMacro(InputType, int);
  //@

  ///@{
  /**
   * Maximum number of buildings per tile in case of buildings input or
   * the number of points per tile in case of point cloud input. Default is 100.
   */
  vtkSetMacro(NumberOfFeaturesPerTile, int);
  vtkGetMacro(NumberOfFeaturesPerTile, int);
  //@

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
  double Offset[3];
  bool SaveTextures;
  int InputType;
  bool ContentGLTF;
  bool SaveTiles;
  bool MergeTilePolyData;
  int NumberOfFeaturesPerTile;
  char* CRS;

private:
  vtkCesium3DTilesWriter(const vtkCesium3DTilesWriter&) = delete;
  void operator=(const vtkCesium3DTilesWriter&) = delete;
};

#endif // vtkCesium3DTilesWriter_h
