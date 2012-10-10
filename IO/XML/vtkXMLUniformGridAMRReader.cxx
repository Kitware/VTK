/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLUniformGridAMRReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLUniformGridAMRReader.h"

#include "vtkAMRBox.h"
#include "vtkAMRInformation.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataObjectTypes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
#include "vtkXMLDataElement.h"

#include <cassert>
#include <vector>

namespace
{
  // Data type used to store a 3-tuple of doubles.
  template <class T, int N> class vtkTuple
    {
  public:
    T Data[N];
    vtkTuple()
      {
      for (int cc=0; cc < N; cc++)
        {
        this->Data[cc] = 0;
        }
      }
    operator T* ()
      {
      return this->Data;
      }
    };

  typedef vtkTuple<double, 3> vtkSpacingType;

  // Helper routine to parse the XML to collect information about the AMR.
  bool vtkReadMetaData(
    vtkXMLDataElement* ePrimary,
    std::vector<unsigned int> &blocks_per_level,
    std::vector<vtkSpacingType> &level_spacing,
    std::vector<std::vector<vtkAMRBox> > &amr_boxes)
    {
    unsigned int numElems = ePrimary->GetNumberOfNestedElements();
    for (unsigned int cc=0; cc < numElems; cc++)
      {
      vtkXMLDataElement* blockXML = ePrimary->GetNestedElement(cc);
      if (!blockXML || !blockXML->GetName() ||
        strcmp(blockXML->GetName(), "Block") != 0)
        {
        continue;
        }

      int level = 0;
      if (!blockXML->GetScalarAttribute("level", level))
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
        blocks_per_level.resize(level+1, 0);
        level_spacing.resize(level+1);
        amr_boxes.resize(level+1);
        }

      double spacing[3];
      if (blockXML->GetVectorAttribute("spacing", 3, spacing))
        {
        level_spacing[level][0] = spacing[0];
        level_spacing[level][1] = spacing[1];
        level_spacing[level][2] = spacing[2];
        }

      // now read the <DataSet/> elements for boxes and counting the number of
      // nodes per level.
      int numDatasets = blockXML->GetNumberOfNestedElements();
      for (int kk=0; kk < numDatasets; kk++)
        {
        vtkXMLDataElement* datasetXML = blockXML->GetNestedElement(kk);
        if (!datasetXML || !datasetXML->GetName() ||
          strcmp(datasetXML->GetName(), "DataSet") != 0)
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
          blocks_per_level[level] = index+1;
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
    return true;
    }

  bool vtkReadMetaData(vtkXMLDataElement* ePrimary,
    std::vector<unsigned int> &blocks_per_level)
    {
    std::vector<vtkSpacingType> spacings;
    std::vector<std::vector<vtkAMRBox> > amr_boxes;
    return vtkReadMetaData(ePrimary, blocks_per_level, spacings, amr_boxes);
    }
}

vtkStandardNewMacro(vtkXMLUniformGridAMRReader);
//----------------------------------------------------------------------------
vtkXMLUniformGridAMRReader::vtkXMLUniformGridAMRReader()
{
  this->OutputDataType = NULL;
  this->MaximumLevelsToReadByDefault = 1;
}

//----------------------------------------------------------------------------
vtkXMLUniformGridAMRReader::~vtkXMLUniformGridAMRReader()
{
  this->SetOutputDataType(NULL);
}

//----------------------------------------------------------------------------
void vtkXMLUniformGridAMRReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MaximumLevelsToReadByDefault: " <<
    this->MaximumLevelsToReadByDefault << endl;
}

//----------------------------------------------------------------------------
const char* vtkXMLUniformGridAMRReader::GetDataSetName()
{
  if (!this->OutputDataType)
    {
    vtkWarningMacro("We haven't determine a valid output type yet.");
    return "vtkUniformGridAMR";
    }

  return this->OutputDataType;
}

