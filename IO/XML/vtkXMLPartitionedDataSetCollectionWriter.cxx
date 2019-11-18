/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPartitionedDataSetCollectionWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPartitionedDataSetCollectionWriter.h"

#include "vtkDataObjectTreeRange.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLPartitionedDataSetCollectionWriter);
//----------------------------------------------------------------------------
vtkXMLPartitionedDataSetCollectionWriter::vtkXMLPartitionedDataSetCollectionWriter() {}

//----------------------------------------------------------------------------
vtkXMLPartitionedDataSetCollectionWriter::~vtkXMLPartitionedDataSetCollectionWriter() {}

//----------------------------------------------------------------------------
int vtkXMLPartitionedDataSetCollectionWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPartitionedDataSetCollectionWriter::WriteComposite(
  vtkCompositeDataSet* compositeData, vtkXMLDataElement* parent, int& writerIdx)
{
  if (!(compositeData->IsA("vtkPartitionedDataSet") ||
        compositeData->IsA("vtkPartitionedDataSetCollection")))
  {
    vtkErrorMacro("Unsupported composite dataset type: " << compositeData->GetClassName() << ".");
    return 0;
  }

  auto* dObjTree = static_cast<vtkDataObjectTree*>(compositeData);

  // Write each input.
  using Opts = vtk::DataObjectTreeOptions;
  const auto dObjRange = vtk::Range(dObjTree, Opts::None);
  int toBeWritten = static_cast<int>(dObjRange.size());

  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  int index = 0;
  int RetVal = 0;
  for (vtkDataObject* curDO : dObjRange)
  {
    if (curDO && curDO->IsA("vtkCompositeDataSet"))
    // if node is a supported composite dataset
    // note in structure file and recurse.
    {
      vtkXMLDataElement* tag = vtkXMLDataElement::New();
      tag->SetName("Partitions");
      tag->SetIntAttribute("index", index);
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
      vtkStdString fileName = this->CreatePieceFileName(writerIdx);

      this->SetProgressRange(progressRange, writerIdx, toBeWritten);
      if (this->WriteNonCompositeData(curDO, datasetXML, writerIdx, fileName.c_str()))
      {
        parent->AddNestedElement(datasetXML);
        RetVal = 1;
      }
      datasetXML->Delete();
    }

    index++;
  }

  return RetVal;
}

//----------------------------------------------------------------------------
void vtkXMLPartitionedDataSetCollectionWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
