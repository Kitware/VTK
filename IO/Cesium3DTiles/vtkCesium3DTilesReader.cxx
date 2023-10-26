// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCesium3DTilesReader.h"

#include "vtkDataObjectTreeIterator.h"
#include "vtkGLTFReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
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

class vtkCesium3DTilesReader::Implementation
{
public:
  Implementation(vtkCesium3DTilesReader* reader);

  /**
   * Open and parse the tileset and compute the number of levels and
   * the number of tiles per level.
   */
  bool Open(const char* fileName);
  /**
   * Returns the root of the tileset
   */
  json& GetRoot();
  /**
   * Traverse the tree in postorder to compute the number of levels
   */
  int GetNumberOfLevels(json& node);
  /**
   * Store partitions (tiles) file names for given
   * this->Reader->Level. 'parentTransform' is used to accumulate
   * transforms from the tileset.
   */
  bool AddPartitions(json& node, int nodeLevel, const std::array<double, 16>& parentTransform);
  void AddContentPartition(json& node, std::array<double, 16>& transform);
  void AddChildrenPartitions(json& node, int nodeLevel, std::array<double, 16>& transform);

  /**
   * Read tiles and add them to 'pd' for given this->Reader->Level and numberOfRanks/rank
   * combination. 'parentTransform' is used to accumulate transforms from the tileset.
   */
  void ReadTiles(vtkPartitionedDataSet* pd, size_t numberOfRanks, size_t rank);
  /**
   * Reads the tile and transforms it.
   */
  vtkSmartPointer<vtkPolyData> ReadTile(std::string tileFileName, vtkTransform* transform);

  bool IsOpen();
  void Close();

private:
  vtkCesium3DTilesReader* Reader;
  std::ifstream TilesetStream;
  json Tileset;
  std::string DirectoryName;
  std::vector<std::string> TileFileNames;
  std::vector<std::array<double, 16>> Transforms;
};

//------------------------------------------------------------------------------
vtkCesium3DTilesReader::Implementation::Implementation(vtkCesium3DTilesReader* reader)
  : Reader(reader)
{
}

