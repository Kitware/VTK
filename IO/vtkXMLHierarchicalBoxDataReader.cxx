/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLHierarchicalBoxDataReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLHierarchicalBoxDataReader.h"

#include "vtkAMRBox.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
#include "vtkXMLDataElement.h"

#include <vector>

vtkStandardNewMacro(vtkXMLHierarchicalBoxDataReader);

//----------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataReader::vtkXMLHierarchicalBoxDataReader()
{
}

//----------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataReader::~vtkXMLHierarchicalBoxDataReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLHierarchicalBoxDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* vtkXMLHierarchicalBoxDataReader::GetDataSetName()
{
  return "vtkHierarchicalBoxDataSet";
}

//----------------------------------------------------------------------------
int vtkXMLHierarchicalBoxDataReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHierarchicalBoxDataSet");
  return 1;
}

//----------------------------------------------------------------------------
// This only reads 0.* version files.
void vtkXMLHierarchicalBoxDataReader::ReadVersion0(vtkXMLDataElement* element, 
  vtkCompositeDataSet* composite, const char* filePath, 
  unsigned int &dataSetIndex)
{
  vtkHierarchicalBoxDataSet* hbox = 
    vtkHierarchicalBoxDataSet::SafeDownCast(composite);

  unsigned int numElems = element->GetNumberOfNestedElements();
  unsigned int cc;

  // Read refinement ratios for each level.
  for (cc=0; cc < numElems; cc++)
    {
    vtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName() ||
      strcmp(childXML->GetName(), "RefinementRatio") != 0)
      {
      continue;
      }
    int level=0;
    int refinement_ratio=0;
    if (childXML->GetScalarAttribute("level", level) &&
      childXML->GetScalarAttribute("refinement", refinement_ratio) &&
      refinement_ratio)
      {
      hbox->SetRefinementRatio(level, refinement_ratio);
      }
    }

  // Read uniform grids.
  for (cc=0; cc < numElems; cc++)
    {
    vtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName() ||
      strcmp(childXML->GetName(), "DataSet") != 0)
      {
      continue;
      }
    int level=0;
    int index=0;
    int box[6];

    if (childXML->GetScalarAttribute("group", level) &&
      childXML->GetScalarAttribute("dataset", index) &&
      childXML->GetVectorAttribute("amr_box", 6, box))
      {
      vtkAMRBox amrBox(box);

      vtkSmartPointer<vtkUniformGrid> childDS = 0;
      if (this->ShouldReadDataSet(dataSetIndex))
        {
        vtkDataSet* ds = this->ReadDataset(childXML, filePath);
        if (ds && !ds->IsA("vtkUniformGrid"))
          {
          vtkErrorMacro("vtkHierarchicalBoxDataSet can only contain "
            "vtkUniformGrid.");
          continue;
          }
        childDS.TakeReference(vtkUniformGrid::SafeDownCast(ds));
        }
      hbox->SetDataSet(level, index, amrBox, childDS); 
      }
    dataSetIndex++;
    }
  hbox->GenerateVisibilityArrays();
}

//----------------------------------------------------------------------------
void vtkXMLHierarchicalBoxDataReader::ReadComposite(vtkXMLDataElement* element, 
  vtkCompositeDataSet* composite, const char* filePath, 
  unsigned int &dataSetIndex)
{
  vtkHierarchicalBoxDataSet* hbox = 
    vtkHierarchicalBoxDataSet::SafeDownCast(composite);
  if (!hbox)
    {
    vtkErrorMacro("Dataset must be a vtkHierarchicalBoxDataSet.");
    return;
    }

  if (this->GetFileMajorVersion() < 1)
    {
    // Read legacy file.
    this->ReadVersion0(element, composite, filePath, dataSetIndex);
    return;
    }

  unsigned int maxElems = element->GetNumberOfNestedElements();
  // Iterate over levels.
  for (unsigned int cc=0; cc < maxElems; ++cc)
    {
    vtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName() || 
      strcmp(childXML->GetName(), "Block") != 0)
      {
      continue;
      }

    int level = 0;
    if (!childXML->GetScalarAttribute("level", level))
      {
      level = hbox->GetNumberOfLevels();
      }

    int refinement_ratio = 0;
    if (!childXML->GetScalarAttribute("refinement_ratio", refinement_ratio))
      {
      vtkWarningMacro("Missing refinement_ratio for level " << level);
      }
    if (refinement_ratio >=2)
      {
      hbox->SetRefinementRatio(level, refinement_ratio);
      }

    // Now read the datasets within this level.
    unsigned int numDatasets = childXML->GetNumberOfNestedElements();
    for (unsigned int kk=0; kk < numDatasets; ++kk)
      {
      vtkXMLDataElement* datasetXML = childXML->GetNestedElement(kk);
      if (!datasetXML || !datasetXML->GetName() || 
        strcmp(datasetXML->GetName(), "DataSet") != 0)
        {
        continue;
        }
      int index = 0;
      if (!datasetXML->GetScalarAttribute("index", index))
        {
        index = hbox->GetNumberOfDataSets(level);
        }
      int box[6];
      vtkAMRBox amrBox;
      if (datasetXML->GetVectorAttribute("amr_box", 6, box))
        {
        amrBox.SetDimensions(box[0],box[2],box[4],box[1],box[3],box[5]);
        }
      else
        {
        vtkWarningMacro("Missing amr box for level " << level << ",  dataset " << index);
        }

      int dimensionality = 3;
      if (!datasetXML->GetScalarAttribute("dimensionality", dimensionality))
        {
        // default.
        dimensionality = 3;
        }
      amrBox.SetDimensionality(dimensionality);

      vtkSmartPointer<vtkUniformGrid> childDS = 0;
      if (this->ShouldReadDataSet(dataSetIndex))
        {
        vtkDataSet* ds = this->ReadDataset(datasetXML, filePath);
        if (ds && !ds->IsA("vtkUniformGrid"))
          {
          vtkErrorMacro("vtkHierarchicalBoxDataSet can only contain "
            "vtkUniformGrid.");
          continue;
          }
        childDS.TakeReference(vtkUniformGrid::SafeDownCast(ds));
        }
      hbox->SetDataSet(level, index, amrBox, childDS); 
      dataSetIndex++;
      }
    }

  hbox->GenerateVisibilityArrays();
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLHierarchicalBoxDataReader::ReadDataset(
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