//----------------------------------------------------------------------------
int vtkXMLUniformGridAMRReader::CanReadFileWithDataType(const char* dsname)
{
  return (dsname && (
          strcmp(dsname, "vtkOverlappingAMR") == 0 ||
          strcmp(dsname, "vtkNonOverlappingAMR") == 0 ||
          strcmp(dsname, "vtkHierarchicalBoxDataSet") == 0))? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkXMLUniformGridAMRReader::ReadVTKFile(vtkXMLDataElement* eVTKFile)
{
  // this->Superclass::ReadVTKFile(..) calls this->GetDataSetName().
  // GetDataSetName() needs to know the data type we're reading and hence it's
  // essential to read the "type" before calling the superclass' method.

  // NOTE: eVTKFile maybe totally invalid, so proceed with caution.
  const char* type = eVTKFile->GetAttribute("type");
  if (type == NULL ||
    (strcmp(type, "vtkHierarchicalBoxDataSet") != 0 &&
     strcmp(type, "vtkOverlappingAMR") != 0 &&
     strcmp(type, "vtkNonOverlappingAMR") != 0))
    {
    vtkErrorMacro(
      "Invalid 'type' specified in the file: " << (type? type : "(none)"));
    return 0;
    }

  this->SetOutputDataType(type);
  return this->Superclass::ReadVTKFile(eVTKFile);
}

//----------------------------------------------------------------------------
int vtkXMLUniformGridAMRReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
    {
    return 0;
    }

  if (this->GetFileMajorVersion() != 1 ||
    this->GetFileMinorVersion() != 1)
    {
    // for old files, we don't support providing meta-data for
    // RequestInformation() pass.
    this->Metadata = NULL;
    return 1;
    }

  if (strcmp(ePrimary->GetName(), "vtkNonOverlappingAMR") == 0)
    {
    // this is a non-overlapping AMR. We don't have meta-data for
    // non-overlapping AMRs.
    this->Metadata = NULL;
    return 1;
    }

  // Read the xml to build the metadata.
  this->Metadata = vtkSmartPointer<vtkOverlappingAMR>::New();

  // iterate over the XML to fill up the AMRInformation with meta-data.
  std::vector<unsigned int> blocks_per_level;
  std::vector<vtkSpacingType> level_spacing;
  std::vector<std::vector<vtkAMRBox> > amr_boxes;
  vtkReadMetaData(ePrimary, blocks_per_level, level_spacing, amr_boxes);

  if (blocks_per_level.size() > 0)
    {
    // initialize vtkAMRInformation.
    this->Metadata->Initialize(
      static_cast<int>(blocks_per_level.size()),
      reinterpret_cast<int*>(&blocks_per_level[0]));

    double origin[3] = {0, 0, 0};
    if (!ePrimary->GetVectorAttribute("origin", 3, origin))
      {
      vtkWarningMacro("Missing 'origin'. Using (0, 0, 0).");
      }
    this->Metadata->SetOrigin(origin);

    const char* grid_description = ePrimary->GetAttribute("grid_description");
    int iGridDescription = VTK_XYZ_GRID;
    if (grid_description && strcmp(grid_description, "XY") == 0)
      {
      iGridDescription = VTK_XY_PLANE;
      }
    else if (grid_description && strcmp(grid_description, "YZ") == 0)
      {
      iGridDescription = VTK_YZ_PLANE;
      }
    else if (grid_description && strcmp(grid_description, "XZ") == 0)
      {
      iGridDescription = VTK_XZ_PLANE;
      }
    this->Metadata->SetGridDescription(iGridDescription);

    // pass refinement ratios.
    for (size_t cc=0; cc < level_spacing.size(); cc++)
      {
      this->Metadata->GetAMRInfo()->SetSpacing(
        static_cast<unsigned int>(cc), level_spacing[cc]);
      }
    //  pass amr boxes.
    for (size_t level=0; level < amr_boxes.size(); level++)
      {
      for (size_t index=0; index < amr_boxes[level].size(); index++)
        {
        const vtkAMRBox& box = amr_boxes[level][index];
        if (!box.Empty())
          {
          this->Metadata->GetAMRInfo()->SetAMRBox(
            static_cast<unsigned int>(level),
            static_cast<unsigned int>(index), box);
          }
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLUniformGridAMRReader::RequestDataObject(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if (!this->ReadXMLInformation())
    {
    return 0;
    }

  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);
  if (!output || !output->IsA(this->OutputDataType))
    {
    vtkDataObject* newDO = vtkDataObjectTypes::NewDataObject(this->OutputDataType);
    if (newDO)
      {
      outputVector->GetInformationObject(0)->Set(
        vtkDataObject::DATA_OBJECT(), newDO);
      newDO->FastDelete();
      return 1;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLUniformGridAMRReader::RequestInformation(vtkInformation *request,
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
    {
    return 0;
    }

  if (this->Metadata)
    {
    outputVector->GetInformationObject(0)->Set(
      vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA(),
      this->Metadata);
    }
  else
    {
    outputVector->GetInformationObject(0)->Remove(
      vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA());
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLUniformGridAMRReader::ReadComposite(vtkXMLDataElement* element,
  vtkCompositeDataSet* composite, const char* filePath,
  unsigned int &dataSetIndex)
{
  vtkUniformGridAMR* amr = vtkUniformGridAMR::SafeDownCast(composite);
  if (!amr)
    {
    vtkErrorMacro("Dataset must be a vtkUniformGridAMR.");
    return;
    }

  if (this->GetFileMajorVersion() != 1 ||
    this->GetFileMinorVersion() != 1)
    {
    vtkErrorMacro(
      "Version not supported. Use vtkXMLHierarchicalBoxDataReader instead.");
    return;
    }

  vtkInformation* outinfo = this->GetCurrentOutputInformation();
  bool has_block_requests =
    outinfo->Has(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES()) != 0;

  vtkOverlappingAMR* oamr = vtkOverlappingAMR::SafeDownCast(amr);
  vtkNonOverlappingAMR* noamr = vtkNonOverlappingAMR::SafeDownCast(amr);
  assert(oamr != NULL || noamr != NULL);

  if (oamr)
    {
    // we don;t have the parse the structure. Just pass the inform from
    // this->Metadata.
    oamr->SetAMRInfo(this->Metadata->GetAMRInfo());
    }
  else if (noamr)
    {
    // We process the XML to collect information about the structure.
    std::vector<unsigned int> blocks_per_level;
    vtkReadMetaData(element, blocks_per_level);
    noamr->Initialize(
      static_cast<int>(blocks_per_level.size()),
      reinterpret_cast<int*>(&blocks_per_level[0]));
    }

  // Now, simply scan the xml for dataset elements and read them as needed.

  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkXMLDataElement* blockXML = element->GetNestedElement(cc);
    if (!blockXML || !blockXML->GetName() ||
      strcmp(blockXML->GetName(), "Block") != 0)
      {
      continue;
      }

    int level = 0;
    if (!blockXML->GetScalarAttribute("level", level) || level < 0)
      {
      continue;
      }

    // now read the <DataSet/> elements for boxes and counting the number of
    // nodes per level.
    int numDatasets = blockXML->GetNumberOfNestedElements();
    for (int kk=0; kk < numDatasets; kk++)
      {
      vtkXMLDataElement* datasetXML = blockXML->GetNestedElement(kk);
      if (!datasetXML || !datasetXML->GetName() ||
        strcmp(datasetXML->GetName(), "DataSet") != 0)
        {
        continue;
        }

      int index = 0;
      if (!datasetXML->GetScalarAttribute("index", index) || index < 0)
        {
        continue;
        }

      if (this->ShouldReadDataSet(dataSetIndex))
        {
        // if has_block_requests==false, then we don't read any blocks greater
        // than the MaximumLevelsToReadByDefault.
        if (has_block_requests == false &&
            this->MaximumLevelsToReadByDefault > 0 &&
            static_cast<unsigned int>(level) >= this->MaximumLevelsToReadByDefault)
          {
          // don't actually read the data.
          }
        else
          {
          vtkSmartPointer<vtkDataSet> ds;
          ds.TakeReference(this->ReadDataset(datasetXML, filePath));
          if (ds && !ds->IsA("vtkUniformGrid"))
            {
            vtkErrorMacro("vtkUniformGridAMR can only contain vtkUniformGrids.");
            }
          else
            {
            amr->SetDataSet(
              static_cast<unsigned int>(level), static_cast<unsigned int>(index),
              vtkUniformGrid::SafeDownCast(ds.GetPointer()));
            }
          }
        }
      dataSetIndex++;
      }
    }

  //Blanking is not done right now.
  //This information should be in the file.vtkAMRUtilities::BlankCells(hbox,NULL);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLUniformGridAMRReader::ReadDataset(
  vtkXMLDataElement* xmlElem, const char* filePath)
{
  vtkDataSet* ds = this->Superclass::ReadDataset(xmlElem, filePath);
  if (ds && ds->IsA("vtkImageData"))
    {
    // Convert vtkImageData to vtkUniformGrid as needed by
    // vtkHierarchicalBoxDataSet.
    vtkUniformGrid* ug = vtkUniformGrid::New();
    ug->ShallowCopy(ds);
    ds->Delete();
    return ug;
    }
  return ds;
}
