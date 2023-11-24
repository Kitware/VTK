// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLMultiBlockDataWriter.h"

#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLMultiBlockDataWriter);
//------------------------------------------------------------------------------
vtkXMLMultiBlockDataWriter::vtkXMLMultiBlockDataWriter() = default;

//------------------------------------------------------------------------------
vtkXMLMultiBlockDataWriter::~vtkXMLMultiBlockDataWriter() = default;

//------------------------------------------------------------------------------
int vtkXMLMultiBlockDataWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkXMLMultiBlockDataWriter::WriteComposite(
  vtkCompositeDataSet* compositeData, vtkXMLDataElement* parent, int& writerIdx)
{
  if (!(compositeData->IsA("vtkMultiBlockDataSet") || compositeData->IsA("vtkMultiPieceDataSet")))
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

  if (toBeWritten == 0)
  {
    // No leaf, nothing to write but this is not an issue.
    return 1;
  }

  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  int index = 0;
  int RetVal = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), index++)
  {
    vtkDataObject* curDO = iter->GetCurrentDataObject();
    const char* name = nullptr;
    if (iter->HasCurrentMetaData())
    {
      name = iter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME());
    }

    if (curDO && curDO->IsA("vtkCompositeDataSet"))
    // if node is a supported composite dataset
    // note in structure file and recurse.
    {
      vtkXMLDataElement* tag = vtkXMLDataElement::New();
      if (name)
      {
        tag->SetAttribute("name", name);
      }

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
      vtkCompositeDataSet* curCD = vtkCompositeDataSet::SafeDownCast(curDO);
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
      if (name)
      {
        datasetXML->SetAttribute("name", name);
      }
      std::string fileName = this->CreatePieceFileName(writerIdx);

      this->SetProgressRange(progressRange, writerIdx, toBeWritten);
      if (this->WriteNonCompositeData(curDO, datasetXML, writerIdx, fileName.c_str()))
      {
        parent->AddNestedElement(datasetXML);
        RetVal = 1;
      }
      datasetXML->Delete();
    }
  }
  return RetVal;
}

//------------------------------------------------------------------------------
void vtkXMLMultiBlockDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
