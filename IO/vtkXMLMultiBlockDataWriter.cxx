/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLMultiBlockDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLMultiBlockDataWriter.h"

#include "vtkCompositeDataIterator.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLMultiBlockDataWriter);
vtkCxxRevisionMacro(vtkXMLMultiBlockDataWriter, "1.1");
//----------------------------------------------------------------------------
vtkXMLMultiBlockDataWriter::vtkXMLMultiBlockDataWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLMultiBlockDataWriter::~vtkXMLMultiBlockDataWriter()
{
}

//----------------------------------------------------------------------------
int vtkXMLMultiBlockDataWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLMultiBlockDataWriter::WriteComposite(vtkCompositeDataSet* compositeData, 
    vtkXMLDataElement* parent, int &writerIdx)
{
  if (!compositeData->IsA("vtkMultiBlockDataSet"))
    {
    vtkErrorMacro("vtkMultiBlockDataSet can only contain other "
      "vtkMultiBlockDataSet dataset nodes");
    return 0;
    }

  // Write each input.
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(compositeData->NewIterator());
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();

  int index = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem(), index++)
    {
    vtkDataObject* curDO = iter->GetCurrentDataObject();
    if (curDO->IsA("vtkCompositeDataSet"))
      {
      // Write a composite dataset.
      vtkCompositeDataSet* curCD = vtkCompositeDataSet::SafeDownCast(curDO);
      vtkXMLDataElement* block = vtkXMLDataElement::New();
      block->SetName("Block");
      block->SetIntAttribute("index", index);
      if (!this->WriteComposite(curCD, block, writerIdx))
        {
        return 0;
        }
      parent->AddNestedElement(block);
      block->Delete();
      }
    else
      {
      vtkXMLDataElement* datasetXML = vtkXMLDataElement::New();
      datasetXML->SetName("DataSet");
      datasetXML->SetIntAttribute("index", index);
      if (!this->WriteNonCompositeData( curDO, datasetXML, writerIdx))
        {
        datasetXML->Delete();
        return 0;
        }
      parent->AddNestedElement(datasetXML);
      datasetXML->Delete();
      }
    }
  return 1;
}


//----------------------------------------------------------------------------
void vtkXMLMultiBlockDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

