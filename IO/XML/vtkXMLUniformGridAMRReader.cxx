// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLUniformGridAMRReader.h"

#include "vtkAMRBox.h"
#include "vtkAMRUtilities.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObjectTypes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkOverlappingAMRMetaData.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkTuple.h"
#include "vtkUniformGrid.h"
#include "vtkXMLDataElement.h"

#include <cassert>
#include <vector>

namespace
{
using vtkSpacingType = vtkTuple<double, 3>;

// Helper routine to parse the XML to collect information about the AMR.
void ReadMetaData(vtkXMLDataElement* ePrimary, std::vector<unsigned int>& blocks_per_level,
  std::vector<vtkSpacingType>& level_spacing, std::vector<std::vector<vtkAMRBox>>& amr_boxes)
{
  unsigned int numElems = ePrimary->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    vtkXMLDataElement* levelXML = ePrimary->GetNestedElement(cc);
    if (!levelXML || !levelXML->GetName() || strcmp(levelXML->GetName(), "Block") != 0)
    {
      continue;
    }

    int level = 0;
    if (!levelXML->GetScalarAttribute("level", level))
    {
      vtkGenericWarningMacro("Missing 'level' on 'Block' element in XML. Skipping");
      continue;
    }
    if (level < 0)
    {
      // sanity check.
      continue;
    }
    if (blocks_per_level.size() <= static_cast<size_t>(level))
    {
      blocks_per_level.resize(level + 1, 0);
      level_spacing.resize(level + 1);
      amr_boxes.resize(level + 1);
    }

    double spacing[3];
    if (levelXML->GetVectorAttribute("spacing", 3, spacing))
    {
      level_spacing[level][0] = spacing[0];
      level_spacing[level][1] = spacing[1];
      level_spacing[level][2] = spacing[2];
    }

    // now read the <DataSet/> elements for boxes and counting the number of
    // nodes per level.
    int numDatasets = levelXML->GetNumberOfNestedElements();
    for (int kk = 0; kk < numDatasets; ++kk)
    {
      vtkXMLDataElement* datasetXML = levelXML->GetNestedElement(kk);
      if (!datasetXML || !datasetXML->GetName() || strcmp(datasetXML->GetName(), "DataSet") != 0)
      {
        continue;
      }

      int index = 0;
      if (!datasetXML->GetScalarAttribute("index", index))
      {
        vtkGenericWarningMacro("Missing 'index' on 'DataSet' element in XML. Skipping");
        continue;
      }
      if (index >= static_cast<int>(blocks_per_level[level]))
      {
        blocks_per_level[level] = index + 1;
      }
      if (static_cast<size_t>(index) >= amr_boxes[level].size())
      {
        amr_boxes[level].resize(index + 1);
      }
      int box[6];
      // note: amr-box is not provided for non-overlapping AMR.
      if (!datasetXML->GetVectorAttribute("amr_box", 6, box))
      {
        continue;
      }
      // box is xLo, xHi, yLo, yHi, zLo, zHi.
      amr_boxes[level][index] = vtkAMRBox(box);
    }
  }
}
}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLUniformGridAMRReader);
//------------------------------------------------------------------------------
vtkXMLUniformGridAMRReader::vtkXMLUniformGridAMRReader() = default;

//------------------------------------------------------------------------------
vtkXMLUniformGridAMRReader::~vtkXMLUniformGridAMRReader()
{
  this->SetOutputDataType(nullptr);
}

//------------------------------------------------------------------------------
void vtkXMLUniformGridAMRReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MaximumLevelsToReadByDefault: " << this->MaximumLevelsToReadByDefault << endl;
}

//------------------------------------------------------------------------------
const char* vtkXMLUniformGridAMRReader::GetDataSetName()
{
  if (!this->OutputDataType)
  {
    vtkWarningMacro("We haven't determine a valid output type yet.");
    return "vtkAMRDataObject";
  }

  return this->OutputDataType;
}

//------------------------------------------------------------------------------
int vtkXMLUniformGridAMRReader::CanReadFileWithDataType(const char* dsname)
{
  return (dsname &&
           (strcmp(dsname, "vtkOverlappingAMR") == 0 ||
             strcmp(dsname, "vtkNonOverlappingAMR") == 0 ||
             strcmp(dsname, "vtkHierarchicalBoxDataSet") == 0))
    ? 1
    : 0;
}

