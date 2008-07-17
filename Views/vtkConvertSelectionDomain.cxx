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
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <vtksys/stl/set>

vtkCxxRevisionMacro(vtkConvertSelectionDomain, "1.4");
vtkStandardNewMacro(vtkConvertSelectionDomain);
//----------------------------------------------------------------------------
vtkConvertSelectionDomain::vtkConvertSelectionDomain()
{
  this->SetNumberOfInputPorts(3);
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

//----------------------------------------------------------------------------
int vtkConvertSelectionDomain::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Retrieve the input and output.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkSelection* input = vtkSelection::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkSelection* output = vtkSelection::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // If we have no mapping table, we are done.
  vtkInformation* mapInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* dataInfo = inputVector[2]->GetInformationObject(0);
  if (!dataInfo || !mapInfo)
    {
    output->ShallowCopy(input);
    return 1;
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
    fieldType1 = vtkSelection::POINT;
    dsa2 = vtkDataSet::SafeDownCast(data)->GetCellData();
    fieldType2 = vtkSelection::CELL;
    }
  else if (vtkGraph::SafeDownCast(data))
    {
    dsa1 = vtkGraph::SafeDownCast(data)->GetVertexData();
    fieldType1 = vtkSelection::VERTEX;
    dsa2 = vtkGraph::SafeDownCast(data)->GetEdgeData();
    fieldType2 = vtkSelection::EDGE;
    }
  else if (vtkTable::SafeDownCast(data))
    {
    dsa1 = vtkDataSetAttributes::SafeDownCast(vtkTable::SafeDownCast(data)->GetRowData());
    fieldType1 = vtkSelection::ROW;
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

  vtkSmartPointer<vtkSelection> dummyParent =
    vtkSmartPointer<vtkSelection>::New();
  if (input->GetContentType() != vtkSelection::SELECTIONS)
    {
    dummyParent->SetContentType(vtkSelection::SELECTIONS);
    dummyParent->AddChild(input);
    input = dummyParent;
    }
  output->SetContentType(vtkSelection::SELECTIONS);

  // Iterate over all input selections
  for (unsigned int c = 0; c < input->GetNumberOfChildren(); ++c)
    {
    vtkSelection* curInput = input->GetChild(c);
    vtkSmartPointer<vtkSelection> curOutput =
      vtkSmartPointer<vtkSelection>::New();
    vtkAbstractArray* inArr = curInput->GetSelectionList();

    // Start with a shallow copy of the input selection.
    curOutput->ShallowCopy(curInput);

    // I don't know how to handle this type of selection,
    // so pass it through.
    if (!inArr || !inArr->GetName() ||
        curInput->GetContentType() != vtkSelection::PEDIGREEIDS)
      {
      output->AddChild(curOutput);
      continue;
      }

    // If the selection already matches, we are done.
    if (domains1.count(inArr->GetName()) > 0)
      {
      curOutput->SetFieldType(fieldType1);
      output->AddChild(curOutput);
      continue;
      }
    if (domains2.count(inArr->GetName()) > 0)
      {
      curOutput->SetFieldType(fieldType2);
      output->AddChild(curOutput);
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
    output->AddChild(curOutput);
    }

  // If the output only has one child, we don't need a parent selection.
  if (output->GetNumberOfChildren() <= 1)
    {
    if (output->GetNumberOfChildren() == 1)
      {
      vtkSelection* child = output->GetChild(0);
      child->Register(0);
      output->RemoveChild(child);
      output->ShallowCopy(child);
      child->Delete();
      }
    else
      {
      output->SetContentType(vtkSelection::INDICES);
      vtkSmartPointer<vtkIdTypeArray> inds =
        vtkSmartPointer<vtkIdTypeArray>::New();
      output->SetSelectionList(inds);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkConvertSelectionDomain::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
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
void vtkConvertSelectionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

