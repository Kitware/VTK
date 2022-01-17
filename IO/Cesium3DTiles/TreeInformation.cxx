/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TreeInformation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TreeInformation.h"

#include "vtkCesium3DTilesWriter.h"
#include "vtkGLTFWriter.h"
#include "vtkMultiBlockDataSet.h"
#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkCellData.h>
#include <vtkCompositeDataSet.h>
#include <vtkDirectory.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIncrementalOctreeNode.h>
#include <vtkLogger.h>
#include <vtkMath.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

#include <vtk_libproj.h>

#include <sstream>
#include <vtksys/FStream.hxx>

using namespace nlohmann;

namespace
{
//------------------------------------------------------------------------------
/**
 * Compute the tight bounding box around all buildings in a tile.
 * 'list', which stores all buildings in a tile as indexes into 'buildings' vector
 * which stores all buildings.
 */
std::array<double, 6> ComputeTightBoudingBox(
  const std::vector<vtkSmartPointer<vtkCompositeDataSet>>& buildings, vtkIdList* tileBuildings)
{
  std::array<double, 6> wholeBB = { std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest() };
  for (int i = 0; i < tileBuildings->GetNumberOfIds(); ++i)
  {
    double bb[6];
    buildings[tileBuildings->GetId(i)]->GetBounds(bb);
    wholeBB = TreeInformation::ExpandBounds(&wholeBB[0], bb);
  }
  return wholeBB;
}

//------------------------------------------------------------------------------
/**
 * bb: xmin, xmax, ymin, ymax, zmin, zmax
 * Return: west, south, east, north, zmin, zmax
 */
std::array<double, 6> ToLonLatRadiansHeight(const char* crs, const std::array<double, 6>& bb)
{
  std::array<double, 6> lonlatheight;
  lonlatheight[4] = bb[4];
  lonlatheight[5] = bb[5];
  std::ostringstream ostr;
  PJ* P;
  P = proj_create_crs_to_crs(PJ_DEFAULT_CTX, crs, "+proj=longlat +ellps=WGS84", nullptr);
  if (P == nullptr)
  {
    vtkLog(ERROR, "proj_create_crs_to_crs failed: " << proj_errno_string(proj_errno(nullptr)));
    return lonlatheight;
  }
  {
    /* For that particular use case, this is not needed. */
    /* proj_normalize_for_visualization() ensures that the coordinate */
    /* order expected and returned by proj_trans() will be longitude, */
    /* latitude for geographic CRS, and easting, northing for projected */
    /* CRS. If instead of using PROJ strings as above, "EPSG:XXXX" codes */
    /* had been used, this might had been necessary. */
    PJ* P_for_GIS = proj_normalize_for_visualization(PJ_DEFAULT_CTX, P);
    if (P_for_GIS == nullptr)
    {
      proj_destroy(P);
      vtkLog(ERROR,
        "proj_normalize_for_visualization failed: " << proj_errno_string(proj_errno(nullptr)));
      return lonlatheight;
    }
    proj_destroy(P);
    P = P_for_GIS;
  }
  PJ_COORD c, c_out;
  for (int i = 0; i < 2; ++i)
  {
    c.xy.x = bb[i];
    c.xy.y = bb[i + 2];
    c_out = proj_trans(P, PJ_FWD, c);
    lonlatheight[2 * i] = vtkMath::RadiansFromDegrees(c_out.lp.lam);
    lonlatheight[2 * i + 1] = vtkMath::RadiansFromDegrees(c_out.lp.phi);
  }
  proj_destroy(P);
  // std::cout << lonlatheight[0] << " "
  //           << lonlatheight[1] << " "
  //           << lonlatheight[2] << " "
  //           << lonlatheight[3] << std::endl;
  return lonlatheight;
}

std::array<std::string, 3> ContentTypeExtension = { ".b3dm", ".glb", ".gltf" };
}

