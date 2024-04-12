// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCesium3DTilesReader.h"

#include "vtkAppendPolyData.h"
#include "vtkCesiumB3DMReader.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkGLTFReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRange.h"
#include "vtkResourceStream.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
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
  bool Open(const char* fileName, std::array<double, 16>& transform);
  bool Open(const std::string& fileName, std::array<double, 16>& transform)
  {
    return Open(fileName.c_str(), transform);
  }

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

  /**
   * Reads the tile and transforms it.
   */
  std::pair<vtkSmartPointer<vtkPartitionedDataSet>, vtkSmartPointer<vtkGLTFReader>> ReadTile(
    std::string tileFileName, vtkTransform* transform);

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
  std::string ParentDirectory;
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
bool vtkCesium3DTilesReader::Tileset::Open(const char* fileName, std::array<double, 16>& transform)
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
    this->ParentDirectory = vtksys::SystemTools::GetParentDirectory(this->FileName);
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
std::pair<vtkSmartPointer<vtkPartitionedDataSet>, vtkSmartPointer<vtkGLTFReader>>
vtkCesium3DTilesReader::Tileset::ReadTile(std::string tileFileName, vtkTransform* transform)
{
  auto tile = vtkSmartPointer<vtkPartitionedDataSet>::New();
  std::string extension = vtksys::SystemTools::GetFilenameExtension(tileFileName);
  vtkSmartPointer<vtkMultiBlockDataSet> mb;
  vtkSmartPointer<vtkGLTFReader> gltfReader;
  if (extension == ".glb" || extension == ".gltf")
  {
    vtkNew<vtkGLTFReader> tileReader;
    tileReader->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
    tileReader->SetFileName((this->ParentDirectory + "/" + tileFileName).c_str());
    tileReader->Update();
    gltfReader = tileReader;
    mb = vtkMultiBlockDataSet::SafeDownCast(tileReader->GetOutput());
  }
  else if (extension == ".b3dm")
  {
    vtkNew<vtkCesiumB3DMReader> tileReader;
    tileReader->SetFileName((this->ParentDirectory + "/" + tileFileName).c_str());
    tileReader->Update();
    gltfReader = tileReader->GetGLTFReader();
    mb = vtkMultiBlockDataSet::SafeDownCast(tileReader->GetOutput());
  }
  else
  {
    throw std::runtime_error("Invalid extension for tile: " + extension);
  }
  vtkNew<vtkTransformPolyDataFilter> transformFilter;
  transformFilter->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  transformFilter->SetTransform(transform);
  transformFilter->SetInputDataObject(mb);
  transformFilter->Update();
  mb->ShallowCopy(transformFilter->GetOutputDataObject(0));
  using Opts = vtk::DataObjectTreeOptions;
  auto range = vtk::Range(mb, Opts::SkipEmptyNodes | Opts::TraverseSubTree | Opts::VisitOnlyLeaves);
  size_t numberOfPartitions = 0;
  for (auto o : range)
  {
    ++numberOfPartitions;
  }
  tile->SetNumberOfPartitions(static_cast<unsigned int>(numberOfPartitions));
  size_t partitionIndex = 0;
  for (auto o : range)
  {
    vtkPolyData* poly = vtkPolyData::SafeDownCast(o);
    if (!poly)
    {
      vtkErrorWithObjectMacro(this->Reader, "Error: Cannot read polydata from: " << tileFileName);
      return { tile, gltfReader };
    }
    tile->SetPartition(static_cast<unsigned int>(partitionIndex), poly);
    ++partitionIndex;
  }
  return { tile, gltfReader };
}

