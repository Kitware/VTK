//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/internal/OutputBuilder.h>

namespace fides
{

size_t OutputBuilder::CreateArray(const RawArray& raw)
{
  if (!raw.IsValid())
  {
    return InvalidToken;
  }

  size_t token = this->AllocToken();
  this->StoredArrays[token] = raw;
  return token;
}

size_t OutputBuilder::CreateUniformCoordinates(const int64_t dims[3],
                                               const double origin[3],
                                               const double spacing[3],
                                               const int64_t start[3])
{
  size_t token = this->AllocToken();
  CoordEntry entry;
  entry.EntryType = CoordEntry::Type::Uniform;
  for (int i = 0; i < 3; i++)
  {
    entry.Dims[i] = dims[i];
    entry.Spacing[i] = spacing[i];
    // Default behavior: fold start into origin so downstream code that
    // works in local coordinates (e.g. Viskores) sees the right per-block
    // origin. VTKBuilder overrides this to preserve the global origin.
    entry.Origin[i] = origin[i];
    if (start)
    {
      entry.Origin[i] += spacing[i] * static_cast<double>(start[i]);
    }
  }
  this->StoredCoords[token] = entry;
  return token;
}

size_t OutputBuilder::CreateRectilinearCoordinates(size_t xToken, size_t yToken, size_t zToken)
{
  size_t token = this->AllocToken();
  CoordEntry entry;
  entry.EntryType = CoordEntry::Type::Rectilinear;
  entry.XToken = xToken;
  entry.YToken = yToken;
  entry.ZToken = zToken;
  this->StoredCoords[token] = entry;
  return token;
}

size_t OutputBuilder::CreateCompositeCoordinates(size_t xToken, size_t yToken, size_t zToken)
{
  size_t token = this->AllocToken();
  CoordEntry entry;
  entry.EntryType = CoordEntry::Type::Composite;
  entry.XToken = xToken;
  entry.YToken = yToken;
  entry.ZToken = zToken;
  this->StoredCoords[token] = entry;
  return token;
}

size_t OutputBuilder::CreateStructuredCellSet(const int64_t dims[3])
{
  size_t token = this->AllocToken();
  CellSetEntry entry;
  entry.EntryType = CellSetEntry::Type::Structured;
  for (int i = 0; i < 3; i++)
  {
    entry.Dims[i] = dims[i];
  }
  this->StoredCellSets[token] = entry;
  return token;
}

size_t OutputBuilder::CreateSingleTypeCellSet(CellShape shape, int vertsPerCell, size_t connToken)
{
  size_t token = this->AllocToken();
  CellSetEntry entry;
  entry.EntryType = CellSetEntry::Type::SingleType;
  entry.Shape = shape;
  entry.VertsPerCell = vertsPerCell;
  entry.ConnToken = connToken;
  this->StoredCellSets[token] = entry;
  return token;
}

size_t OutputBuilder::CreateExplicitCellSet(size_t typesToken, size_t nVertsToken, size_t connToken)
{
  size_t token = this->AllocToken();
  CellSetEntry entry;
  entry.EntryType = CellSetEntry::Type::Explicit;
  entry.TypesToken = typesToken;
  entry.NVertsToken = nVertsToken;
  entry.ConnTokenExplicit = connToken;
  this->StoredCellSets[token] = entry;
  return token;
}

size_t OutputBuilder::CreatePolyDataCellSet(size_t vertsOffsetsToken,
                                            size_t vertsConnToken,
                                            size_t linesOffsetsToken,
                                            size_t linesConnToken,
                                            size_t polysOffsetsToken,
                                            size_t polysConnToken,
                                            size_t stripsOffsetsToken,
                                            size_t stripsConnToken)
{
  size_t token = this->AllocToken();
  CellSetEntry entry;
  entry.EntryType = CellSetEntry::Type::PolyData;
  entry.PolyVertsOffsetsToken = vertsOffsetsToken;
  entry.PolyVertsConnToken = vertsConnToken;
  entry.PolyLinesOffsetsToken = linesOffsetsToken;
  entry.PolyLinesConnToken = linesConnToken;
  entry.PolyPolysOffsetsToken = polysOffsetsToken;
  entry.PolyPolysConnToken = polysConnToken;
  entry.PolyStripsOffsetsToken = stripsOffsetsToken;
  entry.PolyStripsConnToken = stripsConnToken;
  this->StoredCellSets[token] = entry;
  return token;
}

size_t OutputBuilder::CreateDataSet()
{
  size_t token = this->AllocToken();
  this->DataSetTokens.push_back(token);
  return token;
}

void OutputBuilder::SetCoordinateSystem(size_t dsToken, size_t coordToken)
{
  if (coordToken == InvalidToken)
  {
    return;
  }
  this->DeferredCoordSystems.push_back({ dsToken, coordToken });
}

void OutputBuilder::SetCellSet(size_t dsToken, size_t cellSetToken)
{
  if (cellSetToken == InvalidToken)
  {
    return;
  }
  this->DeferredCellSets.push_back({ dsToken, cellSetToken });
}

void OutputBuilder::AddField(size_t dsToken,
                             const std::string& name,
                             FieldAssociation assoc,
                             size_t arrayToken)
{
  if (arrayToken == InvalidToken)
  {
    return;
  }
  this->DeferredFields.push_back({ dsToken, name, assoc, arrayToken });
}

OutputBuilder::ItemEntry& OutputBuilder::CurrentItem()
{
  if (this->Items.empty())
  {
    this->Items.push_back(ItemEntry{});
  }
  return this->Items.back();
}

const std::vector<size_t>& OutputBuilder::LegacyPartitionTokens() const
{
  static const std::vector<size_t> empty;
  return this->Items.empty() ? empty : this->Items.front().Partitions;
}

void OutputBuilder::AddPartition(size_t dsToken)
{
  this->CurrentItem().Partitions.push_back(dsToken);
}

size_t OutputBuilder::CreateItem(const std::string& name)
{
  this->Items.push_back(ItemEntry{ name, {} });
  return this->Items.size() - 1;
}

void OutputBuilder::SetAssembly(const AssemblyNode& root)
{
  this->AssemblyTree = root;
  this->HasAssemblyTree = true;
}

size_t OutputBuilder::GetNumberOfItems() const
{
  return this->Items.size();
}

const std::string& OutputBuilder::GetItemName(size_t i) const
{
  return this->Items.at(i).Name;
}

const std::vector<size_t>& OutputBuilder::GetItemPartitions(size_t i) const
{
  return this->Items.at(i).Partitions;
}

bool OutputBuilder::IsMultiItem() const
{
  return this->Items.size() > 1 || (this->Items.size() == 1 && !this->Items.front().Name.empty());
}

bool OutputBuilder::HasAssembly() const
{
  return this->HasAssemblyTree;
}

const OutputBuilder::AssemblyNode& OutputBuilder::GetAssembly() const
{
  return this->AssemblyTree;
}

size_t OutputBuilder::CreateCellGrid()
{
  size_t token = this->AllocToken();
  this->StoredCellGrids[token] = CellGridEntry{};
  return token;
}

void OutputBuilder::AddCellTypeToCellGrid(size_t cgToken,
                                          const std::string& cellTypeName,
                                          const std::string& shapeName)
{
  auto it = this->StoredCellGrids.find(cgToken);
  if (it == this->StoredCellGrids.end())
  {
    return;
  }
  it->second.CellTypes.push_back({ cellTypeName, shapeName });
}

size_t OutputBuilder::CreateCellAttribute(const std::string& name,
                                          const std::string& space,
                                          int numComponents,
                                          bool isShape)
{
  size_t token = this->AllocToken();
  CellAttributeEntry entry;
  entry.Name = name;
  entry.Space = space;
  entry.Components = numComponents;
  entry.IsShape = isShape;
  this->StoredCellAttributes[token] = std::move(entry);
  return token;
}

void OutputBuilder::SetCellAttributeForType(size_t attrToken,
                                            const std::string& cellTypeName,
                                            const std::string& functionSpace,
                                            const std::string& basis,
                                            int order,
                                            const std::string& dofSharing,
                                            int64_t offset,
                                            bool blanked)
{
  auto it = this->StoredCellAttributes.find(attrToken);
  if (it == this->StoredCellAttributes.end())
  {
    return;
  }
  CellAttributePerTypeEntry entry;
  entry.CellTypeName = cellTypeName;
  entry.FunctionSpace = functionSpace;
  entry.Basis = basis;
  entry.Order = order;
  entry.DOFSharing = dofSharing;
  entry.Offset = offset;
  entry.Blanked = blanked;
  it->second.PerCellType.push_back(std::move(entry));
}

void OutputBuilder::AddCellAttributeArrayForType(size_t attrToken,
                                                 const std::string& cellTypeName,
                                                 const std::string& role,
                                                 const std::string& group,
                                                 const std::string& arrayName,
                                                 size_t arrayToken)
{
  auto it = this->StoredCellAttributes.find(attrToken);
  if (it == this->StoredCellAttributes.end())
  {
    return;
  }
  // Attach to the most recently configured per-type entry matching
  // cellTypeName (SetCellAttributeForType pushes one just before the
  // AddCellAttributeArrayForType calls for that cell type).
  for (auto rit = it->second.PerCellType.rbegin(); rit != it->second.PerCellType.rend(); ++rit)
  {
    if (rit->CellTypeName == cellTypeName)
    {
      rit->Roles[role] = { group, arrayName, arrayToken };
      return;
    }
  }
}

void OutputBuilder::AddCellAttributeToCellGrid(size_t cgToken, size_t attrToken)
{
  auto it = this->StoredCellGrids.find(cgToken);
  if (it == this->StoredCellGrids.end())
  {
    return;
  }
  it->second.AttributeTokens.push_back(attrToken);
}

size_t OutputBuilder::GetArrayNumberOfValues(size_t arrayToken) const
{
  auto it = this->StoredArrays.find(arrayToken);
  if (it == this->StoredArrays.end())
  {
    return 0;
  }
  return it->second.NumValues;
}

int OutputBuilder::GetArrayNumberOfComponents(size_t arrayToken) const
{
  auto it = this->StoredArrays.find(arrayToken);
  if (it == this->StoredArrays.end())
  {
    return 0;
  }
  return it->second.NumComponents;
}

void OutputBuilder::Reset()
{
  this->NextToken = 1;
  this->StoredArrays.clear();
  this->StoredCoords.clear();
  this->StoredCellSets.clear();
  this->DeferredCoordSystems.clear();
  this->DeferredCellSets.clear();
  this->DeferredFields.clear();
  this->DataSetTokens.clear();
  this->DataSetTokenMap.clear();
  this->Items.clear();
  this->HasAssemblyTree = false;
  this->AssemblyTree = AssemblyNode{};
  this->StoredCellGrids.clear();
  this->StoredCellAttributes.clear();
}
} // end namespace fides
