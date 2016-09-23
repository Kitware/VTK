/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemoveHiddenData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkRemoveHiddenData.h"

#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkCellData.h"
#include "vtkConvertSelection.h"
#include "vtkDoubleArray.h"
#include "vtkExtractSelectedGraph.h"
#include "vtkExtractSelectedRows.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkDataSet.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"


vtkStandardNewMacro(vtkRemoveHiddenData);

vtkRemoveHiddenData::vtkRemoveHiddenData()
{
  this->ExtractGraph = vtkSmartPointer<vtkExtractSelectedGraph>::New();
  this->ExtractGraph->SetRemoveIsolatedVertices(false);

  this->ExtractTable = vtkSmartPointer<vtkExtractSelectedRows>::New();

  this->SetNumberOfInputPorts(2);
}

vtkRemoveHiddenData::~vtkRemoveHiddenData()
{
}

int vtkRemoveHiddenData::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkAnnotationLayers");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

int vtkRemoveHiddenData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* annotationsInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkAnnotationLayers* annotations = 0;
  if (annotationsInfo)
  {
    annotations = vtkAnnotationLayers::SafeDownCast(
      annotationsInfo->Get(vtkDataObject::DATA_OBJECT()));
  }

  // Nothing to do if no input annotations
  if (!annotations)
  {
    output->ShallowCopy(input);
    return 1;
  }

  vtkGraph* graph = vtkGraph::SafeDownCast(output);
  vtkTable* table = vtkTable::SafeDownCast(output);

  vtkSmartPointer<vtkSelection> selection = vtkSmartPointer<vtkSelection>::New();
  unsigned int numAnnotations = annotations->GetNumberOfAnnotations();
  int numHiddenAnnotations = 0;
  for (unsigned int a = 0; a < numAnnotations; ++a)
  {
    vtkAnnotation* ann = annotations->GetAnnotation(a);

    // Only if the annotation is both enabled AND hidden will
    // its selection get added
    if (ann->GetInformation()->Has(vtkAnnotation::ENABLE()) &&
        ann->GetInformation()->Get(vtkAnnotation::ENABLE())==1 &&
        ann->GetInformation()->Has(vtkAnnotation::HIDE()) &&
        ann->GetInformation()->Get(vtkAnnotation::HIDE())==1 )
    {
      selection->Union(ann->GetSelection());
      numHiddenAnnotations++;
    }
  }

  // Nothing to do if no hidden annotations
  if(numHiddenAnnotations == 0)
  {
    output->ShallowCopy(input);
    return 1;
  }

  // We want to output the visible data, so the hidden annotation
  // selections need to be inverted before sent to the extraction filter:
  for (unsigned int i = 0; i < selection->GetNumberOfNodes(); ++i)
  {
    vtkSelectionNode* node = selection->GetNode(i);
    node->GetProperties()->Set(vtkSelectionNode::INVERSE(),1);
  }

  if (graph)
  {
    this->ExtractGraph->SetInputData(input);
    this->ExtractGraph->SetInputData(1, selection);
    this->ExtractGraph->Update();
    output->ShallowCopy(this->ExtractGraph->GetOutput());
  }
  else if (table)
  {
    this->ExtractTable->SetInputData(input);
    this->ExtractTable->SetInputData(1, selection);
    this->ExtractTable->Update();
    output->ShallowCopy(this->ExtractTable->GetOutput());
  }
  else
  {
    vtkErrorMacro("Unsupported input data type.");
    return 0;
  }

  return 1;
}



void vtkRemoveHiddenData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
