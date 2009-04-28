/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAddMembershipArray.cxx

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

#include "vtkAddMembershipArray.h"

#include "vtkAbstractArray.h"
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkConvertSelection.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

vtkCxxRevisionMacro(vtkAddMembershipArray, "1.3");
vtkStandardNewMacro(vtkAddMembershipArray);
vtkCxxSetObjectMacro(vtkAddMembershipArray,InputValues,vtkAbstractArray);

//---------------------------------------------------------------------------
vtkAddMembershipArray::vtkAddMembershipArray()
{
  this->FieldType = -1;
  this->OutputArrayName = 0;
  this->SetOutputArrayName("membership");
  this->InputArrayName = 0;
  this->InputValues = NULL;
  this->SetNumberOfInputPorts(2);
}

//---------------------------------------------------------------------------
vtkAddMembershipArray::~vtkAddMembershipArray()
{
  this->SetOutputArrayName(0);
}

//---------------------------------------------------------------------------
int vtkAddMembershipArray::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if(port == 0)
    {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    }
  else if(port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }

  return 1;
}

//---------------------------------------------------------------------------
int vtkAddMembershipArray::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkSelection* selection = vtkSelection::GetData(inputVector[1]);
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = outputInfo->Get(vtkDataObject::DATA_OBJECT());

  output->ShallowCopy(input);

  if(!selection)
    {
    if(!this->InputArrayName || !this->InputValues)
      return 1;

    vtkDataSetAttributes* ds = 0;;  
    switch(this->FieldType)
      {
      case vtkAddMembershipArray::VERTEX_DATA:
        if(vtkGraph* const graph = vtkGraph::SafeDownCast(output))
          {
          if(graph->GetVertexData())
            {
            ds = graph->GetVertexData();
            }
          }
        break;
      case vtkAddMembershipArray::EDGE_DATA:
        if(vtkGraph* const graph = vtkGraph::SafeDownCast(output))
          {
          if(graph->GetEdgeData())
            {
            ds = graph->GetEdgeData();
            }
          }
        break;
      case vtkAddMembershipArray::ROW_DATA:
        if(vtkTable* const table = vtkTable::SafeDownCast(output))
          {
          if(table->GetRowData())
            {
            ds = table->GetRowData();
            }
          }
        break;
      }

    if(!ds)
      {
      vtkErrorMacro("Unsupported input field type.");
      return 0;
      }
    
    vtkIntArray *vals = vtkIntArray::New();
    vals->SetNumberOfTuples(ds->GetNumberOfTuples());
    vals->SetNumberOfComponents(1);
    vals->SetName(this->OutputArrayName);
    vals->FillComponent(0,0);
    
    vtkAbstractArray *inputArray = ds->GetAbstractArray(this->InputArrayName);
    if(inputArray && this->InputValues)
      {
      for(vtkIdType i=0; i<inputArray->GetNumberOfTuples(); ++i)
        {
        vtkVariant v(0);
        switch (inputArray->GetDataType())
          {
          vtkExtraExtendedTemplateMacro(v = *static_cast<VTK_TT*>(inputArray->GetVoidPointer(i)));
          }
        if(this->InputValues->LookupValue(v)>=0)
          {
          vals->SetValue(i,1);
          }
        else
          {
          vals->SetValue(i,0);
          }
        }
      }

    ds->AddArray(vals);

    vals->Delete();

    return 1;
    }

  // Convert the selection to an INDICES selection
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToIndexSelection(selection, input));
  if (!converted.GetPointer())
    {
    vtkErrorMacro("Selection conversion to INDICES failed.");
    return 0;
    }

  // Collect vertex and edge selections.
  vtkSmartPointer<vtkIdTypeArray> rowList = vtkSmartPointer<vtkIdTypeArray>::New();
  bool hasRows = false;
  vtkSmartPointer<vtkIdTypeArray> edgeList = vtkSmartPointer<vtkIdTypeArray>::New();
  bool hasEdges = false;
  vtkSmartPointer<vtkIdTypeArray> vertexList = vtkSmartPointer<vtkIdTypeArray>::New();
  bool hasVertices = false;
  for (unsigned int i = 0; i < converted->GetNumberOfNodes(); ++i)
    {
    vtkSelectionNode* node = converted->GetNode(i);
    vtkIdTypeArray* list = 0;
    if (node->GetFieldType() == vtkSelectionNode::VERTEX)
      {
      list = vertexList;
      hasVertices = true;
      }
    else if (node->GetFieldType() == vtkSelectionNode::EDGE)
      {
      list = edgeList;
      hasEdges = true;
      }
    else if (node->GetFieldType() == vtkSelectionNode::ROW)
      {
      list = rowList;
      hasRows = true;
      }

    if (list)
      {
      // Append the selection list to the selection
      vtkIdTypeArray* curList = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      if (curList)
        {
        vtkIdType numTuples = curList->GetNumberOfTuples();
        for (vtkIdType j = 0; j < numTuples; ++j)
          {
          vtkIdType curValue = curList->GetValue(j);
          if (list->LookupValue(curValue) < 0)
            {
            list->InsertNextValue(curValue);
            }
          }
        } // end if (curList)
      } // end if (list)
    } // end for each child
  
  vtkGraph* graph = vtkGraph::SafeDownCast(output);
  vtkTable* table = vtkTable::SafeDownCast(output);
  if (hasVertices && vertexList->GetNumberOfTuples() != 0) 
    {
    vtkDataSetAttributes *ds;
    if(graph)
      ds = graph->GetVertexData();
    else if(table && this->FieldType == vtkAddMembershipArray::VERTEX_DATA)
      ds = table->GetRowData();
    else
      return 0;
    vtkSmartPointer<vtkIntArray> vals = vtkSmartPointer<vtkIntArray>::New();
    vals->SetNumberOfTuples(ds->GetNumberOfTuples());
    vals->SetNumberOfComponents(1);
    vals->SetName(this->OutputArrayName);
    vals->FillComponent(0,0);
    vtkIdType numSelectedVerts = vertexList->GetNumberOfTuples();
    for (vtkIdType i = 0; i < numSelectedVerts; ++i)
      {
      vals->SetValue(vertexList->GetValue(i), 1);
      }
    ds->AddArray(vals);
    }

  if (hasEdges && edgeList->GetNumberOfTuples() != 0) 
    {
    vtkDataSetAttributes *ds;
    if(graph)
      ds = graph->GetEdgeData();
    else if(table && this->FieldType == vtkAddMembershipArray::EDGE_DATA)
      ds = table->GetRowData();
    else
      return 0;
    vtkSmartPointer<vtkIntArray> vals = vtkSmartPointer<vtkIntArray>::New();
    vals->SetNumberOfTuples(ds->GetNumberOfTuples());
    vals->SetNumberOfComponents(1);
    vals->SetName(this->OutputArrayName);
    vals->FillComponent(0,0);
    vtkIdType numSelectedEdges = edgeList->GetNumberOfTuples();
    for (vtkIdType i = 0; i < numSelectedEdges; ++i)
      {
      vals->SetValue(edgeList->GetValue(i), 1);
      }
    ds->AddArray(vals);
    }

  if (table && hasRows && rowList->GetNumberOfTuples() != 0) 
    {
    vtkDataSetAttributes *ds = table->GetRowData();
    vtkSmartPointer<vtkIntArray> vals = vtkSmartPointer<vtkIntArray>::New();
    vals->SetNumberOfTuples(ds->GetNumberOfTuples());
    vals->SetNumberOfComponents(1);
    vals->SetName(this->OutputArrayName);
    vals->FillComponent(0,0);
    vtkIdType numSelectedRows = rowList->GetNumberOfTuples();
    for (vtkIdType i = 0; i < numSelectedRows; ++i)
      {
      vals->SetValue(rowList->GetValue(i), 1);
      }
    ds->AddArray(vals);
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkAddMembershipArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldType: " << this->FieldType << endl;
  os << indent << "OutputArrayName: " 
    << this->OutputArrayName << endl;
  os << indent << "InputArrayName: " 
    << this->InputArrayName << endl;
  if(this->InputValues)
    {
    os << indent << "Input Values :" << endl;
    int num = this->InputValues->GetNumberOfTuples();
    for (int idx = 0; idx < num; ++idx)
      {
      vtkVariant v(0);
      switch (this->InputValues->GetDataType())
        {
        vtkExtraExtendedTemplateMacro(v = *static_cast<VTK_TT*>(this->InputValues->GetVoidPointer(idx)));
        }
      os << v.ToString() << endl;
      }
    }
}
