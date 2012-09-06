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

#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkAMRInformation.h"
#include <limits>
#include <cassert>
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

  //Read in all meta data to initialize AMR
  //Also read in level 0 data to compute origin
  std::vector<int> blocksPerLevel;
  double origin[3];
  origin[0] = origin[1] = origin[2] = std::numeric_limits<double>::max();

  int description=-1;
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
      vtkSmartPointer<vtkUniformGrid> grid = 0;
      if (level== 0 && this->ShouldReadDataSet(dataSetIndex))
        {
        vtkDataSet* ds = this->ReadDataset(childXML, filePath);
        if (ds && !ds->IsA("vtkUniformGrid"))
          {
          vtkErrorMacro("vtkHierarchicalBoxDataSet can only contain "
            "vtkUniformGrid.");
          continue;
          }
        grid.TakeReference(vtkUniformGrid::SafeDownCast(ds));
        }
      for(int i= static_cast<int>(blocksPerLevel.size()); i<= level; i++)
        {
        blocksPerLevel.push_back(0);
        }
      blocksPerLevel[level]++;
      if(grid)
        {
        description = grid->GetGridDescription();
        double* gridOrigin = grid->GetOrigin();
        for(int i=0; i<3; i++)
          {
          origin[i] = gridOrigin[i]<origin[i]?  gridOrigin[i] : origin[i];
          }
        }
      }
    }

  hbox->Initialize(static_cast<int>(blocksPerLevel.size()),&blocksPerLevel[0],origin,description);

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
      if(childDS)
        {
        hbox->SetAMRBox(level,index,childDS->GetOrigin(), childDS->GetDimensions(), childDS->GetSpacing());
        hbox->SetDataSet(level, index, childDS);
        }
      }
    dataSetIndex++;
    }
   //Blanking should be contained int the file.
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

      // Note: the dimensionality is auto-detected by the AMR box now
      int dimensionality = 3;
      if (!datasetXML->GetScalarAttribute("dimensionality", dimensionality))
        {
        // default.
        dimensionality = 3;
        }

      if (!datasetXML->GetVectorAttribute("amr_box", 6, box))
        {
        vtkWarningMacro("Missing amr box for level " << level << ",  dataset " << index);
        }


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

      if(childDS)
        {
        hbox->SetDataSet(level, index, childDS);
        }
      else
        {
        vtkWarningMacro("Meta data does not contain spacing information!");
        hbox->GetAMRInfo()->SetAMRBox(level,index,box,NULL);
        }
      dataSetIndex++;
      }
    }
  //Blanking is not done right now.
  //This information should be in the file.vtkAMRUtilities::BlankCells(hbox,NULL);
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

//-----------------------------------------------------------------------------
void vtkXMLHierarchicalBoxDataReader::GetDataSetOrigin(
    vtkHierarchicalBoxDataSet *hbox, double origin[3] )
{
  assert( "pre: hbox dataset is NULL" && (hbox != NULL) );

  if( (hbox->GetNumberOfLevels()==0) || (hbox->GetNumberOfDataSets(0)==0) )
    return;

  origin[0] = origin[1] = origin[2] = std::numeric_limits<double>::max();

  // Note, we only need to check at level 0 since, the grids at
  // level 0 are guaranteed to cover the entire domain. Most datasets
  // will have a single grid at level 0.
  for( unsigned int idx=0; idx < hbox->GetNumberOfDataSets(0); ++idx )
    {

      vtkUniformGrid *gridPtr = hbox->GetDataSet( 0, idx );
      if( gridPtr != NULL )
        {
          double *gridBounds = gridPtr->GetBounds();
          assert( "Failed when accessing grid bounds!" && (gridBounds!=NULL) );

          if( gridBounds[0] < origin[0] )
            origin[0] = gridBounds[0];
          if( gridBounds[2] < origin[1] )
            origin[1] = gridBounds[2];
          if( gridBounds[4] < origin[2] )
            origin[2] = gridBounds[4];
        }

    } // END for all data-sets at level 0
}
