// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCesium3DTilesWriter.h"

#include "vtkCellArray.h"
#include "vtkCellCenters.h"
#include "vtkDirectory.h"
#include "vtkImageReader2.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtksys/FStream.hxx"
#include <vtkDataObjectTreeIterator.h>
#include <vtkIncrementalOctreeNode.h>
#include <vtkIncrementalOctreePointLocator.h>
#include <vtkJPEGReader.h>
#include <vtkLogger.h>
#include <vtkPNGReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkStringArray.h>
#include <vtkTexture.h>
#include <vtksys/SystemTools.hxx>

#include "TreeInformation.h"

#include <sstream>

using namespace vtksys;

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCesium3DTilesWriter);

namespace
{
//------------------------------------------------------------------------------
/**
 * Add building centers to the octree.
 */
vtkSmartPointer<vtkIncrementalOctreePointLocator> BuildOctreeBuildings(
  std::vector<vtkSmartPointer<vtkCompositeDataSet>>& buildings,
  const std::array<double, 6>& wholeBB, int buildingsPerTile)
{
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  vtkNew<vtkIncrementalOctreePointLocator> octree;
  octree->SetMaxPointsPerLeaf(buildingsPerTile);
  octree->InitPointInsertion(points, wholeBB.data());

  // TreeInformation::PrintBounds("octreeBB", wholeBB.data());
  for (size_t i = 0; i < buildings.size(); ++i)
  {
    double bb[6];
    buildings[i]->GetBounds(bb);
    double center[3] = { (bb[0] + bb[1]) / 2.0, (bb[2] + bb[3]) / 2, (bb[4] + bb[5]) / 2 };
    octree->InsertNextPoint(center);
    // std::cout << "insert: " << center[0] << ", " << center[1] << ", " << center[2]
    //           << " number of nodes: " << octree->GetNumberOfNodes() << std::endl;
  }
  return octree;
}

/**
 * Build octree for point cloud.
 */
vtkSmartPointer<vtkIncrementalOctreePointLocator> BuildOctreePoints(
  vtkPointSet* pointSet, int pointsPerTile)
{
  vtkNew<vtkIncrementalOctreePointLocator> octree;
  octree->SetMaxPointsPerLeaf(pointsPerTile);
  octree->SetDataSet(pointSet);
  octree->BuildLocator();
  return octree;
}

/**
 * Build octree for point cloud.
 */
vtkSmartPointer<vtkIncrementalOctreePointLocator> BuildOctreeMesh(
  vtkPolyData* polyData, int cellsPerTile)
{
  vtkNew<vtkCellCenters> computeCenters;
  computeCenters->SetInputData(polyData);
  computeCenters->Update();
  vtkPolyData* centers = computeCenters->GetOutput();

  vtkNew<vtkIncrementalOctreePointLocator> octree;
  octree->SetMaxPointsPerLeaf(cellsPerTile);
  octree->SetDataSet(centers);
  octree->BuildLocator();
  return octree;
}

//------------------------------------------------------------------------------
std::array<double, 6> TranslateBuildings(vtkMultiBlockDataSet* rootBuildings,
  const double* fileOffset, std::vector<vtkSmartPointer<vtkCompositeDataSet>>& buildings)
{
  std::array<double, 6> wholeBB;
  rootBuildings->GetBounds(wholeBB.data());

  vtkNew<vtkTransformFilter> f;
  vtkNew<vtkTransform> t;
  t->Identity();
  t->Translate(fileOffset);
  f->SetTransform(t);
  f->SetInputData(rootBuildings);
  f->Update();
  vtkMultiBlockDataSet* tr = vtkMultiBlockDataSet::SafeDownCast(f->GetOutputDataObject(0));
  tr->GetBounds(wholeBB.data());

  // generate normals - these are needed in Cesium if there are no textures
  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputDataObject(tr);
  normals->Update();
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(normals->GetOutputDataObject(0));

  auto buildingIt = vtk::TakeSmartPointer(mb->NewTreeIterator());
  buildingIt->VisitOnlyLeavesOff();
  buildingIt->TraverseSubTreeOff();
  for (buildingIt->InitTraversal(); !buildingIt->IsDoneWithTraversal(); buildingIt->GoToNextItem())
  {
    auto mbBuilding = vtkMultiBlockDataSet::SafeDownCast(buildingIt->GetCurrentDataObject());
    auto polyBuilding = vtkPolyData::SafeDownCast(buildingIt->GetCurrentDataObject());
    if (!mbBuilding)
    {
      if (polyBuilding)
      {
        auto newMbBuilding = vtkSmartPointer<vtkMultiBlockDataSet>::New();
        newMbBuilding->SetNumberOfBlocks(1);
        newMbBuilding->SetBlock(0, polyBuilding);
        buildings.emplace_back(newMbBuilding);
      }
      else
      {
        buildings.clear();
        return wholeBB;
      }
    }
    else
    {
      buildings.emplace_back(mbBuilding);
    }
  }
  return wholeBB;
}

template <typename T>
vtkSmartPointer<T> TranslateMeshOrPoints(T* rootPoints, const double* fileOffset)
{
  vtkSmartPointer<T> ret;
  vtkNew<vtkTransformFilter> f;
  vtkNew<vtkTransform> t;
  t->Identity();
  t->Translate(fileOffset);
  f->SetTransform(t);
  f->SetInputData(rootPoints);
  // generate normals - these are needed in Cesium if there are no textures
  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(f->GetOutputPort());
  normals->Update();
  ret = T::SafeDownCast(normals->GetOutputDataObject(0));
  return ret;
}

vtkPolyData* GetMesh(vtkMultiBlockDataSet* mbMesh)
{
  auto buildingIt = vtk::TakeSmartPointer(mbMesh->NewTreeIterator());
  buildingIt->VisitOnlyLeavesOff();
  buildingIt->TraverseSubTreeOff();
  buildingIt->InitTraversal();
  auto building = vtkMultiBlockDataSet::SafeDownCast(buildingIt->GetCurrentDataObject());
  auto it = vtk::TakeSmartPointer(building->NewIterator());
  it->InitTraversal();
  auto pd = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
  return pd;
}
}

