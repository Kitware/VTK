/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapArrayValues.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMapArrayValues.h"

#include "vtkAbstractArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraph.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include <cctype>

#include <map>
#include <utility>

vtkStandardNewMacro(vtkMapArrayValues);

typedef std::map< vtkVariant, vtkVariant, vtkVariantLessThan > MapBase;
class vtkMapType : public MapBase {};

vtkMapArrayValues::vtkMapArrayValues()
{
  this->InputArrayName = 0;
  this->OutputArrayName = 0;
  this->SetOutputArrayName("ArrayMap");
  this->FieldType = vtkMapArrayValues::POINT_DATA;
  this->OutputArrayType = VTK_INT;
  this->PassArray = 0;
  this->FillValue = -1;

  this->Map = new vtkMapType;
}

vtkMapArrayValues::~vtkMapArrayValues()
{
  this->SetInputArrayName(0);
  this->SetOutputArrayName(0);
  delete this->Map;
}

void vtkMapArrayValues::AddToMap(char *from, int to)
{
  vtkVariant fromVar(from);
  vtkVariant toVar(to);
  this->Map->insert(std::make_pair(fromVar, toVar));

  this->Modified();
}

void vtkMapArrayValues::AddToMap(int from, int to)
{
  vtkVariant fromVar(from);
  vtkVariant toVar(to);
  this->Map->insert(std::make_pair(fromVar, toVar));

  this->Modified();
}

void vtkMapArrayValues::AddToMap(int from, char *to)
{
  vtkVariant fromVar(from);
  vtkVariant toVar(to);
  this->Map->insert(std::make_pair(fromVar, toVar));

  this->Modified();
}

void vtkMapArrayValues::AddToMap(char *from, char *to)
{
  vtkVariant fromVar(from);
  vtkVariant toVar(to);
  this->Map->insert(std::make_pair(fromVar, toVar));

  this->Modified();
}

void vtkMapArrayValues::AddToMap(vtkVariant from, vtkVariant to)
{
  vtkVariant fromVar(from);
  vtkVariant toVar(to);
  this->Map->insert(std::make_pair(fromVar, toVar));

  this->Modified();
}

void vtkMapArrayValues::ClearMap()
{
  this->Map->clear();

  this->Modified();
}

int vtkMapArrayValues::GetMapSize()
{
  return static_cast<int>(this->Map->size());
}

