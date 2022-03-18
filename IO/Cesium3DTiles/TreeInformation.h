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
 * @brief Additional information and routins for 3D Tiles octree nodes.
 *
 * Additional information for all nodes in the octree used to generate
 * the 3D Tiles representation.
 */

#ifndef TreeInformation_h
#define TreeInformation_h

#include <vtkSmartPointer.h>

#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

#include <array>
#include <vector>

class vtkActor;
class vtkCompositeDataSet;
class vtkIdList;
class vtkIntArray;
class vtkPolyData;
class vtkPointSet;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkIncrementalOctreeNode;

class TreeInformation
{
public:
  TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes,
    const std::vector<vtkSmartPointer<vtkCompositeDataSet>>* buildings,
    const std::string& outputDir, const std::string& texturePath, bool saveTextures,
    int contentType, const char* crs);
  TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes, vtkPointSet* points,
    const std::string& output, const char* crs);

  void PrintNode(vtkIncrementalOctreeNode* node);

  //@{
  /**
   * Returns the bounds for node with index 'i'
   * The versions that returns a bool returns true if the node is not empty,
   * false otherwise. For the third version we read the node index from 'node'.
   */
  std::array<double, 6> GetNodeBounds(int i) { return NodeBounds[i]; }
  bool GetNodeBounds(int i, double* bounds);
  static bool GetNodeBounds(void* data, vtkIncrementalOctreeNode* node, double* bounds);
  //@}

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
  void SaveTilesGLTF(bool mergeTilePolyData);
  void SaveTilesPnts();
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
  void SaveTileset(vtkIncrementalOctreeNode* root, const std::string& output);
  nlohmann::json GenerateTileJson(vtkIncrementalOctreeNode* node);
  /**
   * Computes the additional information for 'node'. This includes
   * the tight bounding box around the buildings, if the node is empty or not,
   * and the geometric error.
   */
  void Compute(vtkIncrementalOctreeNode* node, void* aux);
  void SaveTileGLTF(vtkIncrementalOctreeNode* node, void* auxData);
  void SaveTilePnts(vtkIncrementalOctreeNode* node, void* auxData);
  double ComputeTilesetGeometricError();
  std::array<double, 6> ComputeTightBB(vtkIdList* tileBuildings);

private:
  /**
   * Buildings, Points or Mesh. @see vtkCesium3DTilesWriter::InputType
   */
  int InputType;
  vtkIncrementalOctreeNode* Root;
  /**
   * buildings input indexed by building ID
   */
  const std::vector<vtkSmartPointer<vtkCompositeDataSet>>* Buildings;
  /**
   * point cloud input
   */
  vtkPointSet* Points;

  std::string OutputDir;
  std::string TexturePath;
  bool SaveTextures;
  int BuildingContentType;

  const char* CRS;
  /**
   * tight bounds indexed by tile ID
   */
  std::vector<std::array<double, 6>> NodeBounds;
  std::vector<bool> EmptyNode;
  /**
   * volume difference between rendering this node and rendering the most detailed model.
   * indexed by node ID
   */
  std::vector<double> GeometricError;
  nlohmann::json RootJson;
};

#endif
// VTK-HeaderTest-Exclude: TreeInformation.h