//------------------------------------------------------------------------------
int vtkXMLUniformGridAMRReader::ReadVTKFile(vtkXMLDataElement* eVTKFile)
{
  // this->Superclass::ReadVTKFile(..) calls this->GetDataSetName().
  // GetDataSetName() needs to know the data type we're reading and hence it's
  // essential to read the "type" before calling the superclass' method.

  // NOTE: eVTKFile maybe totally invalid, so proceed with caution.
  const char* type = eVTKFile->GetAttribute("type");
  if (type == nullptr ||
    (strcmp(type, "vtkHierarchicalBoxDataSet") != 0 && strcmp(type, "vtkOverlappingAMR") != 0 &&
      strcmp(type, "vtkNonOverlappingAMR") != 0))
  {
    vtkErrorMacro("Invalid 'type' specified in the file: " << (type ? type : "(none)"));
    return 0;
  }

  this->SetOutputDataType(type);
  return this->Superclass::ReadVTKFile(eVTKFile);
}

//------------------------------------------------------------------------------
void vtkXMLUniformGridAMRReader::CreateMetaData(vtkXMLDataElement* ePrimary)
{
  if (this->GetFileMajorVersion() == -1 && this->GetFileMinorVersion() == -1)
  {
    // for old files, we don't support providing meta-data for
    // RequestInformation() pass.
    this->Metadata = nullptr;
    return;
  }

  // iterate over the XML to fill up the AMRInformation with meta-data.
  std::vector<unsigned int> blocksPerLevel;
  std::vector<vtkSpacingType> level_spacing;
  std::vector<std::vector<vtkAMRBox>> amr_boxes;

  ReadMetaData(ePrimary, blocksPerLevel, level_spacing, amr_boxes);

  // Handle Non overlapping AMR first
  vtkSmartPointer<vtkAMRDataObject> amrMetaData;
  if (strcmp(ePrimary->GetName(), "vtkNonOverlappingAMR") == 0)
  {
    amrMetaData = vtkSmartPointer<vtkNonOverlappingAMR>::New();
    amrMetaData->Initialize(blocksPerLevel);
  }
  else
  {
    assert(strcmp(ePrimary->GetName(), "vtkOverlappingAMR") == 0 ||
      strcmp(ePrimary->GetName(), "vtkHierarchicalBoxDataSet") == 0);

    // the following code path is for vtkOverlappingAMR only;
    auto oAmrMetaData = vtkSmartPointer<vtkOverlappingAMR>::New();
    amrMetaData = oAmrMetaData;
    if (!blocksPerLevel.empty())
    {
      // initialize vtkOverlappingAMRMetaData.
      oAmrMetaData->Initialize(blocksPerLevel);

      double origin[3] = { 0, 0, 0 };
      if (!ePrimary->GetVectorAttribute("origin", 3, origin))
      {
        vtkWarningMacro("Missing 'origin'. Using (0, 0, 0).");
      }
      oAmrMetaData->SetOrigin(origin);

      const char* grid_description = ePrimary->GetAttribute("grid_description");
      int iGridDescription = vtkStructuredData::VTK_STRUCTURED_XYZ_GRID;
      if (grid_description && strcmp(grid_description, "XY") == 0)
      {
        iGridDescription = vtkStructuredData::VTK_STRUCTURED_XY_PLANE;
      }
      else if (grid_description && strcmp(grid_description, "YZ") == 0)
      {
        iGridDescription = vtkStructuredData::VTK_STRUCTURED_YZ_PLANE;
      }
      else if (grid_description && strcmp(grid_description, "XZ") == 0)
      {
        iGridDescription = vtkStructuredData::VTK_STRUCTURED_XZ_PLANE;
      }
      oAmrMetaData->SetGridDescription(iGridDescription);

      // pass refinement ratios.
      for (size_t cc = 0; cc < level_spacing.size(); cc++)
      {
        oAmrMetaData->SetSpacing(static_cast<unsigned int>(cc), level_spacing[cc].GetData());
      }
      //  pass amr boxes.
      for (unsigned int level = 0; level < amr_boxes.size(); ++level)
      {
        for (unsigned int idx = 0; idx < amr_boxes[level].size(); ++idx)
        {
          const vtkAMRBox& box = amr_boxes[level][idx];
          if (!box.Empty())
          {
            oAmrMetaData->SetAMRBox(level, idx, box);
          }
        }
      }
    }
  }
  // set the composite name to Level{level}
  for (unsigned int level = 0; level < amrMetaData->GetNumberOfLevels(); ++level)
  {
    amrMetaData->GetMetaData(level)->Set(
      vtkCompositeDataSet::NAME(), "Level" + vtk::to_string(level));
  }
  // create a map from composite ids to level
  this->CompositeIdToLevel.clear();
  unsigned int compositeIndex = 0;
  this->CompositeIdToLevel[compositeIndex++] = 0; // root node
  for (unsigned int level = 0; level < amrMetaData->GetNumberOfLevels(); ++level)
  {
    for (unsigned int idx = 0; idx < amrMetaData->GetNumberOfBlocks(level); ++idx)
    {
      this->CompositeIdToLevel[compositeIndex++] = level;
    }
  }
  this->Metadata = amrMetaData;
}

