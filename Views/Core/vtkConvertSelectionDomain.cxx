/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConvertSelectionDomain.cxx

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
#include "vtkConvertSelectionDomain.h"

#include "vtkAbstractArray.h"
#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSet.h"
#include "vtkGraph.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <vtksys/stl/set>

vtkStandardNewMacro(vtkConvertSelectionDomain);
//----------------------------------------------------------------------------
vtkConvertSelectionDomain::vtkConvertSelectionDomain()
{
  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(2);
}

//----------------------------------------------------------------------------
vtkConvertSelectionDomain::~vtkConvertSelectionDomain()
{
}

//----------------------------------------------------------------------------
void vtkConvertSelectionDomainFindDomains(
  vtkDataSetAttributes* dsa,
  vtksys_stl::set<vtkStdString> & domains)
{
  if (dsa->GetAbstractArray("domain"))
    {
    vtkStringArray* domainArr = vtkStringArray::SafeDownCast(
      dsa->GetAbstractArray("domain"));
    if (!domainArr)
      {
      return; // Do nothing if the array isn't a string array
      }
    vtkIdType numTuples = domainArr->GetNumberOfTuples();
    for (vtkIdType i = 0; i < numTuples; ++i)
      {
      vtkStdString d = domainArr->GetValue(i);
      if (domains.count(d) == 0)
        {
        domains.insert(d);
        }
      }
    }
  else if (dsa->GetPedigreeIds() && dsa->GetPedigreeIds()->GetName())
    {
    domains.insert(dsa->GetPedigreeIds()->GetName());
    }
}

void vtkConvertSelectionDomainConvertAnnotationDomain(
  vtkAnnotation* annIn,
  vtkAnnotation* annOut,
  vtksys_stl::set<vtkStdString>& domains1,
  vtksys_stl::set<vtkStdString>& domains2,
  vtkDataSetAttributes* dsa1,
  vtkDataSetAttributes* dsa2,
  int fieldType1, int fieldType2,
  vtkMultiBlockDataSet* maps)
{
  vtkSelection* inputSel = annIn->GetSelection();
  vtkSmartPointer<vtkSelection> outputSel =
    vtkSmartPointer<vtkSelection>::New();
  // Iterate over all input selections
  for (unsigned int c = 0; c < inputSel->GetNumberOfNodes(); ++c)
    {
    vtkSelectionNode* curInput = inputSel->GetNode(c);
    vtkSmartPointer<vtkSelectionNode> curOutput =
      vtkSmartPointer<vtkSelectionNode>::New();
    vtkAbstractArray* inArr = curInput->GetSelectionList();

    // Start with a shallow copy of the input selection.
    curOutput->ShallowCopy(curInput);

    // I don't know how to handle this type of selection,
    // so pass it through.
    if (!inArr || !inArr->GetName() ||
        curInput->GetContentType() != vtkSelectionNode::PEDIGREEIDS)
      {
      outputSel->AddNode(curOutput);
      continue;
      }

    // If the selection already matches, we are done.
    if (domains1.count(inArr->GetName()) > 0)
      {
      curOutput->SetFieldType(fieldType1);
      outputSel->AddNode(curOutput);
      continue;
      }
    if (domains2.count(inArr->GetName()) > 0)
      {
      curOutput->SetFieldType(fieldType2);
      outputSel->AddNode(curOutput);
      continue;
      }

    // Select the correct source and destination mapping arrays.
    vtkAbstractArray* fromArr = 0;
    vtkAbstractArray* toArr = 0;
    unsigned int numMaps = maps->GetNumberOfBlocks();
    for (unsigned int i = 0; i < numMaps; ++i)
      {
      fromArr = 0;
      toArr = 0;
      vtkTable* table = vtkTable::SafeDownCast(maps->GetBlock(i));
      if (table)
        {
        fromArr = table->GetColumnByName(inArr->GetName());
        vtksys_stl::set<vtkStdString>::iterator it, itEnd;
        if (dsa1)
          {
          it = domains1.begin();
          itEnd = domains1.end();
          for (; it != itEnd; ++it)
            {
            toArr = table->GetColumnByName(it->c_str());
            if (toArr)
              {
              curOutput->SetFieldType(fieldType1);
              break;
              }
            }
          }
        if (!toArr && dsa2)
          {
          it = domains2.begin();
          itEnd = domains2.end();
          for (; it != itEnd; ++it)
            {
            toArr = table->GetColumnByName(it->c_str());
            if (toArr)
              {
              curOutput->SetFieldType(fieldType2);
              break;
              }
            }
          }
        }
      if (fromArr && toArr)
        {
        break;
        }
      }

    // Cannot do the conversion, so don't pass this selection
    // to the output.
    if (!fromArr || !toArr)
      {
      continue;
      }

    // Lookup values in the input selection and map them
    // through the table to the output selection.
    vtkIdType numTuples = inArr->GetNumberOfTuples();
    vtkSmartPointer<vtkAbstractArray> outArr;
    outArr.TakeReference(vtkAbstractArray::CreateArray(toArr->GetDataType()));
    outArr->SetName(toArr->GetName());
    vtkSmartPointer<vtkIdList> ids = vtkSmartPointer<vtkIdList>::New();
    for (vtkIdType i = 0; i < numTuples; ++i)
      {
      fromArr->LookupValue(inArr->GetVariantValue(i), ids);
      vtkIdType numIds = ids->GetNumberOfIds();
      for (vtkIdType j = 0; j < numIds; ++j)
        {
        outArr->InsertNextTuple(ids->GetId(j), toArr);
        }
      }
    curOutput->SetSelectionList(outArr);
    outputSel->AddNode(curOutput);
    }
  // Make sure there is at least something in the output selection.
  if (outputSel->GetNumberOfNodes() == 0)
    {
    vtkSmartPointer<vtkSelectionNode> node = vtkSmartPointer<vtkSelectionNode>::New();
    node->SetContentType(vtkSelectionNode::INDICES);
    vtkSmartPointer<vtkIdTypeArray> inds =
      vtkSmartPointer<vtkIdTypeArray>::New();
    node->SetSelectionList(inds);
    outputSel->AddNode(node);
    }

  annOut->ShallowCopy(annIn);
  annOut->SetSelection(outputSel);
}

