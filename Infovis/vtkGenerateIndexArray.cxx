/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenerateIndexArray.cxx

  Copyright 2007 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkGenerateIndexArray.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTable.h"

#include <map>

vtkStandardNewMacro(vtkGenerateIndexArray);

vtkGenerateIndexArray::vtkGenerateIndexArray() :
  ArrayName(0),
  FieldType(ROW_DATA),
  ReferenceArrayName(0),
  PedigreeID(false)
{
  this->SetArrayName("index");
}

vtkGenerateIndexArray::~vtkGenerateIndexArray()
{
  this->SetArrayName(0);
  this->SetReferenceArrayName(0);
}

void vtkGenerateIndexArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "ArrayName: " << (this->ArrayName ? this->ArrayName : "(none)") << endl;
  os << "FieldType: " << this->FieldType << endl;
  os << "ReferenceArrayName: " << (this->ReferenceArrayName ? this->ReferenceArrayName : "(none)") << endl;
  os << "PedigreeID: " << this->PedigreeID << endl;
}

int vtkGenerateIndexArray::ProcessRequest(
  vtkInformation* request, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int vtkGenerateIndexArray::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
    
      if (!output || !output->IsA(input->GetClassName())) 
        {
        vtkDataObject* newOutput = input->NewInstance();
        newOutput->SetPipelineInformation(info);
        newOutput->Delete();
        }
      }
    return 1;
    }
  return 0;
}

int vtkGenerateIndexArray::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // An output array name is required ...
  if(!(this->ArrayName && strlen(this->ArrayName)))
    {
    vtkErrorMacro(<< "No array name defined.");
    return 0;
    }
  
  // Make a shallow-copy of our input ...
  vtkDataObject* const input = vtkDataObject::GetData(inputVector[0]);
  vtkDataObject* const output = vtkDataObject::GetData(outputVector);
  output->ShallowCopy(input);

  // Figure-out where we'll be reading/writing data ...
  vtkDataSetAttributes* output_attributes = 0;
  vtkIdType output_count = 0;

  switch(this->FieldType)
    {
    case ROW_DATA:
      {
      vtkTable* const table = vtkTable::SafeDownCast(output);
      output_attributes = table ? table->GetRowData() : 0;
      output_count = table ? table->GetNumberOfRows() : 0;
      break;
      }
    case POINT_DATA:
      {
      vtkDataSet* const data_set = vtkDataSet::SafeDownCast(output);
      output_attributes = data_set ? data_set->GetPointData() : 0;
      output_count = data_set ? data_set->GetNumberOfPoints() : 0;
      break;
      }
    case CELL_DATA:
      {
      vtkDataSet* const data_set = vtkDataSet::SafeDownCast(output);
      output_attributes = data_set ? data_set->GetCellData() : 0;
      output_count = data_set ? data_set->GetNumberOfCells() : 0;
      break;
      }
    case VERTEX_DATA:
      {
      vtkGraph* const graph = vtkGraph::SafeDownCast(output);
      output_attributes = graph ? graph->GetVertexData() : 0;
      output_count = graph ? graph->GetNumberOfVertices() : 0;
      break;
      }
    case EDGE_DATA:
      {
      vtkGraph* const graph = vtkGraph::SafeDownCast(output);
      output_attributes = graph ? graph->GetEdgeData() : 0;
      output_count = graph ? graph->GetNumberOfEdges() : 0;
      break;
      }
    }

  if(!output_attributes)
    {
    vtkErrorMacro(<< "Invalid field type for this data object.");
    return 0;
    }

  // Create our output array ...
  vtkIdTypeArray* const output_array = vtkIdTypeArray::New();
  output_array->SetName(this->ArrayName);
  output_array->SetNumberOfTuples(output_count);
  output_attributes->AddArray(output_array);
  output_array->Delete();

  if(this->PedigreeID)
    output_attributes->SetPedigreeIds(output_array);

  // Generate indices based on the reference array ...
  if(this->ReferenceArrayName && strlen(this->ReferenceArrayName))
    {
    int reference_array_index = -1;
    vtkAbstractArray* const reference_array = output_attributes->GetAbstractArray(this->ReferenceArrayName, reference_array_index);
    if(!reference_array)
      {
      vtkErrorMacro(<< "No reference array " << this->ReferenceArrayName);
      return 0;
      }

    typedef std::map<vtkVariant, vtkIdType, vtkVariantLessThan> index_map_t;
    index_map_t index_map;

    for(vtkIdType i = 0; i != output_count; ++i)
      {
      if(!index_map.count(reference_array->GetVariantValue(i)))
        {
#ifdef _RWSTD_NO_MEMBER_TEMPLATES
        // Deal with Sun Studio old libCstd.
        // http://sahajtechstyle.blogspot.com/2007/11/whats-wrong-with-sun-studio-c.html
        index_map.insert(std::pair<const vtkVariant,vtkIdType>(reference_array->GetVariantValue(i), 0));
#else
        index_map.insert(std::make_pair(reference_array->GetVariantValue(i), 0));
#endif
        }
      }

    vtkIdType index = 0;
    for(index_map_t::iterator i = index_map.begin(); i != index_map.end(); ++i, ++index)
      i->second = index;

    for(vtkIdType i = 0; i != output_count; ++i)
      output_array->SetValue(i, index_map[reference_array->GetVariantValue(i)]);
    }
  // Otherwise, generate a trivial index array ...
  else
    {
    for(vtkIdType i = 0; i != output_count; ++i)
      output_array->SetValue(i, i);
    }

  return 1;
} 