//------------------------------------------------------------------------------
vtkCesium3DTilesWriter::vtkCesium3DTilesWriter()
{
  this->SetNumberOfInputPorts(1);
  this->DirectoryName = nullptr;
  this->TextureBaseDirectory = nullptr;
  this->PropertyTextureFile = nullptr;
  this->SetPropertyTextureFile("");
  std::fill(this->Offset, this->Offset + 3, 0);
  this->SaveTextures = true;
  this->SaveTiles = true;
  this->MergeTilePolyData = false;
  this->MergedTextureWidth = std::numeric_limits<int>::max();
  this->InputType = Buildings;
  this->ContentGLTF = false;
  this->ContentGLTFSaveGLB = true;
  this->NumberOfFeaturesPerTile = 100;
  this->CRS = nullptr;
}

//------------------------------------------------------------------------------
vtkCesium3DTilesWriter::~vtkCesium3DTilesWriter()
{
  this->SetDirectoryName(nullptr);
  this->SetTextureBaseDirectory(nullptr);
  this->SetPropertyTextureFile(nullptr);
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DirectoryName: " << (this->DirectoryName ? this->DirectoryName : "NONE")
     << indent
     << "TexturePath: " << (this->TextureBaseDirectory ? this->TextureBaseDirectory : "NONE")
     << endl;
}