//----------------------------------------------------------------------------
int vtkConvertSelectionDomain::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Retrieve the input and output.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkAnnotationLayers* inputAnn = vtkAnnotationLayers::SafeDownCast(input);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkAnnotationLayers* outputAnn = vtkAnnotationLayers::SafeDownCast(output);

  outInfo = outputVector->GetInformationObject(1);
  vtkSelection* outputCurrentSel = vtkSelection::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // If we have no mapping table, we are done.
  vtkInformation* mapInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* dataInfo = inputVector[2]->GetInformationObject(0);
  if (!dataInfo || !mapInfo)
    {
    output->ShallowCopy(input);
    return 1;
    }

  // If the input is instead a vtkSelection, wrap it in a vtkAnnotationLayers
  // object so it can be used uniformly in the function.
  bool createdInput = false;
  if (!inputAnn)
    {
    vtkSelection* inputSel = vtkSelection::SafeDownCast(input);
    inputAnn = vtkAnnotationLayers::New();
    inputAnn->SetCurrentSelection(inputSel);
    vtkSelection* outputSel = vtkSelection::SafeDownCast(output);
    outputAnn = vtkAnnotationLayers::New();
    outputAnn->SetCurrentSelection(outputSel);
    createdInput = true;
    }

  vtkMultiBlockDataSet* maps = vtkMultiBlockDataSet::SafeDownCast(
    mapInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataObject* data = dataInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkDataSetAttributes* dsa1 = 0;
  int fieldType1 = 0;
  vtkDataSetAttributes* dsa2 = 0;
  int fieldType2 = 0;
  if (vtkDataSet::SafeDownCast(data))
    {
    dsa1 = vtkDataSet::SafeDownCast(data)->GetPointData();
    fieldType1 = vtkSelectionNode::POINT;
    dsa2 = vtkDataSet::SafeDownCast(data)->GetCellData();
    fieldType2 = vtkSelectionNode::CELL;
    }
  else if (vtkGraph::SafeDownCast(data))
    {
    dsa1 = vtkGraph::SafeDownCast(data)->GetVertexData();
    fieldType1 = vtkSelectionNode::VERTEX;
    dsa2 = vtkGraph::SafeDownCast(data)->GetEdgeData();
    fieldType2 = vtkSelectionNode::EDGE;
    }
  else if (vtkTable::SafeDownCast(data))
    {
    dsa1 = vtkDataSetAttributes::SafeDownCast(vtkTable::SafeDownCast(data)->GetRowData());
    fieldType1 = vtkSelectionNode::ROW;
    }

  vtksys_stl::set<vtkStdString> domains1;
  vtksys_stl::set<vtkStdString> domains2;
  if (dsa1)
    {
    vtkConvertSelectionDomainFindDomains(dsa1, domains1);
    }
  if (dsa2)
    {
    vtkConvertSelectionDomainFindDomains(dsa2, domains2);
    }

  for (unsigned int a = 0; a < inputAnn->GetNumberOfAnnotations(); ++a)
    {
    vtkSmartPointer<vtkAnnotation> ann =
      vtkSmartPointer<vtkAnnotation>::New();
    vtkConvertSelectionDomainConvertAnnotationDomain(
      inputAnn->GetAnnotation(a), ann,
      domains1, domains2, dsa1, dsa2, fieldType1, fieldType2, maps);
    outputAnn->AddAnnotation(ann);
    }

  if (inputAnn->GetCurrentAnnotation())
    {
    vtkSmartPointer<vtkAnnotation> ann =
      vtkSmartPointer<vtkAnnotation>::New();
    vtkConvertSelectionDomainConvertAnnotationDomain(
      inputAnn->GetCurrentAnnotation(), ann,
      domains1, domains2, dsa1, dsa2, fieldType1, fieldType2, maps);
    outputAnn->SetCurrentAnnotation(ann);
    }
  else
    {
    outputAnn->SetCurrentAnnotation(0);
    }

  // Copy current selection to the second output
  if (outputAnn->GetCurrentSelection())
    {
    outputCurrentSel->ShallowCopy(outputAnn->GetCurrentSelection());
    }

  if (createdInput)
    {
    inputAnn->Delete();
    outputAnn->Delete();
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkConvertSelectionDomain::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkAnnotationLayers");
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
    }
  else if (port == 2)
    {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkConvertSelectionDomain::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  this->Superclass::FillOutputPortInformation(port, info);
  if (port == 1)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkSelection");
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkConvertSelectionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