//------------------------------------------------------------------------------
bool vtkCesium3DTilesReader::Implementation::Open(const char* fileName)
{
  try
  {
    if (this->IsOpen())
    {
      TileFileNames.clear();
      Transforms.clear();
      this->Close();
    }
    this->TilesetStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    this->TilesetStream.open(fileName);
    this->Tileset = json::parse(this->TilesetStream);
    this->Reader->NumberOfLevels = this->GetNumberOfLevels(this->GetRoot());
    std::array<double, 16> transform;
    vtkMatrix4x4::Identity(&transform[0]);
    this->AddPartitions(this->GetRoot(), 0, transform);
    std::cerr << "The number of tiles on level " << this->Reader->Level << " is "
              << TileFileNames.size() << std::endl;
    this->DirectoryName = vtksys::SystemTools::GetParentDirectory(fileName);
  }
  catch (std::exception& e)
  {
    vtkErrorWithObjectMacro(this->Reader, "Error on " << fileName << ": " << e.what());
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
json& vtkCesium3DTilesReader::Implementation::GetRoot()
{
  return this->Tileset["root"];
}

//------------------------------------------------------------------------------
int vtkCesium3DTilesReader::Implementation::GetNumberOfLevels(json& node)
{
  if (!this->IsOpen())
  {
    vtkErrorWithObjectMacro(
      this->Reader, "Error: " << this->Reader->GetFileName() << " is not open.");
  }
  json children = node["children"];
  int max = 0;
  for (json::iterator it = children.begin(); it != children.end(); ++it)
  {
    int numberOfLevels = GetNumberOfLevels(*it);
    if (numberOfLevels > max)
    {
      max = numberOfLevels;
    }
  }
  return 1 + max;
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesReader::Implementation::ReadTiles(
  vtkPartitionedDataSet* pd, size_t numberOfRanks, size_t rank)
{
  size_t numberOfPartitions = this->TileFileNames.size();
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
      std::string tileFileName = this->TileFileNames[rank];
      transform->SetMatrix(this->Transforms[rank].data());
      vtkSmartPointer<vtkPolyData> tile = ReadTile(tileFileName, transform);
      pd->SetPartition(0, tile);
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
      std::string tileFileName = this->TileFileNames[i];
      transform->SetMatrix(this->Transforms[i].data());
      vtkSmartPointer<vtkPolyData> tile = ReadTile(tileFileName, transform);
      pd->SetPartition(partitionIndex++, tile);
      this->Reader->UpdateProgress(static_cast<double>(i) / numberOfPartitions);
    }
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkCesium3DTilesReader::Implementation::ReadTile(
  std::string tileFileName, vtkTransform* transform)
{
  auto tile = vtkSmartPointer<vtkPolyData>::New();
  vtkNew<vtkGLTFReader> tileReader;
  tileReader->SetFileName((this->DirectoryName + "/" + tileFileName).c_str());
  tileReader->Update();
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(tileReader->GetOutput());
  auto it = vtkSmartPointer<vtkDataObjectTreeIterator>::Take(mb->NewTreeIterator());
  it->SetSkipEmptyNodes(true);
  it->SetVisitOnlyLeaves(true);
  it->SetTraverseSubTree(true);
  it->InitTraversal();
  vtkPolyData* poly = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
  if (!poly)
  {
    vtkErrorWithObjectMacro(
      this->Reader, "Error: Cannot read polydata from: " << this->Reader->GetFileName());
    return tile;
  }
  vtkNew<vtkTransformFilter> transformFilter;
  transformFilter->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  transformFilter->SetTransform(transform);
  transformFilter->SetInputDataObject(poly);
  transformFilter->Update();
  tile->ShallowCopy(transformFilter->GetOutput());
  return tile;
}

//------------------------------------------------------------------------------
bool vtkCesium3DTilesReader::Implementation::IsOpen()
{
  return this->TilesetStream.is_open();
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesReader::Implementation::Close()
{
  this->TilesetStream.close();
  this->Tileset.clear();
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkCesium3DTilesReader);

//------------------------------------------------------------------------------
vtkCesium3DTilesReader::vtkCesium3DTilesReader()
{
  this->FileName = nullptr;
  this->Impl = new vtkCesium3DTilesReader::Implementation(this);
  this->SetNumberOfInputPorts(0);
  this->Level = 0;
  this->NumberOfLevels = 0;
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesReader::Implementation::AddContentPartition(
  json& node, std::array<double, 16>& transform)
{
  std::string contentString("content");
  std::array<double, 16> transformYUpToZUp{ 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 1.0 };
  vtkMatrix4x4::Multiply4x4(&transform[0], &transformYUpToZUp[0], &transform[0]);
  // there is a tile at the current node
  std::string tileFileName = node[contentString]["uri"];
  this->TileFileNames.push_back(tileFileName);
  this->Transforms.push_back(transform);
}

//------------------------------------------------------------------------------
void vtkCesium3DTilesReader::Implementation::AddChildrenPartitions(
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
bool vtkCesium3DTilesReader::Implementation::AddPartitions(
  json& node, int nodeLevel, const std::array<double, 16>& parentTransform)
{
  int level = this->Reader->Level;
  std::array<double, 16> transform;
  std::copy(parentTransform.begin(), parentTransform.end(), transform.begin());
  std::string contentString("content");
  std::string childrenString = "children";
  std::string transformString("transform");
  if (node.contains(transformString))
  {
    auto columnNodeTransform = node[transformString].get<std::array<double, 16>>();
    std::array<double, 16> nodeTransform;
    vtkMatrix4x4::Transpose(&columnNodeTransform[0], &nodeTransform[0]);
    vtkMatrix4x4::Multiply4x4(&transform[0], &nodeTransform[0], &transform[0]);
  }
  if (!node.contains(contentString) && !node.contains(childrenString))
  {
    vtkErrorWithObjectMacro(
      this->Reader, "Node is missing both content and children: " << node.dump());
    return false;
  }

  if (level <= nodeLevel)
  {
    // don't refine using children if there is a tile at the current node
    if (node.contains(contentString))
    {
      this->AddContentPartition(node, transform);
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
      this->AddContentPartition(node, transform);
    }
  }
  return true;
}

//------------------------------------------------------------------------------
vtkCesium3DTilesReader::~vtkCesium3DTilesReader()
{
  delete this->Impl;
  this->SetFileName(nullptr);
  this->Level = 0;
  this->NumberOfLevels = 0; // no initialized
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
  if (!this->Impl->Open(this->FileName))
  {
    return 0;
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
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Invalid output information object");
    return 0;
  }
  size_t numberOfRanks = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  size_t rank = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  vtkPartitionedDataSet* output = vtkPartitionedDataSet::GetData(outputVector);
  vtkNew<vtkTransform> transform;
  this->Impl->ReadTiles(output, numberOfRanks, rank);
  return 1;
}

VTK_ABI_NAMESPACE_END