//------------------------------------------------------------------------------
std::pair<size_t, size_t> vtkCesium3DTilesReader::ToLocalIndex(size_t globalIndex)
{
  size_t tileIndex = globalIndex;
  size_t tilesetIndex = 0;
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
void vtkCesium3DTilesReader::ReadTiles(
  vtkPartitionedDataSetCollection* pdc, size_t numberOfRanks, size_t rank)
{
  size_t numberOfPartitionedDataSets =
    std::accumulate(this->Tilesets.begin(), this->Tilesets.end(), static_cast<size_t>(0),
      [](size_t sum, std::shared_ptr<Tileset> t) { return sum + t->TileFileNames.size(); });
  vtkNew<vtkTransform> transform;
  if (numberOfPartitionedDataSets < numberOfRanks)
  {
    // not enough partitions for how many ranks we have
    if (rank >= numberOfPartitionedDataSets)
    {
      pdc->SetNumberOfPartitionedDataSets(0);
      this->TileReaders.resize(0);
    }
    else
    {
      pdc->SetNumberOfPartitionedDataSets(1);
      this->TileReaders.resize(1);
      auto tilesetIndex_tileIndex = this->ToLocalIndex(rank);
      std::string tileFileName =
        this->Tilesets[tilesetIndex_tileIndex.first]->TileFileNames[tilesetIndex_tileIndex.second];
      transform->SetMatrix(this->Tilesets[tilesetIndex_tileIndex.first]
                             ->Transforms[tilesetIndex_tileIndex.second]
                             .data());
      auto tile_reader =
        this->Tilesets[tilesetIndex_tileIndex.first]->ReadTile(tileFileName, transform);
      if (tile_reader.first != nullptr)
      {
        pdc->SetPartitionedDataSet(0, tile_reader.first);
        this->TileReaders[0] = tile_reader.second;
      }
    }
  }
  else
  {
    // we read several partitions per rank
    size_t k = numberOfPartitionedDataSets / numberOfRanks;
    size_t remainingPartitions = numberOfPartitionedDataSets - k * numberOfRanks;
    size_t rankNumberOfPartitionedDataSets =
      static_cast<unsigned int>(k + (rank < remainingPartitions ? 1 : 0));
    pdc->SetNumberOfPartitionedDataSets(static_cast<unsigned int>(rankNumberOfPartitionedDataSets));
    this->TileReaders.resize(rankNumberOfPartitionedDataSets);
    int partitionedDataSetIndex = 0;
    for (size_t i = rank; i < numberOfPartitionedDataSets; i += numberOfRanks)
    {
      auto tilesetIndex_tileIndex = this->ToLocalIndex(i);
      std::string tileFileName =
        this->Tilesets[tilesetIndex_tileIndex.first]->TileFileNames[tilesetIndex_tileIndex.second];
      transform->SetMatrix(this->Tilesets[tilesetIndex_tileIndex.first]
                             ->Transforms[tilesetIndex_tileIndex.second]
                             .data());
      vtkLog(INFO, "Read: " << tileFileName);
      auto tile_gltfReader =
        this->Tilesets[tilesetIndex_tileIndex.first]->ReadTile(tileFileName, transform);
      if (tile_gltfReader.first != nullptr)
      {
        pdc->SetPartitionedDataSet(partitionedDataSetIndex, tile_gltfReader.first);
        this->TileReaders[partitionedDataSetIndex] = tile_gltfReader.second;
        ++partitionedDataSetIndex;
      }
      this->UpdateProgress(static_cast<double>(i) / numberOfPartitionedDataSets);
    }
  }
}
//------------------------------------------------------------------------------
vtkSmartPointer<vtkGLTFReader> vtkCesium3DTilesReader::GetTileReader(size_t index)
{
  return this->TileReaders[index];
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesReader::Tileset::AddContentPartition(
  json& node, int nodeLevel, std::array<double, 16>& transform)
{
  // there is a tile at the current node
  std::string tileFileName = GetContentURI(node);
  std::string extension = vtksys::SystemTools::GetFilenameExtension(tileFileName);
  if (extension == ".json")
  {
    this->Reader->Tilesets.push_back(
      std::make_shared<vtkCesium3DTilesReader::Tileset>(this->Reader));
    size_t i = this->Reader->Tilesets.size() - 1;
    this->Reader->Tilesets[i]->SetLevel(this->Reader->GetLevel() - nodeLevel);
    std::string externalTilesetPath{ this->ParentDirectory + "/" + tileFileName };
    this->Reader->FileNameToTilesetIndex[externalTilesetPath] = i;
    this->Reader->Tilesets[i]->Open(externalTilesetPath, transform);
  }
  else
  {
    std::array<double, 16> transformYUpToZUp{ 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 1.0,
      0.0, 0.0, 0.0, 0.0, 0.0, 1.0 };
    vtkMatrix4x4::Multiply4x4(transform.data(), transformYUpToZUp.data(), transform.data());
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
  std::array<double, 16> transform;
  vtkMatrix4x4::Identity(transform.data());
  if (!this->Tilesets[0]->Open(this->FileName, transform))
  {
    return 0;
  }
  for (size_t i = 0; i < this->Tilesets.size(); ++i)
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
    auto output = vtkPartitionedDataSetCollection::GetData(outputVector);
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