//------------------------------------------------------------------------------
TreeInformation::TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes,
  const std::vector<vtkSmartPointer<vtkCompositeDataSet>>& buildings, const std::string& output,
  const std::string& texturePath, bool saveTextures, int contentType, const char* crs)
  :

  Root(root)
  , Buildings(buildings)
  , OutputDir(output)
  , TexturePath(texturePath)
  , SaveTextures(saveTextures)
  , ContentType(contentType)
  , CRS(crs)
  , NodeBounds(numberOfNodes)
  , EmptyNode(numberOfNodes)
  , GeometricError(numberOfNodes)
  , VolumeError(numberOfNodes)
{
  std::array<double, 6> a = { std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest() };
  std::fill(this->NodeBounds.begin(), this->NodeBounds.end(), a);
  std::fill(this->EmptyNode.begin(), this->EmptyNode.end(), true);
  std::fill(this->GeometricError.begin(), this->GeometricError.end(), 0);
  std::fill(this->VolumeError.begin(), this->VolumeError.end(), 0);
}

//------------------------------------------------------------------------------
void TreeInformation::PrintNode(vtkIncrementalOctreeNode* node)
{
  std::cout << "Node: " << node->GetID() << " buildings: ";
  vtkIdList* nodeBuildings = node->GetPointIdSet();
  if (nodeBuildings)
  {
    for (int i = 0; i < nodeBuildings->GetNumberOfIds(); ++i)
    {
      std::cout << nodeBuildings->GetId(i) << " ";
    }
  }
  std::cout << " children: ";
  if (!node->IsLeaf())
  {
    for (int i = 0; i < 8; ++i)
    {
      // buildings in child nodes contribute to the error in the parent
      vtkIncrementalOctreeNode* child = node->GetChild(i);
      std::cout << child->GetID() << " ";
    }
  }
  std::cout << std::endl;
  std::array<double, 6> bounds;
  node->GetBounds(&bounds[0]);
  std::cout << "Empty: " << this->EmptyNode[node->GetID()] << std::endl;
  // PrintBounds("Bounds", &bounds[0]);
  // PrintBounds("NodeBounds", &this->NodeBounds[node->GetID()][0]);
}

