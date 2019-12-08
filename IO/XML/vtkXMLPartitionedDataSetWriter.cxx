/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPartitionedDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPartitionedDataSetWriter.h"

#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLPartitionedDataSetWriter);
//----------------------------------------------------------------------------
vtkXMLPartitionedDataSetWriter::vtkXMLPartitionedDataSetWriter() {}

//----------------------------------------------------------------------------
vtkXMLPartitionedDataSetWriter::~vtkXMLPartitionedDataSetWriter() {}

//----------------------------------------------------------------------------
int vtkXMLPartitionedDataSetWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPartitionedDataSetWriter::WriteComposite(
  vtkCompositeDataSet* compositeData, vtkXMLDataElement* parent, int& writerIdx)
{
  if (!compositeData->IsA("vtkPartitionedDataSet"))
  {
    vtkErrorMacro("Unsupported composite dataset type: " << compositeData->GetClassName() << ".");
    return 0;
  }

  // Write each input.
  vtkSmartPointer<vtkDataObjectTreeIterator> iter;
  iter.TakeReference(vtkDataObjectTree::SafeDownCast(compositeData)->NewTreeIterator());
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();
  iter->SkipEmptyNodesOff();
  int toBeWritten = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    toBeWritten++;
  }

  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  int index = 0;
  int RetVal = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), index++)
  {
    vtkDataObject* curDO = iter->GetCurrentDataObject();

    vtkXMLDataElement* datasetXML = vtkXMLDataElement::New();
    datasetXML->SetName("DataSet");
    datasetXML->SetIntAttribute("index", index);
    vtkStdString fileName = this->CreatePieceFileName(writerIdx);

    this->SetProgressRange(progressRange, writerIdx, toBeWritten);
    if (this->WriteNonCompositeData(curDO, datasetXML, writerIdx, fileName.c_str()))
    {
      parent->AddNestedElement(datasetXML);
      RetVal = 1;
    }
    datasetXML->Delete();
  }
  return RetVal;
}

//----------------------------------------------------------------------------
void vtkXMLPartitionedDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
