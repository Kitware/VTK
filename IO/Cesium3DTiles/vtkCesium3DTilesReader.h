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
#include "vtkPartitionedDataSetAlgorithm.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN

/**
 * @class   vtkCesium3DTilesReader
 * @brief   Reads a Cesium 3D Tiles tileset
 *
 * Reads a Cesium 3D Tiles dataset as a vtkPartitionedDataSet. If the
 * reader is used in a parallel environment it will try to balance the
 * number of tiles read on each rank.  Currently, the reader only
 * works with tiles saved using GLTF or GLB formats.  Point
 * coordinates in the produced VTK dataset are stored in Cartesian
 * coordinates (cart proj string), as they are in the tileset. Textures are not used
 * in the current version of the reader.
 *
 * @see vtkGeoTransform, vtkCesium3DTilesWriter
 */
class VTKIOCESIUM3DTILES_EXPORT vtkCesium3DTilesReader : public vtkPartitionedDataSetAlgorithm
{
public:
  static vtkCesium3DTilesReader* New();
  vtkTypeMacro(vtkCesium3DTilesReader, vtkPartitionedDataSetAlgorithm);
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
  vtkSetMacro(Level, int);
  vtkGetMacro(Level, int);
  ///@}

  ///@{
  /**
   * Get the number of levels in the tileset.
   */
  vtkGetMacro(NumberOfLevels, int);
  ///@}

protected:
  vtkCesium3DTilesReader();
  ~vtkCesium3DTilesReader() override;

  char* FileName = nullptr;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkCesium3DTilesReader(const vtkCesium3DTilesReader&) = delete;
  void operator=(const vtkCesium3DTilesReader&) = delete;

  int Level;
  int NumberOfLevels;

  class Implementation;
  friend class Implementation;
  Implementation* Impl;
};

VTK_ABI_NAMESPACE_END
#endif
