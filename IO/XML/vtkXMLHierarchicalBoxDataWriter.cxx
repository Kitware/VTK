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

#include "vtkUniformGridAMRDataIterator.h"
#include "vtkErrorCode.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkAMRBox.h"

#include <assert.h>

vtkStandardNewMacro(vtkXMLHierarchicalBoxDataWriter);
//----------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataWriter::vtkXMLHierarchicalBoxDataWriter()
{
  this->AMRBoxes = NULL;
}

//----------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataWriter::~vtkXMLHierarchicalBoxDataWriter()
{
  delete [] this->AMRBoxes;
  this->AMRBoxes = NULL;
}

//----------------------------------------------------------------------------
int vtkXMLHierarchicalBoxDataWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLHierarchicalBoxDataWriter::FillDataTypes(vtkCompositeDataSet* cdInput)
{
  this->Superclass::FillDataTypes(cdInput);
  // Build information about the boxes. This is a bit irrelevant in serial, but
  // makes it easier when processing in parallel.

  vtkHierarchicalBoxDataSet* hdInput =
    vtkHierarchicalBoxDataSet::SafeDownCast(cdInput);
  assert("dataset must be vtkHierarchicalBoxDataSet" && hdInput != NULL);

  delete [] this->AMRBoxes;

  unsigned int numLeafNodes = this->GetNumberOfDataTypes();
  this->AMRBoxes = new int[numLeafNodes * 6];
  memset(this->AMRBoxes, 0, numLeafNodes*6*sizeof(int));

  vtkUniformGridAMRDataIterator* iter = vtkUniformGridAMRDataIterator::SafeDownCast(hdInput->NewIterator());
  iter->SkipEmptyNodesOff();
  int leafNo =  0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem(), leafNo++)
    {
    if (iter->GetCurrentDataObject())
      {
      vtkAMRBox box = hdInput->GetAMRBox(iter->GetCurrentLevel(),iter->GetCurrentIndex());
      box.GetDimensions(&this->AMRBoxes[leafNo*6]);
      }
    }
  iter->Delete();
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
      vtkUniformGrid* ug = hboxData->GetDataSet(level, cc);
      vtkSmartPointer<vtkXMLDataElement> datasetXML =
        vtkSmartPointer<vtkXMLDataElement>::New();
      datasetXML->SetName("DataSet");
      datasetXML->SetIntAttribute("index", cc);
      // we use the box from this->AMRBoxes since that datastructure is
      // synchronized when running in parallel.
      datasetXML->SetVectorAttribute("amr_box", 6, &this->AMRBoxes[writerIdx*6]);
      vtkStdString fileName = this->CreatePieceFileName(writerIdx);
      if (fileName != "")
        {
        // if fileName is empty, it implies that no file is written out for this
        // node, so don't add a filename attribute for it.
        datasetXML->SetAttribute("file", fileName);
        }
      block->AddNestedElement(datasetXML);

      // if this->WriteNonCompositeData() returns 0, it doesn't meant it's an
      // error, it just means that it didn't write a file for the current node.
      this->WriteNonCompositeData(ug, datasetXML, writerIdx, fileName.c_str());

      if (this->GetErrorCode() != vtkErrorCode::NoError)
        {
        return 0;
        }
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