//------------------------------------------------------------------------------
int vtkXMLUniformGridAMRReader::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (!this->ReadXMLInformation())
  {
    return 0;
  }

  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);

  std::string type = this->OutputDataType;

  // vtkHierarchicalBoxDataSet is obsolete
  if (type == "vtkHierarchicalBoxDataSet")
  {
    type = "vtkOverlappingAMR";
  }

  if (!output || !output->IsA(type.c_str()))
  {
    if (vtkDataObject* newDO = vtkDataObjectTypes::NewDataObject(type.c_str()))
    {
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), newDO);
      newDO->FastDelete();
      return 1;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
bool vtkXMLUniformGridAMRReader::IsBlockSelected(unsigned int compositeIndex)
{
  return this->HasBlockRequests || this->MaximumLevelsToReadByDefault == 0 ||
    this->CompositeIdToLevel[compositeIndex] < this->MaximumLevelsToReadByDefault;
}

//------------------------------------------------------------------------------
bool vtkXMLUniformGridAMRReader::CanReadDataObject(vtkDataObject* dataObject)
{
  return dataObject ? dataObject->IsA("vtkCartesianGrid") : true;
}

//------------------------------------------------------------------------------
void vtkXMLUniformGridAMRReader::ReadComposite(vtkXMLDataElement* element,
  vtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex)
{
  vtkAMRDataObject* amr = vtkAMRDataObject::SafeDownCast(composite);
  if (!amr)
  {
    vtkErrorMacro("Dataset must be a vtkAMRDataObject.");
    return;
  }

  if (this->GetFileMajorVersion() == -1 && this->GetFileMinorVersion() == -1)
  {
    vtkErrorMacro("Version not supported.");
    return;
  }

  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  this->HasBlockRequests = outInfo->Has(vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS()) != 0;

  this->Superclass::ReadComposite(element, amr, filePath, dataSetIndex);

  auto oamr = vtkOverlappingAMR::SafeDownCast(amr);
  if (oamr && !this->HasBlockRequests)
  {
    // Each piece only holds some of the blocks, but blanking only needs to know which blocks exist
    // and their AMR boxes, and not where they are. Only the levels that are read exist.
    std::vector<int> processMap(oamr->GetNumberOfBlocks(), -1);
    for (unsigned int level = 0; level < oamr->GetNumberOfLevels(); ++level)
    {
      if (this->MaximumLevelsToReadByDefault > 0 && level >= this->MaximumLevelsToReadByDefault)
      {
        break;
      }
      for (unsigned int index = 0; index < oamr->GetNumberOfBlocks(level); ++index)
      {
        processMap[oamr->GetAbsoluteBlockIndex(level, index)] = 0;
      }
    }
    vtkAMRUtilities::BlankCells(oamr, processMap);
  }
}

//------------------------------------------------------------------------------
vtkDataObject* vtkXMLUniformGridAMRReader::ReadDataObject(
  vtkXMLDataElement* xmlElem, const char* filePath)
{
  vtkDataObject* dObj = this->Superclass::ReadDataObject(xmlElem, filePath);
  if (dObj && dObj->IsA("vtkImageData"))
  {
    // Convert vtkImageData to vtkUniformGrid as needed by AMR datatypes
    vtkUniformGrid* ug = vtkUniformGrid::New();
    ug->ShallowCopy(dObj);
    dObj->Delete();
    return ug;
  }
  return dObj;
}
VTK_ABI_NAMESPACE_END
