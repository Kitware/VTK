// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositeCellGridReader.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridBoundsQuery.h"
#include "vtkCellGridElevationQuery.h"
#include "vtkCellGridReader.h"
#include "vtkCellGridSidesQuery.h"
#include "vtkCellMetadata.h"
#include "vtkDGBoundsResponder.h"
#include "vtkDGElevationResponder.h"
#include "vtkDGHex.h"
#include "vtkDGSidesResponder.h"
#include "vtkDGTet.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSetAttributes.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIOCellGrid.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStringToken.h"

#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

#include <array>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCompositeCellGridReader);

vtkCompositeCellGridReader::vtkCompositeCellGridReader()
{
  this->SetNumberOfInputPorts(0);
  vtkFiltersCellGrid::RegisterCellsAndResponders();
  vtkIOCellGrid::RegisterCellsAndResponders();
}

vtkCompositeCellGridReader::~vtkCompositeCellGridReader()
{
  this->SetFileName(nullptr);
}

void vtkCompositeCellGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: \"" << (this->FileName ? this->FileName : "(none)") << "\"\n";
}

vtkDataArraySelection* vtkCompositeCellGridReader::GetCellTypeSelection()
{
  return this->CellTypeSelection;
}

vtkDataArraySelection* vtkCompositeCellGridReader::GetCellAttributeSelection()
{
  return this->CellAttributeSelection;
}

vtkMTimeType vtkCompositeCellGridReader::GetMTime()
{
  vtkMTimeType result = this->Superclass::GetMTime();
  vtkMTimeType ctt = this->CellTypeSelection->GetMTime();
  vtkMTimeType cat = this->CellAttributeSelection->GetMTime();
  if (result < ctt)
  {
    result = ctt;
  }
  if (result < cat)
  {
    result = cat;
  }
  return result;
}

int vtkCompositeCellGridReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
  return 1;
}

