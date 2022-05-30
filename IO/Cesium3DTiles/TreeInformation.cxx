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

#include "vtk_libproj.h"

#include "vtksys/SystemTools.hxx"
#include <limits>
#include <sstream>
#include <vtksys/FStream.hxx>

using namespace nlohmann;

using RegionType = std::array<int, 6>;

namespace
{
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

std::string GetFieldAsString(vtkDataObject* obj, const char* name)
{
  vtkFieldData* fd = obj->GetFieldData();
  if (!fd)
  {
    return std::string();
  }
  vtkStringArray* sa = vtkStringArray::SafeDownCast(fd->GetAbstractArray(name));
  if (!sa)
  {
    return std::string();
  }
  return sa->GetValue(0);
}

//------------------------------------------------------------------------------
void SetField(vtkDataObject* obj, const char* name, const char* value)
{
  vtkFieldData* fd = obj->GetFieldData();
  if (!fd)
  {
    vtkNew<vtkFieldData> newfd;
    obj->SetFieldData(newfd);
    fd = newfd;
  }
  vtkNew<vtkStringArray> sa;
  sa->SetNumberOfTuples(1);
  sa->SetValue(0, value);
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
  SaveTileMeshData(int selectionField, vtkSmartPointer<vtkImageData> textureImage)
    : SelectionField(selectionField)
    , TextureImage(textureImage)
  {
  }
  int SelectionField;
  vtkSmartPointer<vtkImageData> TextureImage;
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

std::array<std::string, 3> BuildingsContentTypeExtension = { ".b3dm", ".glb" };
std::array<std::string, 3> PointsContentTypeExtension = { ".pnts", ".glb" };

}

//------------------------------------------------------------------------------
TreeInformation::TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes,
  const std::vector<vtkSmartPointer<vtkCompositeDataSet>>* buildings,
  const std::string& textureBaseDirectory, bool saveTextures, bool contentGLTF, const char* crs,
  const std::string& output)
  : InputType(vtkCesium3DTilesWriter::Buildings)
  , Root(root)
  , Buildings(buildings)
  , Points(nullptr)
  , Mesh(nullptr)
  , OutputDir(output)
  , TextureBaseDirectory(textureBaseDirectory)
  , SaveTextures(saveTextures)
  , ContentGLTF(contentGLTF)
  , CRS(crs)
  , NodeTightBounds(numberOfNodes)
  , EmptyNode(numberOfNodes)
  , GeometricError(numberOfNodes)
{
  Initialize();
}

//------------------------------------------------------------------------------
TreeInformation::TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes,
  vtkPointSet* points, bool contentGLTF, const char* crs, const std::string& output)
  : InputType(vtkCesium3DTilesWriter::Points)
  , Root(root)
  , Buildings(nullptr)
  , Points(points)
  , Mesh(vtkPolyData::SafeDownCast(points))
  , OutputDir(output)
  , SaveTextures(false)
  , ContentGLTF(contentGLTF)
  , CRS(crs)
  , NodeTightBounds(numberOfNodes)
  , EmptyNode(numberOfNodes)
  , GeometricError(numberOfNodes)
{
  Initialize();
}

//------------------------------------------------------------------------------
TreeInformation::TreeInformation(vtkIncrementalOctreeNode* root, int numberOfNodes,
  vtkPolyData* mesh, const std::string& textureBaseDirectory, bool saveTextures, bool contentGLTF,
  const char* crs, const std::string& output)
  : InputType(vtkCesium3DTilesWriter::Mesh)
  , Root(root)
  , Buildings(nullptr)
  , Points(nullptr)
  , Mesh(mesh)
  , OutputDir(output)
  , TextureBaseDirectory(textureBaseDirectory)
  , SaveTextures(saveTextures)
  , ContentGLTF(contentGLTF)
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
    double lengthAux = 2 * std::pow(length2, 1.0 / 2);
    this->PreOrderTraversal(&TreeInformation::VisitComputeGeometricError, this->Root, &lengthAux);
  }
  else
  {
    this->PostOrderTraversal(&TreeInformation::VisitComputeGeometricError, this->Root, nullptr);
  }
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTilesBuildings(bool mergeTilePolyData)
{
  this->PostOrderTraversal(&TreeInformation::SaveTileBuildings, this->Root, &mergeTilePolyData);
}