//------------------------------------------------------------------------------
int vtkCesium3DTilesWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  if (this->InputType == Buildings)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  }
  else if (this->InputType == Points)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  }
  else if (this->InputType == Mesh)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  }
  else
  {
    vtkErrorMacro("Invalid InputType: " << this->InputType);
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesWriter::WriteData()
{
  auto root = this->GetInput(0);
  auto rootBuildings = vtkMultiBlockDataSet::SafeDownCast(root);
  auto rootPoints = vtkPointSet::SafeDownCast(root);
  auto mbMesh = vtkMultiBlockDataSet::SafeDownCast(root);
  switch (this->InputType)
  {
    case Buildings:
    {
      if (!rootBuildings)
      {
        vtkLog(ERROR,
          "Expecting vtkMultiBlockDataSet but got " << (root ? root->GetClassName() : "nullptr"));
        return;
      }
      std::vector<vtkSmartPointer<vtkCompositeDataSet>> buildings;
      vtkLog(INFO, "Translate buildings...");
      auto wholeBB = TranslateBuildings(rootBuildings, this->Offset, buildings);
      if (buildings.empty())
      {
        vtkLog(ERROR,
          "No buildings read from the input file. "
          "Maybe buildings are on a different LOD. Try changing --lod parameter.");
        return;
      }
      vtkLog(INFO, "Processing " << buildings.size() << " buildings...");
      vtkDirectory::MakeDirectory(this->DirectoryName);

      vtkSmartPointer<vtkIncrementalOctreePointLocator> octree =
        BuildOctreeBuildings(buildings, wholeBB, this->NumberOfFeaturesPerTile);
      TreeInformation treeInformation(octree->GetRoot(), octree->GetNumberOfNodes(), &buildings,
        this->TextureBaseDirectory, this->PropertyTextureFile, this->SaveTextures,
        this->ContentGLTF, this->ContentGLTFSaveGLB, this->CRS, this->DirectoryName);
      treeInformation.Compute();
      vtkLog(INFO, "Generating tileset.json for " << octree->GetNumberOfNodes() << " nodes...");
      treeInformation.SaveTileset(std::string(this->DirectoryName) + "/tileset.json");
      if (this->SaveTiles)
      {
        treeInformation.SaveTilesBuildings(this->MergeTilePolyData, this->MergedTextureWidth);
      }
      vtkLog(INFO, "Deleting objects ...");
      break;
    }
    case Points:
    {
      if (!rootPoints)
      {
        vtkLog(
          ERROR, "Expecting vtkPointSet but got " << (root ? root->GetClassName() : "nullptr"));
        return;
      }
      vtkDirectory::MakeDirectory(this->DirectoryName);
      vtkSmartPointer<vtkPointSet> pc = TranslateMeshOrPoints(rootPoints, this->Offset);
      vtkSmartPointer<vtkIncrementalOctreePointLocator> octree =
        BuildOctreePoints(pc, this->NumberOfFeaturesPerTile);
      TreeInformation treeInformation(octree->GetRoot(), octree->GetNumberOfNodes(), pc,
        this->ContentGLTF, this->ContentGLTFSaveGLB, this->CRS, this->DirectoryName);
      treeInformation.Compute();
      vtkLog(INFO, "Generating tileset.json for " << octree->GetNumberOfNodes() << " nodes...");
      treeInformation.SaveTileset(std::string(this->DirectoryName) + "/tileset.json");
      if (this->SaveTiles)
      {
        treeInformation.SaveTilesPoints();
      }
      vtkLog(INFO, "Deleting objects ...");
      break;
    }
    case Mesh:
    {
      if (!mbMesh)
      {
        vtkLog(ERROR,
          "Expecting vtkMultiBlockDataSet but got " << (root ? root->GetClassName() : "nullptr"));
        return;
      }

      vtkPolyData* rootMesh = GetMesh(mbMesh);
      vtkDirectory::MakeDirectory(this->DirectoryName);
      vtkSmartPointer<vtkPolyData> pc = TranslateMeshOrPoints(rootMesh, this->Offset);
      vtkSmartPointer<vtkIncrementalOctreePointLocator> octree =
        BuildOctreeMesh(pc, this->NumberOfFeaturesPerTile);
      TreeInformation treeInformation(octree->GetRoot(), octree->GetNumberOfNodes(), pc,
        this->TextureBaseDirectory, this->PropertyTextureFile, this->SaveTextures,
        this->ContentGLTF, this->ContentGLTFSaveGLB, this->CRS, this->DirectoryName);
      treeInformation.Compute();
      vtkLog(INFO, "Generating tileset.json for " << octree->GetNumberOfNodes() << " nodes...");
      treeInformation.SaveTileset(std::string(this->DirectoryName) + "/tileset.json");
      if (this->SaveTiles)
      {
        treeInformation.SaveTilesMesh();
      }
      vtkLog(INFO, "Deleting objects ...");
      break;
    }
  }
}
VTK_ABI_NAMESPACE_END
