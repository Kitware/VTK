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

#include <limits>
#include <cassert>

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
      amrBox.SetDimensionality( childDS->GetDataDimension() );
      amrBox.SetGridDescription( childDS->GetGridDescription( ) );
      hbox->SetDataSet(level, index, amrBox, childDS); 
      }
    dataSetIndex++;
    }
  this->SetMetaData( hbox );
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

      // Note: the dimensionality is auto-detected by the AMR box now
      int dimensionality = 3;
      if (!datasetXML->GetScalarAttribute("dimensionality", dimensionality))
        {
        // default.
        dimensionality = 3;
        }
      amrBox.SetDimensionality(dimensionality);

      if (datasetXML->GetVectorAttribute("amr_box", 6, box))
        {
        amrBox.SetDimensions(box[0],box[2],box[4],box[1],box[3],box[5]);
        }
      else
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
      amrBox.SetGridDescription( childDS->GetGridDescription( ) );
//      hbox->SetDataSet( level, index, childDS );
      hbox->SetDataSet(level, index, amrBox, childDS);
      dataSetIndex++;
      }
    }

//  vtkAMRUtilities::GenerateMetaData( hbox, NULL );
  this->SetMetaData( hbox );
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

//-----------------------------------------------------------------------------
void vtkXMLHierarchicalBoxDataReader::SetMetaData(
    vtkHierarchicalBoxDataSet *hbox )
{
  assert( "pre: hbox dataset is NULL" && (hbox != NULL) );

  if( (hbox->GetNumberOfLevels()==0) || (hbox->GetNumberOfDataSets(0)==0) )
      return;

  double origin[3];
  this->GetDataSetOrigin( hbox, origin );

  unsigned int level=0;
  for( ; level < hbox->GetNumberOfLevels(); ++level )
    {
      unsigned int dataIdx=0;
      for( ; dataIdx < hbox->GetNumberOfDataSets(level); ++dataIdx )
        {

          vtkUniformGrid *ug = hbox->GetDataSet( level, dataIdx );
          assert( "pre: NULL dataset encountered" && (ug != NULL) );

          // NOTE: The dimensions for the AMR box are read from the
          // XML, hence, they are not re-computed here!
          vtkAMRBox box;
          hbox->GetMetaData( level, dataIdx, box );
          box.SetDataSetOrigin( origin );
          box.SetGridSpacing( ug->GetSpacing() );
          box.SetBlockId( dataIdx );
          box.SetLevel( level );
          box.SetProcessId( 0 ); // Data is serial!

          hbox->SetMetaData( level, dataIdx, box );
        } // END for all data at the current level
    } // END for all levels

}
