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
  if (! (compositeData->IsA("vtkMultiBlockDataSet")
        ||compositeData->IsA("vtkMultiPieceDataSet")) )
    {
    vtkErrorMacro("Unsupported composite dataset type: "
                  << compositeData->GetClassName() << ".");
    return 0;
    }

  // Write each input.
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(compositeData->NewIterator());
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();
  iter->SkipEmptyNodesOff();

  int index = 0;
  int RetVal = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem(), index++)
    {
    vtkDataObject* curDO = iter->GetCurrentDataObject();
    if (curDO && curDO->IsA("vtkCompositeDataSet"))
    // if node is a supported composite dataset
    // note in structure file and recurse.
      {
      vtkXMLDataElement* tag = vtkXMLDataElement::New();

      if (curDO->IsA("vtkMultiPieceDataSet"))
        {
        tag->SetName("Piece");
        tag->SetIntAttribute("index", index);
        }
      else if (curDO->IsA("vtkMultiBlockDataSet"))
        {
        tag->SetName("Block");
        tag->SetIntAttribute("index", index);
        }
      vtkCompositeDataSet* curCD
        = vtkCompositeDataSet::SafeDownCast(curDO);
      if (!this->WriteComposite(curCD, tag, writerIdx))
        {
        tag->Delete();
        return 0;
        }
      RetVal = 1;
      parent->AddNestedElement(tag);
      tag->Delete();
      }
    else
    // this node is not a composite data set.
      {
      vtkXMLDataElement* datasetXML = vtkXMLDataElement::New();
      datasetXML->SetName("DataSet");
      datasetXML->SetIntAttribute("index", index);
      vtkStdString fileName = this->CreatePieceFileName(writerIdx);
      if (this->WriteNonCompositeData( curDO, datasetXML, writerIdx, 
                                       fileName.c_str()))
        {
        parent->AddNestedElement(datasetXML);
        RetVal = 1;
        }
      datasetXML->Delete();
      }
    }
  return RetVal;
}


//----------------------------------------------------------------------------
void vtkXMLMultiBlockDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

