/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHierarchicalBoxDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLHierarchicalBoxDataWriter.h"

#include "vtkAMRBox.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"
#include "vtkUniformGrid.h"

vtkStandardNewMacro(vtkXMLHierarchicalBoxDataWriter);
vtkCxxRevisionMacro(vtkXMLHierarchicalBoxDataWriter, "1.6");
//----------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataWriter::vtkXMLHierarchicalBoxDataWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataWriter::~vtkXMLHierarchicalBoxDataWriter()
{
}

//----------------------------------------------------------------------------
int vtkXMLHierarchicalBoxDataWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLHierarchicalBoxDataWriter::WriteComposite(vtkCompositeDataSet* compositeData, 
    vtkXMLDataElement* parent, int &writerIdx)
{
  vtkHierarchicalBoxDataSet* hboxData = vtkHierarchicalBoxDataSet::SafeDownCast(compositeData);

  unsigned int numLevels = hboxData->GetNumberOfLevels();
  // Iterate over each level.
  for (unsigned int level=0; level < numLevels; level++)
    {
    vtkSmartPointer<vtkXMLDataElement> block = vtkSmartPointer<vtkXMLDataElement>::New();
    block->SetName("Block");
    block->SetIntAttribute("level", level);
    block->SetIntAttribute("refinement_ratio", hboxData->GetRefinementRatio(level));
    unsigned int numDS = hboxData->GetNumberOfDataSets(level);
    for (unsigned int cc=0; cc < numDS; cc++)
      {
      vtkAMRBox box;
      int vec_box[6]={0};
      vtkUniformGrid* ug = hboxData->GetDataSet(level, cc, box);
      vtkSmartPointer<vtkXMLDataElement> datasetXML = 
        vtkSmartPointer<vtkXMLDataElement>::New();
      datasetXML->SetName("DataSet");
      datasetXML->SetIntAttribute("index", cc);
      box.GetDimensions(vec_box);
      datasetXML->SetVectorAttribute("amr_box", 6, vec_box);
      vtkStdString FileName = this->CreatePieceFileName(writerIdx);
      if (!this->WriteNonCompositeData(ug, datasetXML, writerIdx, 
                                       FileName.c_str()))
        {
        return 0;
        } 
      block->AddNestedElement(datasetXML);
      }
    parent->AddNestedElement(block);
    }
  return 1;
}


//----------------------------------------------------------------------------
void vtkXMLHierarchicalBoxDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

