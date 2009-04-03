/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayMap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkArrayMap.h"

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
#include "vtkVariant.h"
#include <ctype.h>

#include <vtkstd/map>
#include <vtkstd/utility>

vtkCxxRevisionMacro(vtkArrayMap, "1.1");
vtkStandardNewMacro(vtkArrayMap);

typedef vtkstd::map< vtkVariant, vtkVariant, vtkVariantLessThan > MapBase;
class vtkMapType : public MapBase {};

vtkArrayMap::vtkArrayMap()
{
  this->InputArrayName = 0;
  this->OutputArrayName = 0;
  this->SetOutputArrayName("ArrayMap");
  this->FieldType = vtkArrayMap::POINT_DATA;
  this->OutputArrayType = VTK_INT;
  this->PassArray = 0;
  this->FillValue = -1;

  this->Map = new vtkMapType;
}

vtkArrayMap::~vtkArrayMap()
{
  this->SetInputArrayName(0);
  this->SetOutputArrayName(0);
  delete this->Map;
}

void vtkArrayMap::AddToMap(char *from, int to)
{
  this->Map->insert(vtkstd::make_pair< vtkVariant, vtkVariant >(from, to));

  this->Modified();
}

void vtkArrayMap::AddToMap(int from, int to)
{
  this->Map->insert(vtkstd::make_pair< vtkVariant, vtkVariant >(from, to));

  this->Modified();
}

void vtkArrayMap::AddToMap(int from, char *to)
{
  this->Map->insert(vtkstd::make_pair< vtkVariant, vtkVariant >(from, to));

  this->Modified();
}

void vtkArrayMap::AddToMap(char *from, char *to)
{
  this->Map->insert(vtkstd::make_pair< vtkVariant, vtkVariant >(from, to));

  this->Modified();
}

void vtkArrayMap::AddToMap(vtkVariant from, vtkVariant to)
{
  this->Map->insert(vtkstd::make_pair< vtkVariant, vtkVariant >(from, to));

  this->Modified();
}

void vtkArrayMap::ClearMap()
{
  this->Map->clear();

  this->Modified();
}

int vtkArrayMap::GetMapSize()
{
  return static_cast<int>(this->Map->size());
}

int vtkArrayMap::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  
  if(!this->InputArrayName)
    {
    vtkErrorMacro(<<"Input array not specified.");
    return 0;
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
      case vtkArrayMap::POINT_DATA:
        ods = dsOutput->GetPointData();
        break;
      case vtkArrayMap::CELL_DATA:
        ods = dsOutput->GetCellData();
        break;
      default:
        vtkErrorMacro(<<"Data must be point or cell for vtkDataSet");
        return 0;
      }
    }
  else
    {
    vtkGraph *graphInput = vtkGraph::SafeDownCast(input);
    vtkGraph *graphOutput = vtkGraph::SafeDownCast(output);
    graphOutput->ShallowCopy( graphInput );
    switch (this->FieldType)
      {
      case vtkArrayMap::VERTEX_DATA:
        ods = graphOutput->GetVertexData();
        break;
      case vtkArrayMap::EDGE_DATA:
        ods = graphOutput->GetEdgeData();
        break;
      default:
        vtkErrorMacro(<<"Data must be vertex or edge for vtkGraph");
        return 0;
      }
    }

  vtkAbstractArray *inputArray = ods->GetAbstractArray(this->InputArrayName);
  vtkAbstractArray *outputArray = 
      vtkAbstractArray::CreateArray(this->OutputArrayType);
  vtkDataArray *outputDataArray = vtkDataArray::SafeDownCast(outputArray);
  vtkStringArray *outputStringArray = 
      vtkStringArray::SafeDownCast(outputArray);
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
      vtkErrorMacro(<<"When PassArray is turned on, input and output array "
                    <<"types must be compatible.");
      return 0;
      }
    }
  else
    {
    outputArray->SetNumberOfTuples(inputArray->GetNumberOfTuples());
    outputArray->SetNumberOfComponents(inputArray->GetNumberOfComponents());

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

int vtkArrayMap::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // This algorithm may accept a vtkPointSet or vtkGraph.
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;  
}

void vtkArrayMap::PrintSelf(ostream& os, vtkIndent indent)
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
