/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TreeInformation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class TreeInformation
 * @brief Additional information and routines for 3D Tiles octree nodes.
 *
 * Additional information for all nodes in the octree used to generate
 * the 3D Tiles representation.
 */

#ifndef TreeInformation_h
#define TreeInformation_h

#include "vtkCesium3DTilesWriter.h"
#include <vtkSmartPointer.h>

#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

#include <array>
#include <vector>

class vtkActor;
class vtkCompositeDataSet;
class vtkIdList;
class vtkImageData;
class vtkIntArray;
class vtkPolyData;
class vtkPointSet;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkIncrementalOctreeNode;

class TreeInformation
{
public:
  ///@{
  /**
   * Constructors for buildings, points and meshes.
   */
  TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes,
    const std::vector<vtkSmartPointer<vtkCompositeDataSet>>* buildings,
    const std::string& textureBaseDirectory, bool saveTextures, bool contentGLTF, const char* crs,
    const std::string& outputDir);
  TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes, vtkPointSet* points,
    bool contentGLTF, const char* crs, const std::string& output);
  TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes, vtkPolyData* mesh,
    const std::string& textureBaseDirectory, bool saveTextures, bool contentGLTF, const char* crs,
    const std::string& output);
  ///@}

  void PrintNode(vtkIncrementalOctreeNode* node);

  ///@{
  /**
   * Returns the bounds for node with index 'i'
   * The versions that returns a bool returns true if the node is not empty,
   * false otherwise. For the third version we read the node index from 'node'.
   */
  std::array<double, 6> GetNodeTightBounds(int i) { return NodeTightBounds[i]; }
  bool GetNodeTightBounds(int i, double* bounds);
  static bool GetNodeTightBounds(void* data, vtkIncrementalOctreeNode* node, double* bounds);
  ///@}

  /**
   * Adds a node geometric error cell attribute for the bounding
   * box representation for nodes on a level.
   * Works on the poly data generated for a tree level by
   * vtkIncrementalOctrePointLocator::GenerateRepresentation.
   */
  void AddGeometricError(vtkPolyData* representation);
  /**
   * Computes the additional information for all nodes. This includes
   * the tight bounding box around the buildings, if the node is empty or not,
   * and the geometric error.
   */
  void Compute();
  void SaveTilesBuildings(bool mergeTilePolyData);
  void SaveTilesMesh();
  void SaveTilesPoints();
  void SaveTileset(const std::string& output);
  static void PrintBounds(const char* name, const double* bounds);
  static void PrintBounds(const std::string& name, const double* bounds)
  {
    PrintBounds(name.c_str(), bounds);
  }
  static std::array<double, 6> ExpandBounds(double* first, double* second);

protected:
  void PostOrderTraversal(void (TreeInformation::*Visit)(vtkIncrementalOctreeNode* node, void* aux),
    vtkIncrementalOctreeNode* node, void* aux);
  void PreOrderTraversal(void (TreeInformation::*Visit)(vtkIncrementalOctreeNode* node, void* aux),
    vtkIncrementalOctreeNode* node, void* aux);
  void SaveTileset(vtkIncrementalOctreeNode* root, const std::string& output);
  nlohmann::json GenerateTileJson(vtkIncrementalOctreeNode* node);
  bool ConvertTileCartesianBuildings(vtkIncrementalOctreeNode* node);
  bool ConvertDataSetCartesian(vtkPointSet* points);

  ///@{
  /**
   * Computes the additional information for 'node'. This includes
   * the tight bounding box around the buildings, if the node is empty or not,
   * and the geometric error.
   */
  void VisitCompute(vtkIncrementalOctreeNode* node, void* aux);
  void VisitComputeGeometricError(vtkIncrementalOctreeNode* node, void* aux);
  ///@}
  void SaveTileBuildings(vtkIncrementalOctreeNode* node, void* auxData);
  void SaveTileMesh(vtkIncrementalOctreeNode* node, void* auxData);
  /**
   * Compute the texture image for the tile and recompute texture coordinates
   */
  vtkSmartPointer<vtkImageData> ComputeTileMeshTexture(
    vtkPolyData* tileMesh, vtkImageData* textureImage);
  void SaveTilePoints(vtkIncrementalOctreeNode* node, void* auxData);

  ///@{
  /**
   * Compute geometric error for the tileset and for a node.
   */
  double ComputeGeometricErrorTilesetBuildings();
  double ComputeGeometricErrorTilesetPoints();
  double ComputeGeometricErrorTilesetMesh();
  double ComputeGeometricErrorTileset();
  double ComputeGeometricErrorNodeBuildings(vtkIncrementalOctreeNode* node, void* aux);
  double ComputeGeometricErrorNodePoints(vtkIncrementalOctreeNode* node, void* aux);
  double ComputeGeometricErrorNodeMesh(vtkIncrementalOctreeNode* node, void* aux);
  double ComputeGeometricErrorNode(vtkIncrementalOctreeNode* node, void* aux);
  ///@}
  std::array<double, 6> ComputeTightBB(vtkIdList* tileBuildings);
  std::string ContentTypeExtension() const;
  void Initialize();
  double GetRootLength2();

private:
  /**
   * Buildings, Points or Mesh. @see vtkCesium3DTilesWriter::InputType
   */
  enum vtkCesium3DTilesWriter::InputType InputType;
  vtkIncrementalOctreeNode* Root;
  ///@{
  /**
   * buildings indexed by building ID, Points or Mesh input.
   */
  const std::vector<vtkSmartPointer<vtkCompositeDataSet>>* Buildings;
  vtkPointSet* Points;
  vtkPolyData* Mesh;
  ///@}

  std::string OutputDir;
  std::string TextureBaseDirectory;
  bool SaveTextures;
  bool ContentGLTF;

  const char* CRS;
  /**
   * tight bounds indexed by tile ID
   */
  std::vector<std::array<double, 6>> NodeTightBounds;
  /**
   * You can have leaf nodes that are empty, that is they don't have any points.
   * indexed by tile ID.
   */
  std::vector<bool> EmptyNode;
  /**
   * volume difference between rendering this node and rendering the most detailed model.
   * indexed by tile ID
   */
  std::vector<double> GeometricError;
  nlohmann::json RootJson;
};

#endif
// VTK-HeaderTest-Exclude: TreeInformation.h
