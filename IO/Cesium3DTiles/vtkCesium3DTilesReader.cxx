// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCesium3DTilesReader.h"

#include "vtkAppendPolyData.h"
#include "vtkCesiumB3DMReader.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkGLTFReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkResourceStream.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtksys/SystemTools.hxx"

#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

#include <array>
#include <sstream>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
using json = nlohmann::json;

namespace
{
std::string GetContentURI(const json& node)
{
  std::string contentString("content");
  std::string uriString("uri");
  std::string urlString("url");
  if (node[contentString].contains(uriString))
  {
    return node[contentString][uriString];
  }
  else if (node[contentString].contains(urlString))
  {
    return node[contentString][urlString];
  }
  else
  {
    throw std::runtime_error("content/uri or content/url not found");
  }
  return std::string();
}
}

/**
 * Stores a tileset with the list of tiles on a specified level
 */
class vtkCesium3DTilesReader::Tileset
{
public:
  Tileset(vtkCesium3DTilesReader* reader);
  ~Tileset();

  /**
   * Open and parse the tileset and compute the number of levels and
   * the number of tiles per level.
   */
  bool Open(const char* fileName);
  bool Open(const std::string& fileName) { return Open(fileName.c_str()); }

  /**
   * Returns the root of the tileset
   */
  json& GetRoot();
  /**
   * Store partitions (tiles) file names for given
   * this->Level. 'parentTransform' is used to accumulate
   * transforms from the tileset.
   */
  bool AddPartitions(json& node, int nodeLevel, const std::array<double, 16>& parentTransform);
  void AddContentPartition(json& node, int nodeLevel, std::array<double, 16>& transform);
  void AddChildrenPartitions(json& node, int nodeLevel, std::array<double, 16>& transform);

  bool IsOpen();
  void Close();
  void SetLevel(int level) { this->Level = level; }
  friend inline ostream& operator<<(ostream& out, const Tileset& tileset)
  {
    out << "FileName: " << tileset.FileName << std::endl
        << "Level: " << tileset.Level << std::endl
        << "Number of tiles: " << tileset.TileFileNames.size() << std::endl
        << "Tile paths: ";
    std::copy(tileset.TileFileNames.begin(), tileset.TileFileNames.end(),
      std::ostream_iterator<std::string>(out, " "));
    return out;
  }
  friend class vtkCesium3DTilesReader;

private:
  vtkCesium3DTilesReader* Reader;
  std::ifstream TilesetStream;
  std::string FileName;
  json TilesetJson;
  std::string DirectoryName;
  int Level;
  std::vector<std::string> TileFileNames;
  std::vector<int> TileLevels;
  std::vector<std::array<double, 16>> Transforms;
};

//------------------------------------------------------------------------------
vtkCesium3DTilesReader::Tileset::Tileset(vtkCesium3DTilesReader* reader)
  : Reader(reader)
{
}

//------------------------------------------------------------------------------
vtkCesium3DTilesReader::Tileset::~Tileset()
{
  this->TilesetStream.close();
}