//------------------------------------------------------------------------------
void TreeInformation::SaveTilesMesh()
{
  // load the texture
  vtkSmartPointer<vtkImageData> textureImage;
  std::string textureFileName = GetFieldAsString(this->Mesh, "texture_uri");
  if (!textureFileName.empty())
  {
    std::string texturePath = this->TextureBaseDirectory + "/" + textureFileName;
    auto textureReader = SetupTextureReader(texturePath);
    if (textureReader)
    {
      textureReader->Update();
      textureImage = vtkImageData::SafeDownCast(textureReader->GetOutput());
    }
  }
  SaveTileMeshData aux(vtkSelectionNode::CELL, textureImage);
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

//------------------------------------------------------------------------------
void TreeInformation::SaveTileBuildings(vtkIncrementalOctreeNode* node, void* aux)
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
        vtkCompositeDataSet* building = (*this->Buildings)[buildingId];
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
        tile->SetBlock(i, (*this->Buildings)[buildingId]);
        // ostr << buildingId << " ";
      }
    }

    // vtkLog(INFO, << ostr.str());
    vtkNew<vtkGLTFWriter> writer;
    writer->SetInputData(tile);
    ostr.str("");
    ostr << this->OutputDir << "/" << node->GetID();
    vtkDirectory::MakeDirectory(ostr.str().c_str());
    ostr << "/" << node->GetID() << ".gltf";
    writer->SetFileName(ostr.str().c_str());
    writer->SetTextureBaseDirectory(this->TextureBaseDirectory.c_str());
    writer->SetSaveTextures(this->SaveTextures);
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
vtkSmartPointer<vtkImageData> TreeInformation::ComputeTileMeshTexture(
  vtkPolyData* tileMesh, vtkImageData* datasetImage)
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
      int x = std::round(tcoordsDataset->GetComponent(pointId, 0) * datasetDims[0]);
      int y = std::round(tcoordsDataset->GetComponent(pointId, 1) * datasetDims[1]);
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
    [](RegionCellId& first, RegionCellId& second) {
      return (first.Region[3] - first.Region[2]) > (second.Region[3] - second.Region[2]);
    });
  // approximate the width in pixels of the new image
  float average = 0.0f;
  for (size_t i = 0; i < scatteredRegions.size(); ++i)
  {
    average = average + (scatteredRegions[i].Region[1] - scatteredRegions[i].Region[0] + 1);
  }
  average /= scatteredRegions.size();
  int width = std::round(std::sqrt(scatteredRegions.size())) * average;

  // place cells in the new image using Next-Fit Decreasing Height (NFDH) algorithm
  // https://cgi.csc.liv.ac.uk/~epa/surveyhtml.html
  groupedRegions.emplace_back(std::vector<size_t>());
  int currentWidth = 0;
  int currentHeight = (scatteredRegions[0].Region[3] - scatteredRegions[0].Region[2] + 1);
  for (size_t i = 0; i < scatteredRegions.size(); ++i)
  {
    size_t currentRow = groupedRegions.size() - 1;
    if (currentWidth + (scatteredRegions[i].Region[1] - scatteredRegions[i].Region[0] + 1) < width)
    {
      // add cell to current row
      groupedRegions[currentRow].emplace_back(i);
      currentWidth += (scatteredRegions[i].Region[1] - scatteredRegions[i].Region[0] + 1);
    }
    else
    {
      // create a new row and add the cell there
      groupedRegions.emplace_back(std::vector<size_t>());
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
  auto tcoordsTile = vtk::TakeSmartPointer<vtkDataArray>(tcoordsDataset->NewInstance());
  tcoordsTile->DeepCopy(tcoordsDataset);

  // for all rows
  for (size_t i = 0; i < groupedRegions.size(); ++i)
  { // for all cells in a row
    for (size_t j = 0; j < groupedRegions[i].size(); ++j, ++sortedIndex)
    {
      vtkIdType cellIndex = scatteredRegions[sortedIndex].CellId;
      auto datasetRegion = scatteredRegions[groupedRegions[i][j]].Region;
      RegionType tileRegion{ { tileX, datasetRegion[1] - datasetRegion[0] + tileX, tileY,
        datasetRegion[3] - datasetRegion[2] + tileY, datasetRegion[4], datasetRegion[5] } };
      // recompute texture coordinates to refer to tile image instead of dataset image
      vtkCell* cell = tileMesh->GetCell(cellIndex);
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
      for (int k = 0; k < 3; ++k)
      {
        vtkIdType pointId = cell->GetPointId(k);
        std::array<int, 2> datasetPoint = datasetCoordinates[cellIndex][k];
        std::array<int, 2> tilePoint = { { datasetPoint[0] - datasetRegion[0] + tileX,
          datasetPoint[1] - datasetRegion[2] + tileY } };
        // std::cout << "d: " << datasetPoint[0] << ", " << datasetPoint[1]
        //           << " t: " << tilePoint[0] << ", " << tilePoint[1] << std::endl;
        tcoordsTile->SetComponent(pointId, 0, static_cast<double>(tilePoint[0]) / tileDims[0]);
        tcoordsTile->SetComponent(pointId, 1, static_cast<double>(tilePoint[1]) / tileDims[1]);
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
  tileMesh->GetPointData()->SetTCoords(tcoordsTile);
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
    if (aux->SelectionField == vtkSelectionNode::CELL)
    {
      auto tileImage = this->ComputeTileMeshTexture(tileMesh, aux->TextureImage);
      if (tileImage)
      {
        std::string filePath = ostr.str() + ".png";
        vtkNew<vtkPNGWriter> writer;
        writer->SetFileName(filePath.c_str());
        writer->SetInputDataObject(tileImage);
        writer->Write();
        // add texture file annotation
        SetField(tileMesh, "texture_uri",
          (std::to_string(node->GetID()) + "/" + std::to_string(node->GetID()) + ".png").c_str());
      }
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
    writer->SetInputData(buildings);
    std::string fileName = ostr.str() + ".gltf";
    writer->SetFileName(fileName.c_str());
    writer->SetTextureBaseDirectory(this->OutputDir.c_str());
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
    SaveTileMeshData aux(*static_cast<int*>(voidAux), nullptr);
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
    double tilesetError = std::pow(geometricError, 2);
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
      geometricError = std::max(geometricError, std::pow(this->GeometricError[child->GetID()], 2));
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
    return std::pow(geometricError, 1.0 / 2);
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
    double diagonal = std::pow((bb[1] - bb[0]) * (bb[1] - bb[0]) +
        (bb[3] - bb[2]) * (bb[3] - bb[2]) + (bb[5] - bb[4]) * (bb[5] - bb[4]),
      1.0 / 2);
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
        double diagonal = std::pow((bb[1] - bb[0]) * (bb[1] - bb[0]) +
            (bb[3] - bb[2]) * (bb[3] - bb[2]) + (bb[5] - bb[4]) * (bb[5] - bb[4]),
          1.0 / 2);
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
  return std::pow(length2, 1.0 / 2);
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
  json v;
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
  switch (this->InputType)
  {
    case vtkCesium3DTilesWriter::Buildings:
      return BuildingsContentTypeExtension[this->ContentGLTF];
    case vtkCesium3DTilesWriter::Points:
      return PointsContentTypeExtension[this->ContentGLTF];
    case vtkCesium3DTilesWriter::Mesh:
      return BuildingsContentTypeExtension[this->ContentGLTF];
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
  vtkIdList* pointIds = node->GetPointIds();
  // for each building
  for (int i = 0; i < pointIds->GetNumberOfIds(); ++i)
  {
    int buildingId = pointIds->GetId(i);
    vtkSmartPointer<vtkCompositeDataSet> building = (*this->Buildings)[buildingId];
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
    error->SetValue(i, std::pow(this->GeometricError[index], 1.0 / 2));
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
