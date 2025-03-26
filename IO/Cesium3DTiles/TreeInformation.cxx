// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "TreeInformation.h"

#include "vtk_libproj.h"

#include <limits>
#include <sstream>

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkCesium3DTilesWriter.h"
#include "vtkCesiumPointCloudWriter.h"
#include "vtkCompositeDataSet.h"
#include "vtkDirectory.h"
#include "vtkDoubleArray.h"
#include "vtkExtractSelection.h"
#include "vtkFloatArray.h"
#include "vtkGLTFWriter.h"
#include "vtkGeometryFilter.h"
#include "vtkImageAppend.h"
#include "vtkImageData.h"
#include "vtkImageIterator.h"
#include "vtkIncrementalOctreeNode.h"
#include "vtkJPEGReader.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTransform.h"

#include "vtksys/SystemTools.hxx"
#include <vtksys/FStream.hxx>

using RegionType = std::array<int, 6>;

namespace
{
VTK_ABI_NAMESPACE_BEGIN
constexpr double MIN_ERROR = 20;

//------------------------------------------------------------------------------
/**
 * Compute the tight bounding box around all buildings in a tile.
 * 'tileBuildings', stores all buildings in a tile as indexes into 'buildings'
 * which stores all buildings.
 */
std::array<double, 6> ComputeTightBBBuildings(
  const std::vector<vtkSmartPointer<vtkCompositeDataSet>>* buildings, vtkIdList* tileBuildings)
{
  std::array<double, 6> wholeBB = { std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest() };
  for (int i = 0; i < tileBuildings->GetNumberOfIds(); ++i)
  {
    double bb[6];
    (*buildings)[tileBuildings->GetId(i)]->GetBounds(bb);
    wholeBB = TreeInformation::ExpandBounds(wholeBB.data(), bb);
  }
  return wholeBB;
}

std::array<double, 6> ComputeTightBBMesh(
  const vtkSmartPointer<vtkPolyData> mesh, vtkIdList* tileCells)
{
  std::array<double, 6> wholeBB = { std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest() };
  for (int i = 0; i < tileCells->GetNumberOfIds(); ++i)
  {
    double bb[6];
    mesh->GetCell(tileCells->GetId(i))->GetBounds(bb);
    wholeBB = TreeInformation::ExpandBounds(wholeBB.data(), bb);
  }
  return wholeBB;
}

std::array<double, 6> ComputeTightBBPoints(
  const vtkSmartPointer<vtkPointSet> points, vtkIdList* tilePoints)
{
  std::array<double, 6> wholeBB = { std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest() };
  for (int i = 0; i < tilePoints->GetNumberOfIds(); ++i)
  {
    std::array<double, 6> bb;
    double point[3];
    points->GetPoint(tilePoints->GetId(i), point);
    bb = { { point[0], point[0], point[1], point[1], point[2], point[2] } };
    wholeBB = TreeInformation::ExpandBounds(wholeBB.data(), bb.data());
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
  P = proj_create_crs_to_crs(PJ_DEFAULT_CTX, crs, "+proj=longlat +ellps=WGS84 lon_0=0", nullptr);
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
  PJ_COORD c = { { 0, 0, 0, 0 } }, c_out;
  for (size_t i = 0; i < 2; ++i)
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

//------------------------------------------------------------------------------
void SetField(vtkDataObject* obj, const char* name, const std::vector<std::string>& values)
{
  vtkFieldData* fd = obj->GetFieldData();
  if (!fd)
  {
    vtkNew<vtkFieldData> newfd;
    obj->SetFieldData(newfd);
    fd = newfd;
  }
  vtkNew<vtkStringArray> sa;
  sa->SetNumberOfTuples(values.size());
  for (size_t i = 0; i < values.size(); ++i)
  {
    const std::string& value = values[i];
    sa->SetValue(i, value);
  }
  sa->SetName(name);
  fd->AddArray(sa);
}

vtkSmartPointer<vtkImageReader2> SetupTextureReader(const std::string& texturePath)
{
  std::string ext = vtksys::SystemTools::GetFilenameLastExtension(texturePath);
  vtkSmartPointer<vtkImageReader2> reader;
  if (ext == ".png")
  {
    reader = vtkSmartPointer<vtkPNGReader>::New();
    if (!reader->CanReadFile(texturePath.c_str()))
    {
      vtkLog(ERROR, "Invalid texture file: " << texturePath);
      return nullptr;
    }
  }
  else if (ext == ".jpg")
  {
    reader = vtkSmartPointer<vtkJPEGReader>::New();
    if (!reader->CanReadFile(texturePath.c_str()))
    {
      vtkLog(ERROR, "Invalid texture file: " << texturePath);
      return nullptr;
    }
  }
  else
  {
    vtkLog(ERROR, "Invalid type for texture file: " << texturePath);
    return nullptr;
  }
  reader->SetFileName(texturePath.c_str());
  return reader;
}

struct SaveTileMeshData
{
  SaveTileMeshData(vtkSelectionNode::SelectionField selectionField,
    const std::vector<vtkSmartPointer<vtkImageData>>& textureImages)
    : SelectionField(selectionField)
    , TextureImages(textureImages)
  {
  }
  vtkSelectionNode::SelectionField SelectionField;
  std::vector<vtkSmartPointer<vtkImageData>> TextureImages;
};

struct CopyScalarsWorker
{
  template <typename Array>
  void operator()(Array* vtkNotUsed(srcScalar), vtkImageData* datasetImage,
    RegionType& datasetRegion, vtkImageData* tileImage, RegionType& tileRegion) const
  {
    using Type = vtk::GetAPIType<Array>;

    vtkImageIterator<Type> itDataset(datasetImage, datasetRegion.data());
    vtkImageIterator<Type> itTile(tileImage, tileRegion.data());
    while (!itDataset.IsAtEnd())
    {
      Type* datasetPtr = itDataset.BeginSpan();
      Type* datasetPtrEnd = itDataset.EndSpan();
      Type* tilePtr = itTile.BeginSpan();
      while (datasetPtr != datasetPtrEnd)
      {
        *tilePtr = *datasetPtr;
        ++datasetPtr;
        ++tilePtr;
      }
      itDataset.NextSpan();
      itTile.NextSpan();
    }
  }
};

struct InitializeWorker
{
  template <typename Array>
  void operator()(Array* outArray) const
  {
    auto outRange = vtk::DataArrayTupleRange(outArray);
    const vtk::TupleIdType numTuples = outRange.size();
    const vtk::ComponentIdType numComps = outRange.GetTupleSize();
    for (vtk::TupleIdType tupleId = 0; tupleId < numTuples; ++tupleId)
    {
      auto outTuple = outRange[tupleId];

      for (vtk::ComponentIdType compId = 0; compId < numComps; ++compId)
      {
        outTuple[compId] = compId ? 0 : 255;
      }
    }
  }
};

std::array<std::string, 3> BuildingsContentTypeExtension = { ".b3dm", ".glb", ".gltf" };
std::array<std::string, 3> PointsContentTypeExtension = { ".pnts", ".glb", ".gltf" };

vtkSmartPointer<vtkImageData> GetTexture(
  const std::string& textureBaseDirectory, const std::string& textureFileName)
{
  std::string texturePath =
    textureBaseDirectory.empty() ? textureFileName : (textureBaseDirectory + "/" + textureFileName);
  auto textureReader = SetupTextureReader(texturePath);
  vtkSmartPointer<vtkImageData> textureImage;
  if (textureReader)
  {
    textureReader->Update();
    textureImage = vtkImageData::SafeDownCast(textureReader->GetOutput());
  }
  return textureImage;
}

std::vector<vtkSmartPointer<vtkImageData>> GetTileTextures(const std::string& textureBaseDirectory,
  const std::vector<std::vector<std::string>>& tileTextureFileNames, size_t textureIndex)
{
  std::vector<vtkSmartPointer<vtkImageData>> tileTextures(tileTextureFileNames.size());
  for (size_t i = 0; i < tileTextureFileNames.size(); ++i)
  {
    tileTextures[i] = GetTexture(textureBaseDirectory, tileTextureFileNames[i][textureIndex]);
  }
  return tileTextures;
}

void TranslateTCoords(std::vector<vtkSmartPointer<vtkImageData>> tileTextures,
  const std::vector<std::array<int, 2>>& textureOrigin, int tileDims[3],
  std::vector<vtkDataArray*> tileTCoords)
{
  int dims[3];
  for (size_t i = 0; i < tileTextures.size(); ++i)
  {
    vtkDataArray* tcoordsArray = tileTCoords[i];
    if (tcoordsArray)
    {
      tileTextures[i]->GetDimensions(dims);
      for (int j = 0; j < tcoordsArray->GetNumberOfTuples(); ++j)
      {
        double tcoords[2];
        tcoordsArray->GetTuple(j, tcoords);
        double newTCoords[2];
        for (int k = 0; k < 2; ++k)
        {
          // account for GL_REPEATE textures
          while (tcoords[k] < 0)
          {
            tcoords[k] += 1;
          }
          while (tcoords[k] > 1)
          {
            tcoords[k] -= 1;
          }
          // compute the new texture
          newTCoords[k] = (tcoords[k] * dims[k] + textureOrigin[i][k]) / tileDims[k];
        }
        tcoordsArray->SetTuple(j, newTCoords);
      }
    }
  }
}

vtkSmartPointer<vtkImageData> MergeTextures(std::vector<vtkSmartPointer<vtkImageData>> tileTextures,
  std::vector<size_t> textureId, // sorted decreasing by height
  size_t mergedTextureWidth, std::vector<std::array<int, 2>>& textureOrigin)
{
  if (tileTextures.size() != textureId.size() || tileTextures.size() != textureOrigin.size())
  {
    vtkLog(ERROR,
      "Error texture sizes: " << tileTextures.size() << ", " << textureId.size() << ", "
                              << textureOrigin.size());
    return nullptr;
  }
  vtkNew<vtkImageAppend> append;
  append->PreserveExtentsOn();
  std::array<int, 2> currentOrigin = { { 0, 0 } };
  int row = 0, prevRow = -1;
  size_t column = 0;
  int dims[3];
  int extent[6];
  // currentHeight is set every time the row changes. We set the initial prevRow
  // so that it shows a row change so currentHeight gets initialized.
  int currentHeight = 0;
  for (size_t i = 0; i < tileTextures.size(); ++i)
  {
    // use currentOrigin to translate the extent of texture
    tileTextures[textureId[i]]->GetExtent(extent);
    extent[0] += currentOrigin[0];
    extent[1] += currentOrigin[0];
    extent[2] += currentOrigin[1];
    extent[3] += currentOrigin[1];
    tileTextures[textureId[i]]->SetExtent(extent);
    append->AddInputData(tileTextures[textureId[i]]);
    textureOrigin[textureId[i]] = currentOrigin;

    // compute the currentOrigin for the next texture
    tileTextures[textureId[i]]->GetDimensions(dims);
    if (prevRow < row)
    {
      currentHeight = dims[1];
      prevRow = row;
    }
    if (column < mergedTextureWidth - 1)
    {
      ++column;
      currentOrigin[0] += dims[0];
    }
    else
    {
      ++row;
      column = 0;
      currentOrigin[0] = 0;
      currentOrigin[1] += currentHeight;
    }
  }
  append->Update();
  vtkImageData* tileImage = vtkImageData::SafeDownCast(append->GetOutputDataObject(0));
  return tileImage;
}

struct MergePolyDataInfo
{
  bool MergePolyData;
  size_t MergedTextureWidth;
};
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
TreeInformation::TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes,
  const std::vector<vtkSmartPointer<vtkCompositeDataSet>>* buildings,
  const std::string& textureBaseDirectory, const std::string& propertyTextureFile,
  bool saveTextures, bool contentGLTF, bool contentGLTFSaveGLB, const char* crs,
  const std::string& output)
  : InputType(vtkCesium3DTilesWriter::Buildings)
  , Root(root)
  , Buildings(buildings)
  , Points(nullptr)
  , Mesh(nullptr)
  , OutputDir(output)
  , TextureBaseDirectory(textureBaseDirectory)
  , PropertyTextureFile(propertyTextureFile)
  , SaveTextures(saveTextures)
  , ContentGLTF(contentGLTF)
  , ContentGLTFSaveGLB(contentGLTFSaveGLB)
  , CRS(crs)
  , NodeTightBounds(numberOfNodes)
  , EmptyNode(numberOfNodes)
  , GeometricError(numberOfNodes)
{
  Initialize();
}

//------------------------------------------------------------------------------
TreeInformation::TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes,
  vtkPointSet* points, bool contentGLTF, bool contentGLTFSaveGLB, const char* crs,
  const std::string& output)
  : InputType(vtkCesium3DTilesWriter::Points)
  , Root(root)
  , Buildings(nullptr)
  , Points(points)
  , Mesh(vtkPolyData::SafeDownCast(points))
  , OutputDir(output)
  , SaveTextures(false)
  , ContentGLTF(contentGLTF)
  , ContentGLTFSaveGLB(contentGLTFSaveGLB)
  , CRS(crs)
  , NodeTightBounds(numberOfNodes)
  , EmptyNode(numberOfNodes)
  , GeometricError(numberOfNodes)
{
  Initialize();
}

//------------------------------------------------------------------------------
TreeInformation::TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes,
  vtkPolyData* mesh, const std::string& textureBaseDirectory,
  const std::string& propertyTextureFile, bool saveTextures, bool contentGLTF,
  bool contentGLTFSaveGLB, const char* crs, const std::string& output)
  : InputType(vtkCesium3DTilesWriter::Mesh)
  , Root(root)
  , Buildings(nullptr)
  , Points(nullptr)
  , Mesh(mesh)
  , OutputDir(output)
  , TextureBaseDirectory(textureBaseDirectory)
  , PropertyTextureFile(propertyTextureFile)
  , SaveTextures(saveTextures)
  , ContentGLTF(contentGLTF)
  , ContentGLTFSaveGLB(contentGLTFSaveGLB)
  , CRS(crs)
  , NodeTightBounds(numberOfNodes)
  , EmptyNode(numberOfNodes)
  , GeometricError(numberOfNodes)
{
  Initialize();
}

