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

#include "vtkGLTFWriter.h"
#include "vtkMultiBlockDataSet.h"
#include <vtkActor.h>
#include <vtkCellData.h>
#include <vtkCompositeDataSet.h>
#include <vtkDirectory.h>
#include <vtkDoubleArray.h>
#include <vtkIncrementalOctreeNode.h>
#include <vtkLogger.h>
#include <vtkMath.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

#include <vtk_libproj.h>

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
 * TODO: use srsName which takes precedence to UTM.
 */
std::array<double, 6> ToLonLatRadiansHeight(const char* vtkNotUsed(srsName), int utmZone,
  char utmHemisphere, const std::array<double, 6>& bb, const std::array<double, 3>& offset)
{
  std::array<double, 6> lonlatheight;
  lonlatheight[4] = offset[2] + bb[4];
  lonlatheight[5] = offset[2] + bb[5];
  std::ostringstream ostr;
  ostr << "+proj=utm +zone=" << utmZone << (utmHemisphere == 'S' ? "+south" : "")
       << " +datum=WGS84";
  PJ* P;
  P =
    proj_create_crs_to_crs(PJ_DEFAULT_CTX, ostr.str().c_str(), "+proj=longlat +ellps=WGS84", NULL);
  if (P == 0)
  {
    vtkLog(ERROR, "proj_create_crs_to_crs failed: " << proj_errno_string(proj_errno(0)));
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
    if (0 == P_for_GIS)
    {
      proj_destroy(P);
      vtkLog(
        ERROR, "proj_normalize_for_visualization failed: " << proj_errno_string(proj_errno(0)));
      return lonlatheight;
    }
    proj_destroy(P);
    P = P_for_GIS;
  }
  PJ_COORD c, c_out;
  for (int i = 0; i < 2; ++i)
  {
    c.xy.x = offset[0] + bb[i];
    c.xy.y = offset[1] + bb[i + 2];
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

//------------------------------------------------------------------------------
// TODO: use srsName which takes precedence to UTM.
vtkSmartPointer<vtkMatrix4x4> ComputeTransformCartesian(
  const std::array<double, 3>& originLonLatDegreesHeight)
{
  auto m = vtkSmartPointer<vtkMatrix4x4>::New();
  m->Identity();
  vtkNew<vtkTransform> t;
  std::ostringstream ostr;
  PJ* P;
  PJ_COORD c, c_out;

  // std::cout << "originDegrees: "
  //           << originDegrees[0] << ", " << originDegrees[1] << std::endl;

  // transform from geodetic lon, lat, height to geocentric coordinates (proj >= 5.0)
  P = proj_create_crs_to_crs(
    PJ_DEFAULT_CTX, "+proj=longlat +ellps=WGS84", "+proj=cart +ellps=WGS84", NULL);
  if (P == 0)
  {
    vtkLog(ERROR, "proj_create_crs_to_crs failed:" << proj_errno_string(proj_errno(0)));
    return m;
  }
  {
    /* For that particular use case, this is not needed. */
    /* proj_normalize_for_visualization() ensures that the coordinate */
    /* order expected and returned by proj_trans() will be longitude, */
    /* latitude for geographic CRS, and easting, northing for projected */
    /* CRS. If instead of using PROJ strings as above, "EPSG:XXXX" codes */
    /* had been used, this might had been necessary. */
    PJ* P_for_GIS = proj_normalize_for_visualization(PJ_DEFAULT_CTX, P);
    if (0 == P_for_GIS)
    {
      proj_destroy(P);
      vtkLog(
        ERROR, "proj_normalize_for_visualization failed: " << proj_errno_string(proj_errno(0)));
      return m;
    }
    proj_destroy(P);
    P = P_for_GIS;
  }
  c.lpz.lam = originLonLatDegreesHeight[0];
  c.lpz.phi = originLonLatDegreesHeight[1];
  c.lpz.z = originLonLatDegreesHeight[2];
  c_out = proj_trans(P, PJ_FWD, c);
  proj_destroy(P);

  double ecefOrigin[3];
  ecefOrigin[0] = c_out.xyz.x;
  ecefOrigin[1] = c_out.xyz.y;
  ecefOrigin[2] = c_out.xyz.z;
  // std::cout << "ecefOrigin: "
  //           << ecefOrigin[0] << " "
  //           << ecefOrigin[1] << " "
  //           << ecefOrigin[2]
  //           << std::endl;
  t->Identity();
  t->Translate(ecefOrigin);
  t->RotateZ(90.0 + originLonLatDegreesHeight[0]);
  t->RotateX(-originLonLatDegreesHeight[1]);

  t->GetTranspose(m);
  return m;
}

}

//------------------------------------------------------------------------------
TreeInformation::TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes,
  const std::vector<vtkSmartPointer<vtkCompositeDataSet>>& buildings,
  const std::array<double, 3>& offset, const std::string& output, const std::string& texturePath,
  bool saveTextures, const char* srsName, int utmZone, char utmHemisphere)
  :

  Root(root)
  , Buildings(buildings)
  , Offset(offset)
  , OutputDir(output)
  , TexturePath(texturePath)
  , SaveTextures(saveTextures)
  , SrsName(srsName)
  , UTMZone(utmZone)
  , UTMHemisphere(utmHemisphere)
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
  this->PostOrderTraversal(&TreeInformation::Compute, this->Root);
}