int vtkMapArrayValues::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if(!this->InputArrayName)
  {
    //vtkErrorMacro(<<"Input array not specified.");
    output->ShallowCopy(input);
    return 1;
  }

  vtkDataSetAttributes* ods=0;
  if (vtkDataSet::SafeDownCast(input))
  {
    vtkDataSet *dsInput = vtkDataSet::SafeDownCast(input);
    vtkDataSet *dsOutput = vtkDataSet::SafeDownCast(output);
    // This has to be here because it initialized all field datas.
    dsOutput->CopyStructure( dsInput );

    if ( dsOutput->GetFieldData() && dsInput->GetFieldData() )
    {
      dsOutput->GetFieldData()->PassData( dsInput->GetFieldData() );
    }
    dsOutput->GetPointData()->PassData( dsInput->GetPointData() );
    dsOutput->GetCellData()->PassData( dsInput->GetCellData() );
    switch (this->FieldType)
    {
      case vtkMapArrayValues::POINT_DATA:
        ods = dsOutput->GetPointData();
        break;
      case vtkMapArrayValues::CELL_DATA:
        ods = dsOutput->GetCellData();
        break;
      default:
        vtkErrorMacro(<<"Data must be point or cell for vtkDataSet");
        return 0;
    }
  }
  else if (vtkGraph::SafeDownCast(input))
  {
    vtkGraph *graphInput = vtkGraph::SafeDownCast(input);
    vtkGraph *graphOutput = vtkGraph::SafeDownCast(output);
    graphOutput->ShallowCopy( graphInput );
    switch (this->FieldType)
    {
      case vtkMapArrayValues::VERTEX_DATA:
        ods = graphOutput->GetVertexData();
        break;
      case vtkMapArrayValues::EDGE_DATA:
        ods = graphOutput->GetEdgeData();
        break;
      default:
        vtkErrorMacro(<<"Data must be vertex or edge for vtkGraph");
        return 0;
    }
  }
  else if (vtkTable::SafeDownCast(input))
  {
    vtkTable *tableInput = vtkTable::SafeDownCast(input);
    vtkTable *tableOutput = vtkTable::SafeDownCast(output);
    tableOutput->ShallowCopy( tableInput );
    switch (this->FieldType)
    {
      case vtkMapArrayValues::ROW_DATA:
        ods = tableOutput->GetRowData();
        break;
      default:
        vtkErrorMacro(<<"Data must be row for vtkTable");
        return 0;
    }
  }
  else
  {
    vtkErrorMacro(<<"Invalid input type");
    return 0;
  }

  vtkAbstractArray *inputArray = ods->GetAbstractArray(this->InputArrayName);
  if (!inputArray)
  {
    return 1;
  }
  vtkAbstractArray *outputArray =
      vtkAbstractArray::CreateArray(this->OutputArrayType);
  vtkDataArray *outputDataArray = vtkArrayDownCast<vtkDataArray>(outputArray);
  vtkStringArray *outputStringArray =
      vtkArrayDownCast<vtkStringArray>(outputArray);
  outputArray->SetName(this->OutputArrayName);

  // Are we copying the input array values to the output array before
  // the mapping?
  if(this->PassArray)
  {
    // Make sure the DeepCopy will succeed
    if((inputArray->IsA("vtkDataArray") && outputArray->IsA("vtkDataArray"))
         || (inputArray->IsA("vtkStringArray")
             && outputArray->IsA("vtkStringArray")))
    {
      outputArray->DeepCopy(inputArray);
    }
    else
    {
      vtkIdType numComps = inputArray->GetNumberOfComponents();
      vtkIdType numTuples = inputArray->GetNumberOfTuples();
      outputArray->SetNumberOfComponents(numComps);
      outputArray->SetNumberOfTuples(numTuples);
      for (vtkIdType i = 0; i < numTuples; ++i)
      {
        for (vtkIdType j = 0; j < numComps; ++j)
        {
          outputArray->InsertVariantValue(
            i*numComps + j, inputArray->GetVariantValue(i*numComps + j));
        }
      }
    }
  }
  else
  {
    outputArray->SetNumberOfComponents(inputArray->GetNumberOfComponents());
    outputArray->SetNumberOfTuples(inputArray->GetNumberOfTuples());

    // Fill the output array with a default value
    if(outputDataArray)
    {
      outputDataArray->FillComponent(0, this->FillValue);
    }
  }

  // Use the internal map to set the mapped values in the output array
  vtkIdList* results = vtkIdList::New();
  for(MapBase::iterator i = this->Map->begin(); i != this->Map->end(); ++i)
  {
    inputArray->LookupValue(i->first, results);
    for(vtkIdType j=0; j<results->GetNumberOfIds(); ++j)
    {
      if(outputDataArray)
      {
        outputDataArray->SetComponent(results->GetId(j), 0, i->second.ToDouble());
      }
      else if(outputStringArray)
      {
        outputStringArray->SetValue(results->GetId(j), i->second.ToString());
      }
    }
  }

  // Finally, add the array to the appropriate vtkDataSetAttributes
  ods->AddArray(outputArray);

  results->Delete();
  outputArray->Delete();

  return 1;
}

int vtkMapArrayValues::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // This algorithm may accept a vtkPointSet or vtkGraph.
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

void vtkMapArrayValues::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Input array name: ";
  if (this->InputArrayName)
  {
    os << this->InputArrayName << endl;
  }
  else
  {
    os << "(none)" << endl;
  }

  os << indent << "Output array name: ";
  if (this->OutputArrayName)
  {
    os << this->OutputArrayName << endl;
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "Field type: " << this->FieldType << endl;
  os << indent << "Output array type: " << this->OutputArrayType << endl;
  os << indent << "PassArray: " << this->PassArray << endl;
  os << indent << "FillValue: " << this->FillValue << endl;
}