//------------------------------------------------------------------------------
bool vtkCesium3DTilesReader::Tileset::Open(const char* fileName)
{
  try
  {
    if (!fileName || std::string(fileName).empty())
    {
      vtkErrorWithObjectMacro(this->Reader, "Invalid input filename: nullptr or empty");
      return false;
    }
    if (this->IsOpen())
    {
      vtkErrorWithObjectMacro(this->Reader, "File already opened: " << fileName);
      return false;
    }
    this->FileName = fileName;
    this->TilesetStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    this->TilesetStream.open(fileName);
    this->TilesetJson = json::parse(this->TilesetStream);
    std::array<double, 16> transform;
    vtkMatrix4x4::Identity(transform.data());
    this->AddPartitions(this->GetRoot(), 0, transform);
  }
  catch (std::exception& e)
  {
    this->TilesetStream.close();
    this->TilesetJson.clear();
    this->TileFileNames.clear();
    this->Transforms.clear();
    vtkErrorWithObjectMacro(this->Reader, "Error on " << fileName << ": " << e.what());
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkCesium3DTilesReader::Tileset::IsOpen()
{
  return this->TilesetStream.is_open();
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesReader::Tileset::Close()
{
  this->TilesetStream.close();
  this->TilesetJson.clear();
}

//------------------------------------------------------------------------------
json& vtkCesium3DTilesReader::Tileset::GetRoot()
{
  return this->TilesetJson["root"];
}

//------------------------------------------------------------------------------
std::pair<size_t, size_t> vtkCesium3DTilesReader::ToLocalIndex(size_t globalIndex)
{
  int tileIndex = globalIndex;
  int tilesetIndex = 0;
  while (tilesetIndex < this->Tilesets.size())
  {
    if (tileIndex < this->Tilesets[tilesetIndex]->TileFileNames.size())
    {
      break;
    }
    tileIndex -= this->Tilesets[tilesetIndex]->TileFileNames.size();
    ++tilesetIndex;
  }
  return { tilesetIndex, tileIndex };
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkCesium3DTilesReader);

//------------------------------------------------------------------------------
vtkCesium3DTilesReader::vtkCesium3DTilesReader()
{
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(0);
  this->Level = 0;
}

//------------------------------------------------------------------------------
vtkCesium3DTilesReader::~vtkCesium3DTilesReader()
{
  this->SetFileName(nullptr);
  this->Level = 0;
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesReader::ReadTiles(vtkPartitionedDataSet* pd, size_t numberOfRanks, size_t rank)
{
  size_t numberOfPartitions = std::accumulate(this->Tilesets.begin(), this->Tilesets.end(), 0,
    [](size_t sum, std::shared_ptr<Tileset> t) { return sum + t->TileFileNames.size(); });
  vtkNew<vtkTransform> transform;
  if (numberOfPartitions < numberOfRanks)
  {
    // not enough partitions for how many ranks we have
    if (rank >= numberOfPartitions)
    {
      pd->SetNumberOfPartitions(0);
    }
    else
    {
      pd->SetNumberOfPartitions(1);
      auto [tilesetIndex, tileIndex] = this->ToLocalIndex(rank);
      std::string tileFileName = this->Tilesets[tilesetIndex]->TileFileNames[tileIndex];
      transform->SetMatrix(this->Tilesets[tilesetIndex]->Transforms[tileIndex].data());
      vtkSmartPointer<vtkPolyData> tile = this->ReadTile(tileFileName, transform);
      if (tile != nullptr)
      {
        pd->SetPartition(0, tile);
      }
    }
  }
  else
  {
    // we read several partitions per rank
    size_t k = numberOfPartitions / numberOfRanks;
    size_t remainingPartitions = numberOfPartitions - k * numberOfRanks;
    pd->SetNumberOfPartitions(static_cast<unsigned int>(k + (rank < remainingPartitions ? 1 : 0)));
    int partitionIndex = 0;
    for (size_t i = rank; i < numberOfPartitions; i += numberOfRanks)
    {
      auto [tilesetIndex, tileIndex] = this->ToLocalIndex(i);
      std::string tileFileName = this->Tilesets[tilesetIndex]->TileFileNames[tileIndex];
      transform->SetMatrix(this->Tilesets[tilesetIndex]->Transforms[tileIndex].data());
      vtkLog(INFO, "Read: " << tileFileName);
      vtkSmartPointer<vtkPolyData> tile = this->ReadTile(tileFileName, transform);
      if (tile != nullptr)
      {
        pd->SetPartition(partitionIndex++, tile);
      }
      this->UpdateProgress(static_cast<double>(i) / numberOfPartitions);
    }
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkCesium3DTilesReader::ReadTile(
  std::string tileFileName, vtkTransform* transform)
{
  auto tile = vtkSmartPointer<vtkPolyData>::New();
  std::string extension = vtksys::SystemTools::GetFilenameExtension(tileFileName);
  vtkSmartPointer<vtkMultiBlockDataSet> mb;
  if (extension == ".glb" || extension == ".gltf")
  {
    vtkNew<vtkGLTFReader> tileReader;
    tileReader->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
    tileReader->SetFileName((this->DirectoryName + "/" + tileFileName).c_str());
    tileReader->Update();
    mb = vtkMultiBlockDataSet::SafeDownCast(tileReader->GetOutput());
  }
  else if (extension == ".b3dm")
  {
    vtkNew<vtkCesiumB3DMReader> tileReader;
    tileReader->SetFileName((this->DirectoryName + "/" + tileFileName).c_str());
    tileReader->Update();
    mb = vtkMultiBlockDataSet::SafeDownCast(tileReader->GetOutput());
  }
  else
  {
    throw std::runtime_error("Invalid extension for tile: " + extension);
  }
  auto it = vtkSmartPointer<vtkDataObjectTreeIterator>::Take(mb->NewTreeIterator());
  it->SetSkipEmptyNodes(true);
  it->SetVisitOnlyLeaves(true);
  it->SetTraverseSubTree(true);
  it->InitTraversal();
  vtkNew<vtkAppendPolyData> append;
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    vtkPolyData* poly = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
    if (!poly)
    {
      vtkErrorMacro("Error: Cannot read polydata from: " << this->GetFileName());
      return tile;
    }
    append->AddInputDataObject(poly);
  }
  append->Update();
  vtkNew<vtkTransformFilter> transformFilter;
  transformFilter->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  transformFilter->SetTransform(transform);
  transformFilter->SetInputConnection(append->GetOutputPort());
  transformFilter->Update();
  tile->ShallowCopy(transformFilter->GetOutput());
  return tile;
}

//------------------------------------------------------------------------------
int vtkCesium3DTilesReader::GetDepth(json& node)
{
  std::string contentString("content");
  std::string childrenString = "children";
  int max = -1;
  if (node.contains(contentString))
  {
    std::string tileFileName = GetContentURI(node);
    std::string extension = vtksys::SystemTools::GetFilenameExtension(tileFileName);
    if (extension == ".json")
    {
      std::string externalTilesetPath{ this->DirectoryName + "/" + tileFileName };
      int i = this->FileNameToTilesetIndex[externalTilesetPath];
      return this->GetDepth(Tilesets[i]->GetRoot());
    }
  }
  if (node.contains(childrenString))
  {
    json children = node[childrenString];
    for (json::iterator it = children.begin(); it != children.end(); ++it)
    {
      int childDepth = GetDepth(*it);
      if (childDepth > max)
      {
        max = childDepth;
      }
    }
  }
  return 1 + max;
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesReader::SetLevel(int level)
{
  vtkDebugMacro(<< " setting Level to " << level);
  if (this->Level != level)
  {
    this->Level = level;
    this->Tilesets[0]->SetLevel(level);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesReader::Tileset::AddContentPartition(
  json& node, int nodeLevel, std::array<double, 16>& transform)
{
  std::array<double, 16> transformYUpToZUp{ 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 1.0 };
  vtkMatrix4x4::Multiply4x4(transform.data(), transformYUpToZUp.data(), transform.data());
  // there is a tile at the current node
  std::string tileFileName = GetContentURI(node);
  std::string extension = vtksys::SystemTools::GetFilenameExtension(tileFileName);
  if (extension == ".json")
  {
    this->Reader->Tilesets.push_back(
      std::make_shared<vtkCesium3DTilesReader::Tileset>(this->Reader));
    size_t i = this->Reader->Tilesets.size() - 1;
    this->Reader->Tilesets[i]->SetLevel(this->Reader->GetLevel() - nodeLevel);
    std::string externalTilesetPath{ this->Reader->DirectoryName + "/" + tileFileName };
    this->Reader->FileNameToTilesetIndex[externalTilesetPath] = i;
    this->Reader->Tilesets[i]->Open(externalTilesetPath);
  }
  else
  {
    this->TileFileNames.push_back(tileFileName);
    this->TileLevels.push_back(nodeLevel);
    this->Transforms.push_back(transform);
  }
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesReader::Tileset::AddChildrenPartitions(
  json& node, int nodeLevel, std::array<double, 16>& transform)
{
  std::string childrenString = "children";
  // no content at the current node, use the children
  json children = node[childrenString];
  for (auto it = children.begin(); it != children.end(); ++it)
  {
    this->AddPartitions(*it, nodeLevel + 1, transform);
  }
}

//------------------------------------------------------------------------------
bool vtkCesium3DTilesReader::Tileset::AddPartitions(
  json& node, int nodeLevel, const std::array<double, 16>& parentTransform)
{
  std::array<double, 16> transform;
  std::copy(parentTransform.begin(), parentTransform.end(), transform.begin());
  std::string contentString("content");
  std::string childrenString = "children";
  std::string transformString("transform");
  if (node.contains(transformString))
  {
    auto columnNodeTransform = node[transformString].get<std::array<double, 16>>();
    std::array<double, 16> nodeTransform;
    vtkMatrix4x4::Transpose(columnNodeTransform.data(), nodeTransform.data());
    vtkMatrix4x4::Multiply4x4(transform.data(), nodeTransform.data(), transform.data());
  }
  if (!node.contains(contentString) && !node.contains(childrenString))
  {
    vtkErrorWithObjectMacro(
      this->Reader, "Node is missing both content and children: " << node.dump());
    return false;
  }

  if (this->Level <= nodeLevel)
  {
    // don't refine using children if there is a tile at the current node
    if (node.contains(contentString))
    {
      this->AddContentPartition(node, nodeLevel, transform);
    }
    else
    {
      this->AddChildrenPartitions(node, nodeLevel, transform);
    }
  }
  else
  {
    // we need to refine the tile if possible (there are children).
    if (node.contains(childrenString))
    {
      this->AddChildrenPartitions(node, nodeLevel, transform);
    }
    else
    {
      this->AddContentPartition(node, nodeLevel, transform);
    }
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

//------------------------------------------------------------------------------
int vtkCesium3DTilesReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (!this->FileName)
  {
    vtkErrorMacro("Requires valid input file name");
    return 0;
  }
  this->Tilesets.clear();
  this->Tilesets.push_back(std::make_shared<vtkCesium3DTilesReader::Tileset>(this));
  this->Tilesets[0]->SetLevel(this->Level);
  this->FileNameToTilesetIndex[this->FileName] = 0;
  this->DirectoryName = vtksys::SystemTools::GetParentDirectory(this->FileName);
  if (!this->Tilesets[0]->Open(this->FileName))
  {
    return 0;
  }
  for (int i = 0; i < this->Tilesets.size(); ++i)
  {
    vtkLog(INFO, "Tileset: " << i << ", " << *this->Tilesets[i]);
  }
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Invalid output information object");
    return 0;
  }
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//------------------------------------------------------------------------------
int vtkCesium3DTilesReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  try
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    if (!outInfo)
    {
      vtkErrorMacro("Invalid output information object");
      return 0;
    }
    size_t numberOfRanks =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    size_t rank = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    vtkPartitionedDataSet* output = vtkPartitionedDataSet::GetData(outputVector);
    this->ReadTiles(output, numberOfRanks, rank);
  }
  catch (std::exception& e)
  {
    vtkErrorMacro(<< e.what());
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkCesium3DTilesReader::CanReadFile(const char* filename)
{
  try
  {
    if (!filename || std::string(filename).empty())
    {
      vtkErrorWithObjectMacro(this, "Invalid input filename: nullptr or empty");
      return 0;
    }
    std::ifstream s;
    s.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    s.open(filename);
    json fileJson = json::parse(s);
    // check for {asset: {version: ...}}
    json j = fileJson.at("asset");
    j = j.at("version");
    // check for {root: {geometricError: ...}}
    j = fileJson.at("root");
    j = j.at("geometricError");
  }
  catch (std::exception& vtkNotUsed(e))
  {
    return 0;
  }
  return 1;
}

VTK_ABI_NAMESPACE_END