//------------------------------------------------------------------------------
void TreeInformation::SaveGLTF()
{
  this->PostOrderTraversal(&TreeInformation::SaveGLTF, this->Root);
}

//------------------------------------------------------------------------------
void TreeInformation::PostOrderTraversal(
  void (TreeInformation::*Visit)(vtkIncrementalOctreeNode* node), vtkIncrementalOctreeNode* node)
{
  if (!node->IsLeaf())
  {
    for (int i = 0; i < 8; i++)
    {
      this->PostOrderTraversal(Visit, node->GetChild(i));
    }
  }
  (this->*Visit)(node);
}

//------------------------------------------------------------------------------
void TreeInformation::SaveGLTF(vtkIncrementalOctreeNode* node)
{
  if (node->IsLeaf() && !this->EmptyNode[node->GetID()])
  {
    std::ostringstream ostr;
    vtkIdList* pointIds = node->GetPointIds();
    // ostr << "Rendering buildings for node " << node->GetID() << ": ";
    vtkNew<vtkMultiBlockDataSet> tile;
    for (int i = 0; i < pointIds->GetNumberOfIds(); ++i)
    {
      int buildingId = pointIds->GetId(i);
      // add all buildings to the tile
      tile->SetBlock(i, this->Buildings[buildingId]);
      // ostr << buildingId << " ";
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
    writer->SetSaveNormal(!this->SaveTextures);
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
void TreeInformation::Compute(vtkIncrementalOctreeNode* node)
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
void TreeInformation::Generate3DTiles(const std::string& output)
{
  this->GenerateCesium3DTiles(this->Root, output);
}

//------------------------------------------------------------------------------
void TreeInformation::GenerateCesium3DTiles(
  vtkIncrementalOctreeNode* root, const std::string& output)
{

  this->RootJson["asset"]["version"] = "1.0";
  this->RootJson["geometricError"] = this->ComputeTilesetGeometricError();
  this->RootJson["root"] = this->GenerateCesium3DTiles(root);
  std::ofstream file(output);
  if (!file)
  {
    vtkLog(ERROR, "Cannot open " << output << " for writing");
    return;
  }
  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "  ";
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  writer->write(this->RootJson, &file);
}

//------------------------------------------------------------------------------
Json::Value TreeInformation::GenerateCesium3DTiles(vtkIncrementalOctreeNode* node)
{
  Json::Value tree;
  Json::Value v;
  std::array<double, 6> lonLatRadiansHeight = ToLonLatRadiansHeight(this->SrsName, this->UTMZone,
    this->UTMHemisphere, this->NodeBounds[node->GetID()], this->Offset);
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
    ostr << std::string("NodeBounds(") << node->GetID() << ")";
    PrintBounds(ostr.str(), &this->NodeBounds[node->GetID()][0]);
    tree["refine"] = "REPLACE";
    std::array<double, 3> originLonLatDegreesHeight = { { vtkMath::DegreesFromRadians(
                                                            lonLatRadiansHeight[0]),
      vtkMath::DegreesFromRadians(lonLatRadiansHeight[1]), lonLatRadiansHeight[4] } };
    vtkSmartPointer<vtkMatrix4x4> m = ::ComputeTransformCartesian(originLonLatDegreesHeight);
    double* t = m->GetData();
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
        v[j++] = this->GenerateCesium3DTiles(node->GetChild(i));
      }
      tree["children"] = v;
    }
  }
  else
  {
    if (!this->EmptyNode[node->GetID()])
    {
      ostr.str("");
      ostr << node->GetID() << "/" << node->GetID() << ".b3dm";
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