int vtkCompositeCellGridReader::ReadMetaData(vtkInformation* metadata)
{
  vtkLogScopeF(TRACE, "ReadMetaData");

  // Read block and array info if needed.
  this->UpdateMetadata();

  metadata->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

int vtkCompositeCellGridReader::ReadMesh(
  int piece, int npieces, int nghosts, int timestep, vtkDataObject* output)
{
  (void)nghosts;
  (void)timestep;

  vtkLogScopeF(TRACE, "ReadMesh");
  vtkIndent indent;
  vtkLog(TRACE,
    "ReadMesh " << output << " p " << (piece + 1) << "/" << npieces << " g " << nghosts << " t "
                << timestep);
  auto* pdc = vtkPartitionedDataSetCollection::SafeDownCast(output);
  std::size_t nfiles = this->Groups.Files.size();
  pdc->SetNumberOfPartitionedDataSets(static_cast<unsigned int>(nfiles));
  for (std::size_t ii = 0; ii < nfiles / npieces; ++ii)
  {
    std::size_t ff = ii * npieces + piece;
    if (ff > nfiles)
    {
      break;
    }

    vtkNew<vtkCellGridReader> partReader;
    vtkNew<vtkPartitionedDataSet> part;
    partReader->SetFileName(this->Groups.Files[ff].c_str());
    partReader->Update();
    auto* cellgrid = vtkCellGrid::SafeDownCast(partReader->GetOutputDataObject(0));
    if (!cellgrid)
    {
      continue;
    }
    // Downselect cell types and attributes to the enabled set.
    for (const auto& typeToken : cellgrid->CellTypeArray())
    {
      if (!this->CellTypeSelection->ArrayIsEnabled(typeToken.Data().c_str()))
      {
        cellgrid->RemoveCellMetadata(cellgrid->GetCellType(typeToken));
        vtkLog(TRACE, "    Disabling " << typeToken.Data());
      }
    }
    for (const auto& attributeId : cellgrid->GetCellAttributeIds())
    {
      auto* attribute = cellgrid->GetCellAttributeById(attributeId);
      if (!attribute)
      {
        continue;
      }
      if (!this->CellAttributeSelection->ArrayIsEnabled(attribute->GetName().Data().c_str()))
      {
        vtkLog(TRACE, "    Disabling " << attribute->GetName().Data());
        cellgrid->RemoveCellAttribute(attribute);
      }
    }
    part->SetNumberOfPartitions(1);
    part->SetPartition(0, cellgrid);
    pdc->SetPartitionedDataSet(static_cast<unsigned int>(ff), part);
  }
  return 1;
}

int vtkCompositeCellGridReader::ReadPoints(
  int piece, int npieces, int nghosts, int timestep, vtkDataObject* output)
{
  (void)piece;
  (void)npieces;
  (void)nghosts;
  (void)timestep;
  (void)output;
  vtkLogScopeF(TRACE, "ReadPoints");
  return 1;
}

int vtkCompositeCellGridReader::ReadArrays(
  int piece, int npieces, int nghosts, int timestep, vtkDataObject* output)
{
  (void)piece;
  (void)npieces;
  (void)nghosts;
  (void)timestep;
  (void)output;
  vtkLogScopeF(TRACE, "ReadArrays");
  return 1;
}

struct vtkCompositeCellGridReader::MetadataGuard
{
  MetadataGuard(vtkCompositeCellGridReader* self)
    : Self(self)
  {
  }

  /// When destroyed, copy the buffered changes to the reader.
  ~MetadataGuard()
  {
    if (this->Self)
    {
      bool didUpdate = false;
      auto cellTypeMTime = this->Self->GetCellTypeSelection()->GetMTime();
      auto cellAttributeMTime = this->Self->GetCellAttributeSelection()->GetMTime();
      this->Self->GetCellTypeSelection()->CopySelections(this->CellTypeSelection);
      this->Self->GetCellAttributeSelection()->CopySelections(this->CellAttributeSelection);
      didUpdate |= (cellTypeMTime < this->Self->GetCellTypeSelection()->GetMTime());
      didUpdate |= (cellAttributeMTime < this->Self->GetCellAttributeSelection()->GetMTime());
      if (this->Self->Groups.Files != this->Group.Files)
      {
        this->Self->Groups = this->Group;
        didUpdate = true;
      }
      if (didUpdate)
      {
        this->Self->MetadataTime.Modified();
      }
    }
  }

  vtkCompositeCellGridReader* Self{ nullptr };
  vtkCompositeCellGridReader::FileGroup Group;
  vtkNew<vtkDataArraySelection> CellTypeSelection;
  vtkNew<vtkDataArraySelection> CellAttributeSelection;
};

bool vtkCompositeCellGridReader::UpdateMetadata()
{
  if (this->MetadataTime.GetMTime() >= this->GetMTime())
  {
    return false;
  }

  MetadataGuard meta(this);
  if (!this->FileName)
  { // Make sure we have a file to read.
    return false;
  }

  // All files listed in this->FileName must be absolute or are assumed
  // relative to the location of this->FileName. Get the absolute path
  // for this->FileName so we can turn relative paths into absolute ones.
  std::string err;
  auto realFileName = vtksys::SystemTools::GetRealPath(this->FileName, &err);
  if (!err.empty())
  {
    vtkErrorMacro("Could not determine location of \"" << this->FileName << "\".");
    return false;
  }
  std::vector<std::string> pathParts;
  vtksys::SystemTools::SplitPath(realFileName, pathParts);
  if (pathParts.size() < 2)
  {
    vtkErrorMacro("Could not determine parent directory of \"" << this->FileName << "\".");
    return false;
  }
  pathParts.pop_back(); // Drop the filename so we are left with the parent directory.

  // Check the file's validity.
  std::ifstream file(this->FileName);
  if (!file.good())
  {
    vtkErrorMacro("Cannot read file \"" << this->FileName << "\".");
    return false;
  }

  // Read the file into nlohmann json.
  nlohmann::json jj;
  try
  {
    jj = nlohmann::json::parse(file);
  }
  catch (...)
  {
    vtkErrorMacro("Cannot parse file \"" << this->FileName << "\".");
    return false;
  }

  vtkLogScopeF(TRACE, "UpdateMetadata");
  bool isComposite = true;
  auto jtype = jj.find("data-type");
  if (jtype == jj.end())
  {
    vtkErrorMacro("Data type is missing.");
    return false;
  }
  auto dtype = jtype->get<std::string>();
  isComposite = (dtype == "composite");
  if (!isComposite && dtype != "cell-grid")
  {
    vtkErrorMacro("Data type \"" << dtype << "\" is unsupported.");
    return false;
  }

  // If we are given a "leaf" type file as input, we should read it
  // and present a single block with its data as our output.
  if (!isComposite)
  {
    // This is horrible, but we need to find the cell-types and
    // cell-attributes inside each file, which means parsing it.
    vtkNew<vtkCellGridReader> reader;
    reader->SetFileName(this->FileName);
    reader->Update();
    auto* grid = vtkCellGrid::SafeDownCast(reader->GetOutputDataObject(0));
    if (!grid || grid->GetNumberOfCells() == 0)
    {
      vtkErrorMacro("Unsupported file \"" << this->FileName << "\". Skipping.");
      return false;
    }
    meta.Group.Files.emplace_back(this->FileName);
    for (const auto& typeToken : grid->CellTypeArray())
    {
      const char* cellTypeName = typeToken.Data().c_str();
      meta.CellTypeSelection->AddArray(cellTypeName);
      if (this->GetCellTypeSelection()->ArrayExists(cellTypeName) &&
        !this->GetCellTypeSelection()->ArrayIsEnabled(cellTypeName))
      {
        meta.CellTypeSelection->DisableArray(cellTypeName);
      }
    }
    for (const auto& attributeId : grid->GetCellAttributeIds())
    {
      auto* cellAtt = grid->GetCellAttributeById(attributeId);
      const char* attName = cellAtt->GetName().Data().c_str();
      meta.CellAttributeSelection->AddArray(attName);
      vtkLog(TRACE, "    Adding " << attName);
      if (this->GetCellAttributeSelection()->ArrayExists(attName) &&
        !this->GetCellAttributeSelection()->ArrayIsEnabled(attName))
      {
        vtkLog(TRACE, "      Disabling " << attName);
        meta.CellAttributeSelection->DisableArray(attName);
      }
    }
    return true;
  }

  auto jArrayGroup = jj.find("group");
  if (jArrayGroup == jj.end() || !jArrayGroup->is_object())
  {
    vtkErrorMacro("Missing group section.");
    return false;
  }

  auto jGroupType = jArrayGroup->find("group-type");
  if (jGroupType == jArrayGroup->end() || jGroupType->get<std::string>() != "collection")
  {
    vtkErrorMacro("Missing or unsupported group-type inside group specifier.");
    return false;
  }

  auto jFileList = jArrayGroup->find("files");
  if (jFileList == jArrayGroup->end() || !jFileList->is_array())
  {
    vtkErrorMacro("Missing files section in group specification.");
    return false;
  }

  // Add each file's cell types and attributes to "meta" metadata-buffer.
  for (const auto& jPath : *jFileList)
  {
    // meta.Group.Files = jFileList.get<std::vector<std::string>>();
    auto path = jPath.get<std::string>();
    if (!vtksys::SystemTools::FileIsFullPath(path))
    {
      std::vector<std::string> location = pathParts;
      location.push_back(path);
      auto newPath = vtksys::SystemTools::JoinPath(location);
      vtkLog(TRACE, "  Expanding \"" << path << "\" to \"" << newPath << "\"");
      path = newPath;
    }
    std::ifstream groupData(path.c_str());
    if (!groupData.good())
    {
      vtkErrorMacro("Cannot read file \"" << path << "\".");
      return false;
    }

    // This is horrible, but we need to find the cell-types and
    // cell-attributes inside each file, which means parsing it.
    vtkNew<vtkCellGridReader> reader;
    reader->SetFileName(path.c_str());
    reader->Update();
    auto* grid = vtkCellGrid::SafeDownCast(reader->GetOutputDataObject(0));
    if (!grid || grid->GetNumberOfCells() == 0)
    {
      vtkErrorMacro("Empty or missing file \"" << path << "\". Skipping.");
      continue;
    }
    meta.Group.Files.push_back(path);
    for (const auto& typeToken : grid->CellTypeArray())
    {
      const char* cellTypeName = typeToken.Data().c_str();
      meta.CellTypeSelection->AddArray(cellTypeName);
      if (this->GetCellTypeSelection()->ArrayExists(cellTypeName) &&
        !this->GetCellTypeSelection()->ArrayIsEnabled(cellTypeName))
      {
        meta.CellTypeSelection->DisableArray(cellTypeName);
      }
    }
    if (meta.Group.Files.size() == 1)
    {
      for (const auto& attributeId : grid->GetCellAttributeIds())
      {
        auto* cellAtt = grid->GetCellAttributeById(attributeId);
        const char* attName = cellAtt->GetName().Data().c_str();
        meta.CellAttributeSelection->AddArray(attName);
        vtkLog(TRACE, "    Adding " << attName);
        if (this->GetCellAttributeSelection()->ArrayExists(attName) &&
          !this->GetCellAttributeSelection()->ArrayIsEnabled(attName))
        {
          vtkLog(TRACE, "      Disabling " << attName);
          meta.CellAttributeSelection->DisableArray(attName);
        }
      }
    }
    else
    {
      // Subtract any attributes not present in the current grid.
      int nn = meta.CellAttributeSelection->GetNumberOfArrays();
      for (int ii = 0; ii < nn; ++ii)
      {
        const char* arrayName = meta.CellAttributeSelection->GetArrayName(ii);
        if (!arrayName)
        {
          continue;
        }
        auto* cellAtt = grid->GetCellAttributeByName(arrayName);
        // TODO: Add more checks that attributes with the same name
        //       have the same hash (i.e., same number of components,
        //       same space, etc.).
        if (!cellAtt)
        {
          vtkLog(TRACE, "      Disabling " << arrayName << "; it is not in " << path);
          meta.CellAttributeSelection->RemoveArrayByName(arrayName);
          // We just removed an array, so change loop variable and count.
          --nn;
          --ii;
        }
        else
        {
          vtkLog(TRACE, "    Validated " << arrayName << " is present.");
        }
      }
    }
  }

  return true;
}

VTK_ABI_NAMESPACE_END