//------------------------------------------------------------------------------
void TreeInformation::Initialize()
{
  std::array<double, 6> a = { std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest() };
  std::fill(this->NodeTightBounds.begin(), this->NodeTightBounds.end(), a);
  std::fill(this->EmptyNode.begin(), this->EmptyNode.end(), true);
  std::fill(this->GeometricError.begin(), this->GeometricError.end(), 0);
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
  node->GetBounds(bounds.data());
  std::cout << "Empty: " << this->EmptyNode[node->GetID()] << std::endl;
  // PrintBounds("Bounds", bounds.data());
  // PrintBounds("NodeTightBounds", &this->NodeTightBounds[node->GetID()][0]);
}

//------------------------------------------------------------------------------
void TreeInformation::Compute()
{
  this->PostOrderTraversal(&TreeInformation::VisitCompute, this->Root, nullptr);
  if (this->InputType == vtkCesium3DTilesWriter::Mesh)
  {
    double length2 = this->GetRootLength2();
    double lengthAux = 2 * std::sqrt(length2);
    this->PreOrderTraversal(&TreeInformation::VisitComputeGeometricError, this->Root, &lengthAux);
  }
  else
  {
    this->PostOrderTraversal(&TreeInformation::VisitComputeGeometricError, this->Root, nullptr);
  }
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTilesBuildings(bool mergeTilePolyData, size_t mergedTextureWidth)
{
  MergePolyDataInfo info{ mergeTilePolyData, mergedTextureWidth };
  this->PostOrderTraversal(&TreeInformation::SaveTileBuildings, this->Root, &info);
}

void TreeInformation::WriteTileTexture(
  vtkIncrementalOctreeNode* node, const std::string& fileName, vtkImageData* tileImage)
{
  std::string dirPath = this->OutputDir + "/" + std::to_string(node->GetID());
  vtkDirectory::MakeDirectory(dirPath.c_str());
  std::string filePath = dirPath + "/" + fileName;
  vtkNew<vtkPNGWriter> writer;
  writer->SetFileName(filePath.c_str());
  writer->SetInputDataObject(tileImage);
  writer->Write();
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTilesMesh()
{
  std::vector<std::string> textureFileNames =
    vtkGLTFWriter::GetFieldAsStringVector(this->Mesh, "texture_uri");
  vtkLog(INFO, "Input has " << textureFileNames.size() << " textures");

  std::vector<vtkSmartPointer<vtkImageData>> textureImages(textureFileNames.size());
  for (size_t i = 0; i < textureFileNames.size(); ++i)
  {
    auto textureFileName = textureFileNames[i];
    textureImages[i] = GetTexture(this->TextureBaseDirectory, textureFileName);
  }
  SaveTileMeshData aux(vtkSelectionNode::CELL, textureImages);
  this->PostOrderTraversal(&TreeInformation::SaveTileMesh, this->Root, &aux);
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTilesPoints()
{
  int selectionField = vtkSelectionNode::POINT;
  this->PostOrderTraversal(&TreeInformation::SaveTilePoints, this->Root, &selectionField);
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
void TreeInformation::PreOrderTraversal(
  void (TreeInformation::*Visit)(vtkIncrementalOctreeNode* node, void* aux),
  vtkIncrementalOctreeNode* node, void* aux)
{
  (this->*Visit)(node, aux);
  if (!node->IsLeaf())
  {
    for (int i = 0; i < 8; i++)
    {
      this->PreOrderTraversal(Visit, node->GetChild(i), aux);
    }
  }
}

bool TreeInformation::ForEachBuilding(
  vtkIncrementalOctreeNode* node, std::function<bool(vtkPolyData* pd)> Execute)
{
  vtkIdList* pointIds = node->GetPointIds();
  bool continueForEach = true;
  for (int i = 0; i < pointIds->GetNumberOfIds() && continueForEach; ++i)
  {
    int buildingId = pointIds->GetId(i);
    vtkCompositeDataSet* building = (*this->Buildings)[buildingId];
    auto it = vtk::TakeSmartPointer(building->NewIterator());
    // for each poly data in the building
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
      vtkPolyData* pd = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
      continueForEach = Execute(pd);
    }
  }
  return continueForEach;
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTileBuildings(vtkIncrementalOctreeNode* node, void* aux)
{
  MergePolyDataInfo info = *static_cast<MergePolyDataInfo*>(aux);
  if (node->IsLeaf() && !this->EmptyNode[node->GetID()])
  {
    std::ostringstream ostr;
    vtkIdList* pointIds = node->GetPointIds();
    // ostr << "Rendering buildings for node " << node->GetID() << ": ";
    vtkNew<vtkMultiBlockDataSet> tile;
    std::string textureBaseDirectory;
    if (info.MergePolyData)
    {
      // each polydata has a vector of textures (for instance 7).
      // we merge textures for all polydata for index 0, 1, ..., 6. We get 7 merged
      // textures.
      std::vector<vtkPolyData*> meshes;
      std::vector<std::vector<std::string>> meshTextureFileNames;
      std::vector<vtkPolyData*> meshesWithTexture;
      // each polydata has a tcoord array
      std::vector<vtkDataArray*> meshTCoords;
      size_t numberOfTextures = 0;
      // accumulate all texture file names and tcoords
      std::function<bool(vtkPolyData*)> accumulateNamesAndTCoords =
        [&meshes, &numberOfTextures, &meshTextureFileNames, &meshTCoords, &meshesWithTexture](
          vtkPolyData* pd) {
          auto pdTextureFileNames = vtkGLTFWriter::GetFieldAsStringVector(pd, "texture_uri");
          if (pdTextureFileNames.empty())
          {
            meshes.push_back(pd);
          }
          else
          {
            if (numberOfTextures && numberOfTextures != pdTextureFileNames.size())
            {
              vtkLog(ERROR,
                "Different polydata in the tile have different "
                "number of textures "
                  << pdTextureFileNames.size() << " expecting " << numberOfTextures);
              // disable texture merging
              numberOfTextures = 0;
              return false;
            }
            numberOfTextures = pdTextureFileNames.size();
            meshesWithTexture.push_back(pd);
            meshTextureFileNames.push_back(pdTextureFileNames);
            meshTCoords.push_back(pd->GetPointData()->GetTCoords());
          }
          return true;
        };
      this->ForEachBuilding(node, accumulateNamesAndTCoords);

      // how many polydata textures along one side of the merged texture
      size_t mergedTextureWidth = std::ceil(std::sqrt(meshesWithTexture.size()));
      if (info.MergedTextureWidth < mergedTextureWidth)
      {
        mergedTextureWidth = info.MergedTextureWidth;
      }
      // merge textures and change the tcoords arrays
      // all textures use the same tcoords array
      // if there is only one texture, there is nothing to merge.
      std::vector<std::string> mergedFileNames;
      if (meshTextureFileNames.size() > 1 && this->SaveTextures)
      {
        mergedFileNames.resize(meshTextureFileNames[0].size());
        std::vector<std::array<int, 2>> textureOrigin(meshTextureFileNames.size());
        for (size_t i = 0; i < numberOfTextures; ++i)
        {
          // load all textures we need to merge
          std::vector<vtkSmartPointer<vtkImageData>> tileTextures =
            GetTileTextures(this->TextureBaseDirectory, meshTextureFileNames, i);
          // permutation of indexes to tileTextures
          // sorted on decreasing height of textures
          std::vector<size_t> textureIds(tileTextures.size());
          std::iota(textureIds.begin(), textureIds.end(), 0);
          std::sort(
            textureIds.begin(), textureIds.end(), [tileTextures](size_t first, size_t second) {
              double* firstBounds = tileTextures[first]->GetBounds();
              double* secondBounds = tileTextures[second]->GetBounds();
              return (firstBounds[3] - firstBounds[2]) > (secondBounds[3] - secondBounds[2]);
            });
          std::string mergedFileName = "merged_texture_" + std::to_string(i) + ".png";
          int tileDims[3];
          vtkSmartPointer<vtkImageData> tileImage =
            MergeTextures(tileTextures, textureIds, mergedTextureWidth, textureOrigin);
          tileImage->GetDimensions(tileDims);
          mergedFileNames[i] = mergedFileName;
          this->WriteTileTexture(node, mergedFileName, tileImage);
          if (i == 0)
          {
            // we only need to change the tcoords for the first set of textures
            // all sets share the same tcoords
            TranslateTCoords(tileTextures, textureOrigin, tileDims, meshTCoords);
          }
        }
      }

      vtkNew<vtkMultiBlockDataSet> b;
      int meshBlockIndex = 0;
      // merge meshes without textures
      if (meshes.size() > 1)
      {
        vtkNew<vtkAppendPolyData> append;
        for (vtkPolyData* pd : meshes)
        {
          append->AddInputDataObject(pd);
        }
        append->Update();
        vtkPolyData* tileMeshWithoutTexture = vtkPolyData::SafeDownCast(append->GetOutput());
        b->SetBlock(meshBlockIndex++, tileMeshWithoutTexture);
      }
      else
      {
        if (!meshes.empty())
        {
          b->SetBlock(meshBlockIndex++, meshes[0]);
        }
      }

      // merge meshes with textures
      if (meshesWithTexture.size() > 1)
      {
        vtkNew<vtkAppendPolyData> append;
        for (vtkPolyData* pd : meshesWithTexture)
        {
          append->AddInputDataObject(pd);
        }
        append->Update();
        vtkPolyData* tileMeshWithTexture = vtkPolyData::SafeDownCast(append->GetOutput());
        b->SetBlock(meshBlockIndex++, tileMeshWithTexture);
        SetField(tileMeshWithTexture, "texture_uri", mergedFileNames);
        textureBaseDirectory = this->OutputDir + "/" + std::to_string(node->GetID());
      }
      else
      {
        if (!meshesWithTexture.empty())
        {
          b->SetBlock(meshBlockIndex++, meshesWithTexture[0]);
          textureBaseDirectory = this->TextureBaseDirectory;
        }
      }
      tile->SetBlock(0, b);
    }
    else
    {
      for (int i = 0; i < pointIds->GetNumberOfIds(); ++i)
      {
        int buildingId = pointIds->GetId(i);
        // add all buildings to the tile
        tile->SetBlock(i, (*this->Buildings)[buildingId]);
        // ostr << buildingId << " ";
      }
      textureBaseDirectory = this->TextureBaseDirectory;
    }

    // vtkLog(INFO, << ostr.str());
    vtkNew<vtkGLTFWriter> writer;
    writer->RelativeCoordinatesOn();
    writer->SetInputData(tile);
    ostr.str("");
    ostr << this->OutputDir << "/" << node->GetID();
    vtkDirectory::MakeDirectory(ostr.str().c_str());
    ostr << "/" << node->GetID() << (this->ContentGLTFSaveGLB ? ".glb" : ".gltf");
    writer->SetFileName(ostr.str().c_str());
    writer->SetTextureBaseDirectory(textureBaseDirectory.c_str());
    if (!this->PropertyTextureFile.empty())
    {
      writer->SetPropertyTextureFile(this->PropertyTextureFile.c_str());
    }
    writer->SetSaveTextures(this->SaveTextures);
    // if you use the gltf format, 3DTiles are not served correctly if
    // the textures are in a different location.
    writer->SetCopyTextures(true);
    writer->SetSaveNormal(true);
    vtkLog(INFO,
      "Saving GLTF file: " << ostr.str() << " for " << pointIds->GetNumberOfIds()
                           << " buildings...");
    writer->Write();
  }
}

struct RegionCellId
{
  RegionType Region;
  vtkIdType CellId;
};

//------------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> TreeInformation::SplitTileTexture(
  vtkPolyData* tileMesh, vtkImageData* datasetImage, vtkDataArray* tcoordsTile)
{
  if (!datasetImage)
  {
    return nullptr;
  }
  // types of color components for PNG and Jpeg images.
  typedef vtkArrayDispatch::DispatchByValueType<vtkTypeList::Create<unsigned char, unsigned short>>
    Dispatcher;

  // tile texture triangles bounding box (BB) in datasetImage index coordinates:
  // minx, maxx, miny, maxy
  std::vector<RegionCellId> scatteredRegions(tileMesh->GetNumberOfCells());
  // list of rows, each element in the row is a index in scatteredRegions
  std::vector<std::vector<size_t>> groupedRegions;
  // the width and the height of each row (in points).
  std::vector<std::array<int, 2>> rowWidthHeight;
  // coordinates of the tile mesh in datasetImage index coordinates
  std::vector<std::array<std::array<int, 2>, 3>> datasetCoordinates(tileMesh->GetNumberOfCells());

  // compute scatteredRegions
  vtkDataArray* tcoordsDataset = tileMesh->GetPointData()->GetTCoords();
  int datasetDims[3];
  datasetImage->GetDimensions(datasetDims);
  for (vtkIdType i = 0; i < tileMesh->GetNumberOfCells(); ++i)
  {
    vtkCell* cell = tileMesh->GetCell(i);
    RegionType bb{ { std::numeric_limits<int>::max(), std::numeric_limits<int>::min(),
      std::numeric_limits<int>::max(), std::numeric_limits<int>::min(), 0, 0 } };
    for (int j = 0; j < 3; ++j)
    {
      vtkIdType pointId = cell->GetPointId(j);
      int x = tcoordsDataset->GetComponent(pointId, 0) * (datasetDims[0] - 1);
      int y = tcoordsDataset->GetComponent(pointId, 1) * (datasetDims[1] - 1);
      datasetCoordinates[i][j] = { { x, y } };
      bb[0] = std::min(bb[0], x);
      bb[1] = std::max(bb[1], x);
      bb[2] = std::min(bb[2], y);
      bb[3] = std::max(bb[3], y);
    }
    scatteredRegions[i].Region = bb;
    scatteredRegions[i].CellId = i;
  }
  // sort decreasing on height of the BB
  std::sort(scatteredRegions.begin(), scatteredRegions.end(),
    [](const RegionCellId& first, const RegionCellId& second) {
      return (first.Region[3] - first.Region[2]) > (second.Region[3] - second.Region[2]);
    });
  // approximate the width in pixels of the new image
  float average = 0.0f;
  for (size_t i = 0; i < scatteredRegions.size(); ++i)
  {
    average = average + (scatteredRegions[i].Region[1] - scatteredRegions[i].Region[0] + 1);
  }
  average /= scatteredRegions.size();
  int width = std::ceil(std::sqrt(scatteredRegions.size())) * average;

  // place cells in the new image using Next-Fit Decreasing Height (NFDH) algorithm
  // https://cgi.csc.liv.ac.uk/~epa/surveyhtml.html
  groupedRegions.emplace_back();
  int currentWidth = 0;
  int currentHeight = (scatteredRegions[0].Region[3] - scatteredRegions[0].Region[2] + 1);
  for (size_t i = 0; i < scatteredRegions.size(); ++i)
  {
    size_t currentRow = groupedRegions.size() - 1;
    if (currentWidth + (scatteredRegions[i].Region[1] - scatteredRegions[i].Region[0] + 1) <= width)
    {
      // add cell to current row
      groupedRegions[currentRow].emplace_back(i);
      currentWidth += (scatteredRegions[i].Region[1] - scatteredRegions[i].Region[0] + 1);
    }
    else
    {
      if (currentWidth == 0)
      {
        // the region does not fit in an empty row
        vtkLog(ERROR,
          "Empty row of size  " << width << " is too small for region of size "
                                << (scatteredRegions[i].Region[1] - scatteredRegions[i].Region[0] +
                                     1));
        return nullptr;
      }
      // create a new row and add the cell there
      groupedRegions.emplace_back();
      rowWidthHeight.push_back({ { currentWidth, currentHeight } });
      currentWidth = 0;
      currentHeight = (scatteredRegions[i].Region[3] - scatteredRegions[i].Region[2] + 1);
      --i;
    }
  }
  rowWidthHeight.push_back({ { currentWidth, currentHeight } });

  // create the tile image
  std::array<int, 3> tileDims{ { std::numeric_limits<int>::min(), 0, 1 } };
  for (size_t i = 0; i < rowWidthHeight.size(); ++i)
  {
    tileDims[0] = std::max(tileDims[0], static_cast<int>(rowWidthHeight[i][0]));
    tileDims[1] += rowWidthHeight[i][1];
  }
  auto tileImage = vtkSmartPointer<vtkImageData>::New();
  tileImage->SetDimensions(tileDims.data());
  vtkDataArray* colors = datasetImage->GetPointData()->GetScalars();
  auto tileColors = vtk::TakeSmartPointer<vtkDataArray>(colors->NewInstance());
  tileColors->SetNumberOfComponents(colors->GetNumberOfComponents());
  tileColors->SetNumberOfTuples(tileImage->GetNumberOfPoints());
  InitializeWorker initializeWorker;
  if (!Dispatcher::Execute(tileColors, initializeWorker))
  {
    vtkLog(ERROR,
      "Invalid image type: " << colors->GetDataType()
                             << " expecting unsigned char or unsigned short.");
    return tileImage;
  }
  tileImage->GetPointData()->SetScalars(tileColors);
  int tileX = 0, tileY = 0;
  vtkIdType sortedIndex = 0;

  vtkCellArray* cellArray = tileMesh->GetPolys();
  // for all rows
  for (size_t i = 0; i < groupedRegions.size(); ++i)
  { // for all cells in a row
    for (size_t j = 0; j < groupedRegions[i].size(); ++j, ++sortedIndex)
    {
      vtkIdType cellId = scatteredRegions[sortedIndex].CellId;
      auto datasetRegion = scatteredRegions[groupedRegions[i][j]].Region;
      RegionType tileRegion{ { tileX, datasetRegion[1] - datasetRegion[0] + tileX, tileY,
        datasetRegion[3] - datasetRegion[2] + tileY, datasetRegion[4], datasetRegion[5] } };
      // recompute texture coordinates to refer to tile image instead of dataset image
      vtkCell* cell = tileMesh->GetCell(cellId);
      if (cell->GetCellType() != VTK_TRIANGLE)
      {
        vtkLog(
          ERROR, "We only know to process triangles but we got cell type: " << cell->GetCellType());
        return tileImage;
      }
      // std::cout << "Cell " << cellIndex << ": " << std::endl;
      // std::cout << "tileX: " << tileX << " tileY: " << tileY << std::endl;
      // std::cout << "dataset region: ";
      // std::copy(datasetRegion.begin(),
      //           datasetRegion.end(),
      //           std::ostream_iterator<int>(std::cout," "));
      // std::cout << "\ntile region: ";
      // std::copy(tileRegion.begin(),
      //           tileRegion.end(),
      //           std::ostream_iterator<int>(std::cout," "));
      // std::cout << std::endl;
      if (tcoordsTile)
      {
        for (int k = 0; k < 3; ++k)
        {
          vtkIdType pointId = cell->GetPointId(k);
          std::array<int, 2> datasetPoint = datasetCoordinates[cellId][k];
          std::array<int, 2> tilePoint = { { datasetPoint[0] - datasetRegion[0] + tileX,
            datasetPoint[1] - datasetRegion[2] + tileY } };
          double tcoords[2] = { static_cast<double>(tilePoint[0]) / tileDims[0],
            static_cast<double>(tilePoint[1]) / tileDims[1] };
          // std::cout << "d: " << datasetPoint[0] << ", " << datasetPoint[1]
          //           << " t: " << tilePoint[0] << ", " << tilePoint[1] << std::endl;
          double tcoord0;
          tcoord0 = tcoordsTile->GetComponent(pointId, 0);
          if (tcoord0 != -1)
          {
            // need to duplicate pointId as it has different texture coordinates in
            // different cells.
            vtkPoints* points = tileMesh->GetPoints();
            points->InsertNextPoint(points->GetPoint(pointId));
            vtkPointData* pointData = tileMesh->GetPointData();
            pointData->CopyAllocate(pointData, points->GetNumberOfPoints());
            pointData->CopyData(pointData, pointId, points->GetNumberOfPoints() - 1);
            cellArray->ReplaceCellPointAtId(cellId, k, points->GetNumberOfPoints() - 1);
            pointId = points->GetNumberOfPoints() - 1;
            tcoordsTile->InsertNextTuple(tcoords);
          }
          else
          {
            tcoordsTile->SetTuple(pointId, tcoords);
          }
        }
      }
      // copy a region from the dataset to the tile image
      CopyScalarsWorker copyWorker;
      if (!Dispatcher::Execute(
            colors, copyWorker, datasetImage, datasetRegion, tileImage, tileRegion))
      {
        vtkLog(ERROR,
          "Invalid image type: " << colors->GetDataType()
                                 << " expecting unsigned char or unsigned short.");
        return tileImage;
      }
      tileX += (tileRegion[1] - tileRegion[0] + 1);
    }
    tileX = 0;
    tileY += rowWidthHeight[i][1];
  }
  return tileImage;
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTileMesh(vtkIncrementalOctreeNode* node, void* voidAux)
{
  if (node->IsLeaf() && !this->EmptyNode[node->GetID()])
  {
    SaveTileMeshData* aux = static_cast<SaveTileMeshData*>(voidAux);
    std::ostringstream ostr;
    // extract all cells/points in a tile
    vtkIdList* cellIdList = node->GetPointIds();
    vtkNew<vtkIdTypeArray> cellIds;
    cellIds->SetArray(cellIdList->GetPointer(0), cellIdList->GetNumberOfIds(), 1 /*save*/);
    vtkNew<vtkSelectionNode> selectionNode;
    selectionNode->SetSelectionList(cellIds);
    selectionNode->SetFieldType(aux->SelectionField);
    selectionNode->SetContentType(vtkSelectionNode::INDICES);
    vtkNew<vtkSelection> selection;
    selection->AddNode(selectionNode);
    vtkNew<vtkExtractSelection> extractSelection;
    extractSelection->SetInputData(0, this->Mesh);
    extractSelection->SetInputData(1, selection);
    vtkNew<vtkGeometryFilter> geometryFilter;
    geometryFilter->SetInputConnection(extractSelection->GetOutputPort());
    geometryFilter->Update();
    vtkPolyData* tileMesh = vtkPolyData::SafeDownCast(geometryFilter->GetOutput());
    ostr << this->OutputDir << "/" << node->GetID();
    vtkDirectory::MakeDirectory(ostr.str().c_str());
    ostr << "/" << node->GetID();
    // compute tile texture
    if (aux->SelectionField == vtkSelectionNode::CELL && !aux->TextureImages.empty() &&
      tileMesh->GetPointData()->GetTCoords())
    {
      std::vector<std::string> tileTextureFileNames;
      vtkDataArray* tcoordsDataset = tileMesh->GetPointData()->GetTCoords();
      auto tcoordsTile = vtk::TakeSmartPointer<vtkDataArray>(tcoordsDataset->NewInstance());
      tcoordsTile->SetNumberOfComponents(2);
      tcoordsTile->SetNumberOfTuples(tileMesh->GetNumberOfPoints());
      tcoordsTile->Fill(-1);
      int* dims = aux->TextureImages[0]->GetDimensions();
      int maxDim = dims[0];
      size_t maxIndex = 0;
      double ratio0 = static_cast<double>(dims[0]) / dims[1];
      for (size_t i = 1; i < aux->TextureImages.size(); ++i)
      {
        auto textureImage = aux->TextureImages[i];
        dims = textureImage->GetDimensions();
        double ratio = static_cast<double>(dims[0]) / dims[1];
        if (!vtkMathUtilities::FuzzyCompare(ratio0, ratio))
        {
          vtkLog(WARNING,
            "Different ratios for textures with the same texture coordinates 0:"
              << ratio0 << " " << i << ": " << ratio);
        }
        if (maxDim < dims[0])
        {
          maxDim = dims[0];
          maxIndex = i;
        }
      }
      for (size_t i = 0; i < aux->TextureImages.size(); ++i)
      {
        auto datasetImage = aux->TextureImages[i];
        auto tileImage =
          this->SplitTileTexture(tileMesh, datasetImage, maxIndex == i ? tcoordsTile : nullptr);
        if (tileImage)
        {
          this->WriteTileTexture(node, std::to_string(i) + ".png", tileImage);
          tileTextureFileNames.push_back(
            std::to_string(node->GetID()) + "/" + std::to_string(i) + ".png");
        }
      }
      tileMesh->GetPointData()->SetTCoords(tcoordsTile);
      SetField(tileMesh, "texture_uri", tileTextureFileNames);
    }
    // store tileMesh into a multiblock
    vtkNew<vtkMultiBlockDataSet> buildings;
    vtkNew<vtkMultiBlockDataSet> building;
    buildings->SetNumberOfBlocks(1);
    building->SetNumberOfBlocks(1);
    buildings->SetBlock(0, building);
    building->SetBlock(0, tileMesh);

    // write tileMesh to GLTF
    vtkNew<vtkGLTFWriter> writer;
    writer->RelativeCoordinatesOn();
    writer->SetInputData(buildings);
    std::string fileName = ostr.str() + (this->ContentGLTFSaveGLB ? ".glb" : ".gltf");
    writer->SetFileName(fileName.c_str());
    writer->SetTextureBaseDirectory(this->OutputDir.c_str());
    if (!this->PropertyTextureFile.empty())
    {
      writer->SetPropertyTextureFile(this->PropertyTextureFile.c_str());
    }
    writer->SetSaveTextures(this->SaveTextures);
    if (aux->SelectionField == vtkSelectionNode::CELL)
    {
      writer->SetSaveNormal(true);
    }
    vtkLog(INFO,
      "Saving GLTF file: " << fileName << " for " << cellIdList->GetNumberOfIds()
                           << (aux->SelectionField == vtkSelectionNode::CELL ? " cells..."
                                                                             : " points..."));
    writer->Write();
  }
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTilePoints(vtkIncrementalOctreeNode* node, void* voidAux)
{
  if (this->ContentGLTF)
  {
    SaveTileMeshData aux(*static_cast<vtkSelectionNode::SelectionField*>(voidAux),
      std::vector<vtkSmartPointer<vtkImageData>>());
    SaveTileMesh(node, &aux);
  }
  else if (node->IsLeaf() && !this->EmptyNode[node->GetID()])
  {
    vtkSmartPointer<vtkIdList> pointIds = node->GetPointIds();
    vtkNew<vtkCesiumPointCloudWriter> writer;
    writer->SetInputDataObject(this->Points);
    writer->SetPointIds(pointIds);
    std::ostringstream ostr;
    ostr << this->OutputDir << "/" << node->GetID();
    vtkDirectory::MakeDirectory(ostr.str().c_str());
    ostr << "/" << node->GetID() << this->ContentTypeExtension();
    writer->SetFileName(ostr.str().c_str());
    writer->Write();
  }
}

//------------------------------------------------------------------------------
double TreeInformation::ComputeGeometricErrorTilesetBuildings()
{
  // buildings in child nodes contribute to the error in the parent
  vtkIdList* rootBuildings = this->Root->GetPointIdSet();
  double geometricError = this->GeometricError[this->Root->GetID()];
  if (rootBuildings)
  {
    double tilesetError = geometricError * geometricError;
    for (int i = 0; i < rootBuildings->GetNumberOfIds(); ++i)
    {
      double bb[6];
      (*this->Buildings)[rootBuildings->GetId(i)]->GetBounds(bb);
      std::array<double, 3> length = { { bb[1] - bb[0], bb[3] - bb[2], bb[5] - bb[4] } };
      double length2 = length[0] * length[0] + length[1] * length[1] + length[2] * length[2];
      tilesetError = std::max(tilesetError, length2);
    }
    return std::pow(tilesetError, 1.0 / 2);
  }
  else
  {
    return geometricError;
  }
}

//------------------------------------------------------------------------------
double TreeInformation::ComputeGeometricErrorNodeBuildings(vtkIncrementalOctreeNode* node, void*)
{
  if (node->IsLeaf())
  {
    return 0;
  }
  else
  {
    double geometricError = 0;
    for (int i = 0; i < 8; ++i)
    {
      // buildings in child nodes contribute to the error in the parent
      vtkIncrementalOctreeNode* child = node->GetChild(i);
      auto geometricError_2 =
        this->GeometricError[child->GetID()] * this->GeometricError[child->GetID()];
      geometricError = std::max(geometricError, geometricError_2);
      vtkIdList* childBuildings = child->GetPointIdSet();
      if (childBuildings)
      {
        for (vtkIdType j = 0; j < childBuildings->GetNumberOfIds(); ++j)
        {
          double bb[6];
          (*this->Buildings)[childBuildings->GetId(j)]->GetBounds(bb);
          std::array<double, 3> length = { { bb[1] - bb[0], bb[3] - bb[2], bb[5] - bb[4] } };
          double length2 = length[0] * length[0] + length[1] * length[1] + length[2] * length[2];
          geometricError = std::max(geometricError, length2);
        }
      }
    }
    return std::sqrt(geometricError);
  }
}

//------------------------------------------------------------------------------
double TreeInformation::ComputeGeometricErrorTilesetPoints()
{
  double geometricError;
  // buildings in child nodes contribute to the error in the parent
  geometricError = this->GeometricError[this->Root->GetID()];
  vtkIdList* rootPoints = this->Root->GetPointIdSet();
  if (rootPoints)
  {
    double bb[6];
    this->Root->GetBounds(bb);
    double diagonal = std::sqrt((bb[1] - bb[0]) * (bb[1] - bb[0]) +
      (bb[3] - bb[2]) * (bb[3] - bb[2]) + (bb[5] - bb[4]) * (bb[5] - bb[4]));
    geometricError = std::max(geometricError, diagonal);
  }
  return geometricError;
}

//------------------------------------------------------------------------------
double TreeInformation::ComputeGeometricErrorNodePoints(vtkIncrementalOctreeNode* node, void*)
{
  if (node->IsLeaf())
  {
    return 0.0;
  }
  else
  {
    double geometricError = 0;
    for (int i = 0; i < 8; ++i)
    {
      // buildings in child nodes contribute to the error in the parent
      vtkIncrementalOctreeNode* childNode = node->GetChild(i);
      geometricError = std::max(geometricError, this->GeometricError[childNode->GetID()]);
      vtkIdList* childPoints = childNode->GetPointIdSet();
      if (childPoints)
      {
        double bb[6];
        childNode->GetBounds(bb);
        double diagonal = std::sqrt((bb[1] - bb[0]) * (bb[1] - bb[0]) +
          (bb[3] - bb[2]) * (bb[3] - bb[2]) + (bb[5] - bb[4]) * (bb[5] - bb[4]));
        geometricError = std::max(geometricError, diagonal);
      }
    }
    return std::max(geometricError, MIN_ERROR);
  }
}

double TreeInformation::GetRootLength2()
{
  std::array<double, 6>& bb = this->NodeTightBounds[this->Root->GetID()];
  std::array<double, 3> length = { { bb[1] - bb[0], bb[3] - bb[2], bb[5] - bb[4] } };
  return length[0] * length[0] + length[1] * length[1] + length[2] * length[2];
}

//------------------------------------------------------------------------------
double TreeInformation::ComputeGeometricErrorTilesetMesh()
{
  double length2 = this->GetRootLength2();
  return std::sqrt(length2);
}

//------------------------------------------------------------------------------
double TreeInformation::ComputeGeometricErrorNodeMesh(
  vtkIncrementalOctreeNode* vtkNotUsed(node), void* aux)
{
  double* parentError = static_cast<double*>(aux);
  return *parentError / 2;
}

//------------------------------------------------------------------------------
double TreeInformation::ComputeGeometricErrorTileset()
{
  double d = 0;
  switch (this->InputType)
  {
    case vtkCesium3DTilesWriter::Buildings:
      return ComputeGeometricErrorTilesetBuildings();
    case vtkCesium3DTilesWriter::Points:
      return ComputeGeometricErrorTilesetPoints();
    case vtkCesium3DTilesWriter::Mesh:
      return ComputeGeometricErrorTilesetMesh();
    default:
      vtkLog(ERROR, "Invalid InputType " << this->InputType);
  }
  return d;
}

//------------------------------------------------------------------------------
double TreeInformation::ComputeGeometricErrorNode(vtkIncrementalOctreeNode* node, void* aux)
{
  double d = 0;
  switch (this->InputType)
  {
    case vtkCesium3DTilesWriter::Buildings:
      return ComputeGeometricErrorNodeBuildings(node, aux);
    case vtkCesium3DTilesWriter::Points:
      return ComputeGeometricErrorNodePoints(node, aux);
    case vtkCesium3DTilesWriter::Mesh:
      return ComputeGeometricErrorNodeMesh(node, aux);
    default:
      vtkLog(ERROR, "Invalid InputType " << this->InputType);
  }
  return d;
}

//------------------------------------------------------------------------------
std::array<double, 6> TreeInformation::ComputeTightBB(vtkIdList* tileFeatures)
{

  std::array<double, 6> d;
  d.fill(0);
  switch (this->InputType)
  {
    case vtkCesium3DTilesWriter::Buildings:
      return ComputeTightBBBuildings(this->Buildings, tileFeatures);
    case vtkCesium3DTilesWriter::Points:
      return ComputeTightBBPoints(this->Points, tileFeatures);
    case vtkCesium3DTilesWriter::Mesh:
      return ComputeTightBBMesh(this->Mesh, tileFeatures);
    default:
      vtkLog(ERROR, "Invalid InputType " << this->InputType);
  }
  return d;
}

//------------------------------------------------------------------------------
void TreeInformation::VisitComputeGeometricError(vtkIncrementalOctreeNode* node, void* aux)
{
  if (node->IsLeaf())
  {
    this->GeometricError[node->GetID()] = 0;
  }
  else
  {
    this->GeometricError[node->GetID()] = this->ComputeGeometricErrorNode(node, aux);
  }
}

//------------------------------------------------------------------------------
void TreeInformation::VisitCompute(vtkIncrementalOctreeNode* node, void*)
{
  vtkIdList* nodeFeatures = node->GetPointIdSet();
  // compute the bounding box for the current node
  if (nodeFeatures)
  {
    this->NodeTightBounds[node->GetID()] = this->ComputeTightBB(nodeFeatures);
    this->EmptyNode[node->GetID()] = false;
  }
  // propagate the node bounding box from the children.
  if (!node->IsLeaf())
  {
    for (int i = 0; i < 8; ++i)
    {
      // buildings in child nodes contribute to the error in the parent
      vtkIncrementalOctreeNode* child = node->GetChild(i);
      if (!this->EmptyNode[child->GetID()])
      {
        this->NodeTightBounds[node->GetID()] =
          ExpandBounds(this->NodeTightBounds[node->GetID()].data(),
            this->NodeTightBounds[child->GetID()].data());
        this->EmptyNode[node->GetID()] = false;
      }
    }
  }
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTileset(const std::string& output)
{
  this->SaveTileset(this->Root, output);
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTileset(vtkIncrementalOctreeNode* root, const std::string& output)
{
  nlohmann::json v;
  this->RootJson["asset"]["version"] = "1.0";
  if (this->ContentGLTF)
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
  this->RootJson["geometricError"] = this->ComputeGeometricErrorTileset();
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
nlohmann::json TreeInformation::GenerateTileJson(vtkIncrementalOctreeNode* node)
{
  nlohmann::json tree;
  nlohmann::json v;
  std::array<double, 6> nodeBounds = this->NodeTightBounds[node->GetID()];
  std::array<double, 6> lonLatRadiansHeight = ToLonLatRadiansHeight(this->CRS, nodeBounds);
  std::ostringstream ostr;
  for (int i = 0; i < 6; ++i)
  {
    v[i] = lonLatRadiansHeight[i];
  }
  tree["boundingVolume"]["region"] = v;
  tree["geometricError"] = this->GeometricError[node->GetID()];
  if (node == this->Root)
  {
    // for points and mesh do the conversion to cartesian for the whole dataset
    switch (this->InputType)
    {
      case vtkCesium3DTilesWriter::Points:
        ConvertDataSetCartesian(this->Points);
        break;
      case vtkCesium3DTilesWriter::Mesh:
        ConvertDataSetCartesian(this->Mesh);
        break;
      default:
        break;
    }
    tree["refine"] = "REPLACE";
    if (this->InputType != vtkCesium3DTilesWriter::Points || this->ContentGLTF)
    {
      // gltf y-up to 3d-tiles z-up transform
      std::array<double, 16> t = { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 1.0 };
      tree["transform"] = t;
    }
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
      if (this->InputType == vtkCesium3DTilesWriter::Buildings &&
        !ConvertTileCartesianBuildings(node))
      {
        return tree;
      }
      ostr.str("");
      ostr << node->GetID() << "/" << node->GetID() << this->ContentTypeExtension();
      tree["content"]["uri"] = ostr.str();
    }
  }
  return tree;
}

std::string TreeInformation::ContentTypeExtension() const
{
  int index = this->ContentGLTF ? (this->ContentGLTFSaveGLB ? 1 : 2) : 0;
  switch (this->InputType)
  {
    case vtkCesium3DTilesWriter::Buildings:
      return BuildingsContentTypeExtension[index];
    case vtkCesium3DTilesWriter::Points:
      return PointsContentTypeExtension[index];
    case vtkCesium3DTilesWriter::Mesh:
      return BuildingsContentTypeExtension[index];
    default:
      vtkLog(ERROR, "Invalid InputType " << this->InputType);
      return "";
  }
}

//------------------------------------------------------------------------------
bool TreeInformation::ConvertTileCartesianBuildings(vtkIncrementalOctreeNode* node)
{
  PJ* P;
  P = proj_create_crs_to_crs(PJ_DEFAULT_CTX, this->CRS, "+proj=cart", nullptr);
  if (P == nullptr)
  {
    vtkLog(ERROR, "proj_create_crs_to_crs failed: " << proj_errno_string(proj_errno(nullptr)));
    return false;
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
    vtkLog(
      ERROR, "proj_normalize_for_visualization failed: " << proj_errno_string(proj_errno(nullptr)));
    return false;
  }
  proj_destroy(P);
  P = P_for_GIS;

  // transform points to Cartesian coordinates
  std::function<bool(vtkPolyData*)> transformPointToCartesian = [P](vtkPolyData* pd) {
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
        return false;
      }
    }
    double* d = da->GetPointer(0);
    int n = da->GetNumberOfTuples();
    proj_trans_generic(P, PJ_FWD, d, sizeof(d[0]) * 3, n, d + 1, sizeof(d[0]) * 3, n, d + 2,
      sizeof(d[0]) * 3, n, nullptr, 0, 0);
    pd->GetPoints()->Modified();
    if (conversion)
    {
      pd->GetPoints()->SetData(newPoints);
    }
    return true;
  };
  this->ForEachBuilding(node, transformPointToCartesian);
  proj_destroy(P);
  return true;
}

//------------------------------------------------------------------------------
bool TreeInformation::ConvertDataSetCartesian(vtkPointSet* pointSet)
{
  PJ* P;
  P = proj_create_crs_to_crs(PJ_DEFAULT_CTX, this->CRS, "+proj=cart", nullptr);
  if (P == nullptr)
  {
    vtkLog(ERROR, "proj_create_crs_to_crs failed: " << proj_errno_string(proj_errno(nullptr)));
    return false;
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
    vtkLog(
      ERROR, "proj_normalize_for_visualization failed: " << proj_errno_string(proj_errno(nullptr)));
    return false;
  }
  proj_destroy(P);
  P = P_for_GIS;

  // transform points to Cartesian coordinates
  vtkDataArray* points = pointSet->GetPoints()->GetData();
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
      return false;
    }
  }
  double* d = da->GetPointer(0);
  int n = da->GetNumberOfTuples();
  proj_trans_generic(P, PJ_FWD, d, sizeof(d[0]) * 3, n, d + 1, sizeof(d[0]) * 3, n, d + 2,
    sizeof(d[0]) * 3, n, nullptr, 0, 0);
  pointSet->GetPoints()->Modified();
  if (conversion)
  {
    pointSet->GetPoints()->SetData(newPoints);
  }
  proj_destroy(P);
  return true;
}

//------------------------------------------------------------------------------
bool TreeInformation::GetNodeTightBounds(int i, double* bounds)
{
  if (this->EmptyNode[i])
  {
    return false;
  }
  std::copy(this->NodeTightBounds[i].begin(), this->NodeTightBounds[i].end(), bounds);
  return true;
}

//------------------------------------------------------------------------------
bool TreeInformation::GetNodeTightBounds(void* data, vtkIncrementalOctreeNode* node, double* bounds)
{
  return static_cast<TreeInformation*>(data)->GetNodeTightBounds(node->GetID(), bounds);
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
    error->SetValue(i, std::sqrt(this->GeometricError[index]));
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
VTK_ABI_NAMESPACE_END
