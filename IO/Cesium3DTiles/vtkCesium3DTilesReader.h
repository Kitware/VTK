// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkCesium3DTilesReader
 * @brief   Reads a Cesium 3D Tiles tileset.
 *
 *
 * @sa
 * vtkPartitionedDataSetAlgorithm
 * vtkGLTFReader
 */

#ifndef vtkCesium3DTilesReader_h
#define vtkCesium3DTilesReader_h

#include "vtkIOCesium3DTilesModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

#include <memory>

VTK_ABI_NAMESPACE_BEGIN
class vtkPartitionedDataSet;
class vtkTransform;

/**
 * @class   vtkCesium3DTilesReader
 * @brief   Reads a Cesium 3D Tiles tileset
 *
 * Reads a Cesium 3D Tiles dataset as a vtkPartitionedDataSet. If the
 * reader is used in a parallel environment it will try to balance the
 * number of tiles read on each rank.  Currently, the reader only
 * works with tiles saved using GLTF, GLB or B3DM formats.  Point
 * coordinates in the produced VTK dataset are stored in Cartesian
 * coordinates (cart proj string), as they are in the tileset.Textures
 * are not used in the current version of the reader.
 *
 * @see vtkGeoTransform, vtkCesium3DTilesWriter
 */
class VTKIOCESIUM3DTILES_EXPORT vtkCesium3DTilesReader
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkCesium3DTilesReader* New();
  vtkTypeMacro(vtkCesium3DTilesReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the name of the file from which to read points.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Set/Get the Level (level of detail) in the tree where you want to read tiles from.
   * We start with root tile and then we refine each tile recursively until we reach Level.
   * Possible values are from 0 to NumberOfLevels - 1. Initialized to
   * NumberOfLevels - 1 (reads the most detailed tiles available)
   */
  void SetLevel(int level);
  vtkGetMacro(Level, int);
  ///@}

  /**
   * Returns true if it can read the json file (it is a 3D Tiles tileset), false otherwise
   */
  virtual int CanReadFile(VTK_FILEPATH const char* name);
  /**
   * Traverse the tree in postorder to compute the depth of the tree
   */
  int GetDepth(nlohmann::json& node);

protected:
  vtkCesium3DTilesReader();
  ~vtkCesium3DTilesReader() override;

  /**
   * Read tiles and add them to 'pd' for given this->Level and numberOfRanks/rank
   * combination. 'parentTransform' is used to accumulate transforms from the tileset.
   */
  void ReadTiles(vtkPartitionedDataSetCollection* pd, size_t numberOfRanks, size_t rank);
  /**
   * Reads the tile and transforms it.
   */
  vtkSmartPointer<vtkPartitionedDataSet> ReadTile(
    std::string tileFileName, vtkTransform* transform);
  /**
   * Converts globalIndex to  (tilesetIndex, tileIndex) pair.
   */
  std::pair<size_t, size_t> ToLocalIndex(size_t globalIndex);

  char* FileName = nullptr;
  std::string DirectoryName;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkCesium3DTilesReader(const vtkCesium3DTilesReader&) = delete;
  void operator=(const vtkCesium3DTilesReader&) = delete;

  int Level;
  class Tileset;
  friend class Tileset;
  /**
   * Tilesets (root and external), each tileset stores tiles on a
   * certain level.
   */
  std::vector<std::shared_ptr<Tileset>> Tilesets;
  std::map<std::string, size_t> FileNameToTilesetIndex;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkCesium3DTilesReader.h
