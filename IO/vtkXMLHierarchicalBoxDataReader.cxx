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

vtkCxxRevisionMacro(vtkXMLHierarchicalBoxDataReader, "1.2");
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

  hb->GenerateVisibilityArrays();
}

//----------------------------------------------------------------------------
void vtkXMLHierarchicalBoxDataReader::HandleBlock(vtkXMLDataElement* ds,
                                                  int level, int dsId, 
                                                  vtkHierarchicalDataSet* output,
                                                  vtkDataSet* data)
{
  vtkUniformGrid* ugrid = vtkUniformGrid::SafeDownCast(data);
  if (!ugrid)
    {
    vtkErrorMacro("HierarchicalBoxDataSet can only contain uniform grids."
                  << " The file contains: " << data->GetClassName()
                  << "dataset. Ignoring block.");
    }
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
}

