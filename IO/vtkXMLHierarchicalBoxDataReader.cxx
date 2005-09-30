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
#include "vtkUniformGrid.h"
#include "vtkXMLDataElement.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkXMLHierarchicalBoxDataReader, "1.5");
vtkStandardNewMacro(vtkXMLHierarchicalBoxDataReader);

struct vtkXMLHierarchicalBoxDataReaderInternals
{
  vtkstd::vector<vtkXMLDataElement*> Refinements;
};

//----------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataReader::vtkXMLHierarchicalBoxDataReader()
{
  this->Internal = new vtkXMLHierarchicalBoxDataReaderInternals;
}

//----------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataReader::~vtkXMLHierarchicalBoxDataReader()
{
  delete this->Internal;
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
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_TYPE_NAME(), 
            "vtkHierarchicalBoxDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLHierarchicalBoxDataReader::ReadXMLData()
{
  this->Superclass::ReadXMLData();

  vtkExecutive* exec = this->GetExecutive();
  vtkInformation* info = exec->GetOutputInformation(0);

  vtkDataObject* doOutput = 
    info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkHierarchicalBoxDataSet* hb = 
    vtkHierarchicalBoxDataSet::SafeDownCast(doOutput);
  if (!hb)
    {
    return;
    }

  vtkstd::vector<vtkXMLDataElement*>::iterator d;
  for(d=this->Internal->Refinements.begin();
      d != this->Internal->Refinements.end(); ++d)
    {
    vtkXMLDataElement* ds = *d;

    int level = 0;
    int refinement = 0;

    if (!ds->GetScalarAttribute("level", level))
      {
      continue;
      }

    if (ds->GetScalarAttribute("refinement", refinement))
      {
      hb->SetRefinementRatio(level, refinement);
      }
    }

  hb->GenerateVisibilityArrays();
}

//----------------------------------------------------------------------------
int vtkXMLHierarchicalBoxDataReader::ReadPrimaryElement(
  vtkXMLDataElement* ePrimary)
{
  if(!this->Superclass::ReadPrimaryElement(ePrimary)) { return 0; }

  int numNested = ePrimary->GetNumberOfNestedElements();
  int i;
  this->Internal->Refinements.clear();
  for(i=0; i < numNested; ++i)
    {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "RefinementRatio") == 0) 
      { 
      this->Internal->Refinements.push_back(eNested);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLHierarchicalBoxDataReader::HandleDataSet(vtkXMLDataElement* ds,
                                                  int level, int dsId, 
                                                  vtkMultiGroupDataSet* output,
                                                  vtkDataSet* data)
{
  vtkImageData* image = 0;
  if (data)
    {
    image = vtkImageData::SafeDownCast(data);
    if (!image)
      {
      vtkErrorMacro("HierarchicalBoxDataSet can only contain image data."
                    << " The file contains: " << data->GetClassName()
                    << ". Ignoring dataset.");
      }
    }

  vtkUniformGrid* ugrid = vtkUniformGrid::New();
  ugrid->ShallowCopy(image);

  int box[6];
  if (ds->GetVectorAttribute("amr_box", 6, box))
    {
    vtkHierarchicalBoxDataSet* hbds = 
      vtkHierarchicalBoxDataSet::SafeDownCast(output);
    vtkAMRBox abox;
    abox.LoCorner[0] = box[0];
    abox.HiCorner[0] = box[1];
    abox.LoCorner[1] = box[2];
    abox.HiCorner[1] = box[3];
    abox.LoCorner[2] = box[4];
    abox.HiCorner[2] = box[5];
    hbds->SetDataSet(level, dsId, abox, ugrid);
    }
  else
    {
    output->SetDataSet(level, dsId, data);
    }

  ugrid->Delete();
}