//------------------------------------------------------------------------------
void TreeInformation::Compute()
{
  this->PostOrderTraversal(&TreeInformation::Compute, this->Root, nullptr);
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTiles(bool mergeTilePolyData)
{
  this->PostOrderTraversal(&TreeInformation::SaveTile, this->Root, &mergeTilePolyData);
}

//------------------------------------------------------------------------------
void TreeInformation::PostOrderTraversal(
  void (TreeInformation::*Visit)(vtkIncrementalOctreeNode* node, void* aux),
  vtkIncrementalOctreeNode* node, void* aux)
{
  if (!node->IsLeaf())
  {
    for (int i = 0; i < 8; i++)
    {
      this->PostOrderTraversal(Visit, node->GetChild(i), aux);
    }
  }
  (this->*Visit)(node, aux);
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTile(vtkIncrementalOctreeNode* node, void* aux)
{
  bool mergeTilePolyData = *static_cast<bool*>(aux);
  if (node->IsLeaf() && !this->EmptyNode[node->GetID()])
  {
    std::ostringstream ostr;
    vtkIdList* pointIds = node->GetPointIds();
    // ostr << "Rendering buildings for node " << node->GetID() << ": ";
    vtkNew<vtkMultiBlockDataSet> tile;
    if (mergeTilePolyData)
    {
      vtkNew<vtkAppendPolyData> append;
      vtkNew<vtkMultiBlockDataSet> b;
      for (int i = 0; i < pointIds->GetNumberOfIds(); ++i)
      {
        int buildingId = pointIds->GetId(i);
        vtkCompositeDataSet* building = this->Buildings[buildingId];
        auto it = vtk::TakeSmartPointer(building->NewIterator());
        // for each poly data in the building
        for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
        {
          vtkPolyData* pd = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
          append->AddInputDataObject(pd);
        }
        append->Update();
        b->SetBlock(0, append->GetOutput());
        tile->SetBlock(0, b);
      }
    }
    else
    {
      for (int i = 0; i < pointIds->GetNumberOfIds(); ++i)
      {
        int buildingId = pointIds->GetId(i);
        // add all buildings to the tile
        tile->SetBlock(i, this->Buildings[buildingId]);
        // ostr << buildingId << " ";
      }
    }

    vtkLog(INFO, "Saving GLTF file for " << pointIds->GetNumberOfIds() << " buildings...");
    // vtkLog(INFO, << ostr.str());
    vtkNew<vtkGLTFWriter> writer;
    writer->SetInputData(tile);
    ostr.str("");
    ostr << this->OutputDir << "/" << node->GetID();
    vtkDirectory::MakeDirectory(ostr.str().c_str());
    ostr << "/" << node->GetID() << ".gltf";
    writer->SetFileName(ostr.str().c_str());
    writer->SetTextureBaseDirectory(this->TexturePath.c_str());
    writer->SetSaveTextures(this->SaveTextures);
    writer->SetSaveNormal(true);
    writer->Write();
  }
}

//------------------------------------------------------------------------------
double TreeInformation::ComputeTilesetGeometricError()
{
  double tilesetVolumeError;
  // buildings in child nodes contribute to the error in the parent
  tilesetVolumeError = this->VolumeError[this->Root->GetID()];
  vtkIdList* childBuildings = this->Root->GetPointIdSet();
  if (childBuildings)
  {
    for (int i = 0; i < childBuildings->GetNumberOfIds(); ++i)
    {
      double bb[6];
      this->Buildings[childBuildings->GetId(i)]->GetBounds(bb);
      double volume = (bb[1] - bb[0]) * (bb[3] - bb[2]) * (bb[5] - bb[4]);
      tilesetVolumeError += volume;
    }
  }
  return std::pow(tilesetVolumeError, 1.0 / 3);
}

//------------------------------------------------------------------------------
void TreeInformation::Compute(vtkIncrementalOctreeNode* node, void*)
{
  vtkIdList* nodeBuildings = node->GetPointIdSet();
  // compute the bounding box for the current node
  if (nodeBuildings)
  {
    this->NodeBounds[node->GetID()] = ComputeTightBoudingBox(this->Buildings, nodeBuildings);
    this->EmptyNode[node->GetID()] = false;
  }
  // propagate the node bounding box from the children.
  if (!node->IsLeaf())
  {
    for (int i = 0; i < 8; ++i)
    {
      // buildings in child nodes contribute to the error in the parent
      vtkIncrementalOctreeNode* child = node->GetChild(i);
      this->VolumeError[node->GetID()] += this->VolumeError[child->GetID()];
      vtkIdList* childBuildings = child->GetPointIdSet();
      if (childBuildings)
      {
        for (vtkIdType j = 0; j < childBuildings->GetNumberOfIds(); ++j)
        {
          double bb[6];
          this->Buildings[childBuildings->GetId(j)]->GetBounds(bb);
          double volume = (bb[1] - bb[0]) * (bb[3] - bb[2]) * (bb[5] - bb[4]);
          this->VolumeError[node->GetID()] += volume;
        }
      }
      if (!this->EmptyNode[child->GetID()])
      {
        this->NodeBounds[node->GetID()] =
          ExpandBounds(&this->NodeBounds[node->GetID()][0], &this->NodeBounds[child->GetID()][0]);
        this->EmptyNode[node->GetID()] = false;
      }
    }
  }
  this->GeometricError[node->GetID()] = std::pow(this->VolumeError[node->GetID()], 1.0 / 3);
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTileset(const std::string& output)
{
  this->SaveTileset(this->Root, output);
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTileset(vtkIncrementalOctreeNode* root, const std::string& output)
{
  json v;
  this->RootJson["asset"]["version"] = "1.0";
  if (this->ContentType != vtkCesium3DTilesWriter::B3DM)
  {
    std::string content_gltf = "3DTILES_content_gltf";
    std::string mesh_gpu_instancing = "EXT_mesh_gpu_instancing";
    std::string extensionsUsed = "extensionsUsed";
    std::string extensionsRequired = "extensionsRequired";
    v = { content_gltf };
    this->RootJson[extensionsUsed] = v;
    this->RootJson[extensionsRequired] = v;
    v = { mesh_gpu_instancing };
    this->RootJson["extensions"][content_gltf][extensionsUsed] = v;
    this->RootJson["extensions"][content_gltf][extensionsRequired] = v;
  }
  this->RootJson["geometricError"] = this->ComputeTilesetGeometricError();
  this->RootJson["root"] = this->GenerateTileJson(root);
  vtksys::ofstream file(output.c_str());
  if (!file)
  {
    vtkLog(ERROR, "Cannot open " << output << " for writing");
    return;
  }
  file << std::setw(4) << this->RootJson << std::endl;
}

//------------------------------------------------------------------------------
json TreeInformation::GenerateTileJson(vtkIncrementalOctreeNode* node)
{
  json tree;
  json v;
  std::array<double, 6> nodeBounds = this->NodeBounds[node->GetID()];
  std::array<double, 6> lonLatRadiansHeight = ToLonLatRadiansHeight(this->CRS, nodeBounds);
  std::ostringstream ostr;
  // std::cout << "lonLatRadiansHeight: " << (lonLatRadiansHeight[0] * 180.0) / vtkMath::Pi() << " "
  //           << (lonLatRadiansHeight[1] * 180.0) / vtkMath::Pi() << " "
  //           << (lonLatRadiansHeight[2] * 180.0) / vtkMath::Pi() << " "
  //           << (lonLatRadiansHeight[3] * 180.0) / vtkMath::Pi() << " " << lonLatRadiansHeight[4]
  //           << " "
  //           << lonLatRadiansHeight[5] << " " << std::endl;
  for (int i = 0; i < 6; ++i)
  {
    v[i] = lonLatRadiansHeight[i];
  }
  tree["boundingVolume"]["region"] = v;
  tree["geometricError"] = this->GeometricError[node->GetID()];
  if (node == this->Root)
  {
    tree["refine"] = "REPLACE";
    std::array<double, 16> t = { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 1.0 };
    v.clear();
    for (int i = 0; i < 16; ++i)
    {
      v[i] = t[i];
    }
    tree["transform"] = v;
  }
  // generate json for the node
  if (!node->IsLeaf())
  {
    v.clear();
    for (int i = 0, j = 0; i < 8; i++)
    {
      if (!this->EmptyNode[node->GetChild(i)->GetID()])
      {
        v[j++] = this->GenerateTileJson(node->GetChild(i));
      }
      tree["children"] = v;
    }
  }
  else
  {
    if (!this->EmptyNode[node->GetID()])
    {

      PJ* P;
      P = proj_create_crs_to_crs(PJ_DEFAULT_CTX, this->CRS, "+proj=cart", nullptr);
      if (P == nullptr)
      {
        vtkLog(ERROR, "proj_create_crs_to_crs failed: " << proj_errno_string(proj_errno(nullptr)));
        return tree;
      }
      /* For that particular use case, this is not needed. */
      /* proj_normalize_for_visualization() ensures that the coordinate */
      /* order expected and returned by proj_trans() will be longitude, */
      /* latitude for geographic CRS, and easting, northing for projected */
      /* CRS. If instead of using PROJ strings as above, "EPSG:XXXX" codes */
      /* had been used, this might had been necessary. */
      PJ* P_for_GIS = proj_normalize_for_visualization(PJ_DEFAULT_CTX, P);
      if (P_for_GIS == nullptr)
      {
        proj_destroy(P);
        vtkLog(ERROR,
          "proj_normalize_for_visualization failed: " << proj_errno_string(proj_errno(nullptr)));
        return tree;
      }
      proj_destroy(P);
      P = P_for_GIS;

      // transform points to Cartesian coordinates
      vtkIdList* pointIds = node->GetPointIds();
      // for each building
      for (int i = 0; i < pointIds->GetNumberOfIds(); ++i)
      {
        int buildingId = pointIds->GetId(i);
        vtkSmartPointer<vtkCompositeDataSet> building = this->Buildings[buildingId];
        auto it = vtk::TakeSmartPointer(building->NewIterator());
        // for each poly data in the building
        for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
        {
          vtkPolyData* pd = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
          vtkDataArray* points = pd->GetPoints()->GetData();
          vtkNew<vtkDoubleArray> newPoints;
          vtkDoubleArray* da = vtkArrayDownCast<vtkDoubleArray>(points);
          vtkFloatArray* fa = vtkArrayDownCast<vtkFloatArray>(points);
          bool conversion = false;
          if (!da)
          {
            if (fa)
            {
              vtkLog(WARNING, "Converting float to double points.");
              newPoints->DeepCopy(fa);
              da = newPoints;
              conversion = true;
            }
            else
            {
              vtkLog(ERROR, "Points are not float or double.");
              break;
            }
          }
          double* d = da->GetPointer(0);
          int n = da->GetNumberOfTuples();
          proj_trans_generic(P, PJ_FWD, d, sizeof(d[0]) * 3, n, d + 1, sizeof(d[0]) * 3, n, d + 2,
            sizeof(d[0]) * 3, n, nullptr, 0, 0);
          if (conversion)
          {
            pd->GetPoints()->SetData(newPoints);
          }
        }
      }
      proj_destroy(P);

      ostr.str("");
      ostr << node->GetID() << "/" << node->GetID() << ContentTypeExtension[this->ContentType];
      tree["content"]["uri"] = ostr.str();
    }
  }
  return tree;
}

//------------------------------------------------------------------------------
bool TreeInformation::GetNodeBounds(int i, double* bounds)
{
  if (this->EmptyNode[i])
  {
    return false;
  }
  std::copy(this->NodeBounds[i].begin(), this->NodeBounds[i].end(), bounds);
  return true;
}

//------------------------------------------------------------------------------
bool TreeInformation::GetNodeBounds(void* data, vtkIncrementalOctreeNode* node, double* bounds)
{
  return static_cast<TreeInformation*>(data)->GetNodeBounds(node->GetID(), bounds);
}

//------------------------------------------------------------------------------
void TreeInformation::AddGeometricError(vtkPolyData* poly)
{
  vtkIntArray* indexArray = vtkIntArray::SafeDownCast(poly->GetCellData()->GetArray("Index"));
  vtkNew<vtkDoubleArray> error;
  error->SetName("Error");
  error->SetNumberOfTuples(indexArray->GetNumberOfTuples());
  for (int i = 0; i < indexArray->GetNumberOfTuples(); ++i)
  {
    int index = indexArray->GetValue(i);
    error->SetValue(i, this->GeometricError[index]);
  }
  poly->GetCellData()->AddArray(error);
}

//------------------------------------------------------------------------------
void TreeInformation::PrintBounds(const char* name, const double* bounds)
{
  std::cout << name << ": [" << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
            << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << "]"
            << " dims: [" << (bounds[1] - bounds[0]) << ", " << (bounds[3] - bounds[2]) << ", "
            << (bounds[5] - bounds[4]) << "]" << std::endl;
}

//------------------------------------------------------------------------------
std::array<double, 6> TreeInformation::ExpandBounds(double* first, double* second)
{
  return { std::min(first[0], second[0]), std::max(first[1], second[1]),
    std::min(first[2], second[2]), std::max(first[3], second[3]), std::min(first[4], second[4]),
    std::max(first[5], second[5]) };
}
