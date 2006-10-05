/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeFieldAggregator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTreeFieldAggregator.h"
#include "vtkTreeDFSIterator.h"

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkIntArray.h>
#include <vtkDoubleArray.h>
#include <vtkStringArray.h>
#include <vtkIdList.h>
#include <math.h>
#include "vtkTree.h"
#include "vtkGraph.h"
#include "vtkVariantArray.h"

vtkCxxRevisionMacro(vtkTreeFieldAggregator, "1.1");
vtkStandardNewMacro(vtkTreeFieldAggregator);

vtkTreeFieldAggregator::vtkTreeFieldAggregator():MinValue(0.0)
{
  this->MinValue = 0;
  this->Field = 0;
  this->SetLeafNodeUnitSize(true);
  this->SetLogScale(false);
}

vtkTreeFieldAggregator::~vtkTreeFieldAggregator()
{
  this->SetField(0);
}

int vtkTreeFieldAggregator::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  // get the input and output
  vtkTree *input = vtkTree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree *output = vtkTree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Shallow copy the input
  output->ShallowCopy(input);

  // Check for the existance of the field to be aggregated
  if (!output->GetNodeData()->HasArray(this->Field))
    {
    //vtkWarningMacro(<< "The field " << this->Field << " was NOT found! Setting LeafNodeUnitSize = true");
    this->LeafNodeUnitSize = true;
    }
    
  // Extract the field from the tree
  vtkAbstractArray* arr;
  if (this->LeafNodeUnitSize)
    {
    arr = vtkIntArray::New();
    arr->SetNumberOfTuples(output->GetNumberOfNodes());
    arr->SetName(this->Field);
    for (vtkIdType i = 0; i < arr->GetNumberOfTuples(); i++)
      {
      vtkIntArray::SafeDownCast(arr)->SetTuple1(i, 1);
      }
    output->GetNodeData()->AddArray(arr);
    arr->Delete();
    }
  else
    {
    vtkAbstractArray* oldArr = output->GetNodeData()->GetAbstractArray(this->Field);
    if (oldArr->GetNumberOfComponents() != 1)
      {
      vtkErrorMacro(<< "The field " << this->Field << " must have one component per tuple");
      }
    if (oldArr->IsA("vtkStringArray"))
      {
      vtkDoubleArray* doubleArr = vtkDoubleArray::New();
      doubleArr->Resize(oldArr->GetNumberOfTuples());
      for (vtkIdType i = 0; i < oldArr->GetNumberOfTuples(); i++)
        {
        doubleArr->InsertNextTuple1(vtkTreeFieldAggregator::GetDoubleValue(oldArr, i));
        }
      arr = doubleArr;
      }
    else
      {
      arr = vtkAbstractArray::CreateArray(oldArr->GetDataType());
      arr->DeepCopy(oldArr);
      }
    arr->SetName(this->Field);

    // We would like to do just perform output->GetNodeData()->RemoveArray(this->Field),
    // but because of a bug in vtkDataSetAttributes::RemoveArray(char*), we need to do it this way.
    vtkFieldData* data = vtkFieldData::SafeDownCast(output->GetNodeData());
    data->RemoveArray(this->Field);

    output->GetNodeData()->AddArray(arr);
    arr->Delete();
    }

  vtkTreeDFSIterator* dfs = vtkTreeDFSIterator::New();
  vtkIdType nchildren;
  const vtkIdType* children;
  dfs->SetTree(output);
  dfs->SetMode(vtkTreeDFSIterator::FINISH);
  while (dfs->HasNext())
    {
    vtkIdType node = dfs->Next();
    output->GetChildren(node, nchildren, children);

    double value = 0;
    if (nchildren == 0)
      {
      value = vtkTreeFieldAggregator::GetDoubleValue(arr, node);
      if (this->LogScale)
        {
        value = log10(value);
        if (value < this->MinValue)
          {
          value = this->MinValue;
          }
        }
      }
    else
      {
      for (vtkIdType i = 0; i < nchildren; i++)
        {
        value += vtkTreeFieldAggregator::GetDoubleValue(arr, children[i]);
        }
      }
    vtkTreeFieldAggregator::SetDoubleValue(arr, node, value);
    }
  dfs->Delete();

  return 1;
}

void vtkTreeFieldAggregator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Field: " << this->Field << endl;
  os << indent << "LeafNodeUnitSize: " << (this->LeafNodeUnitSize ? "on" : "off") << endl;
  os << indent << "MinValue: " << this->MinValue << endl;
  os << indent << "LogScale: " << (this->LogScale? "on" : "off") << endl;
}

double vtkTreeFieldAggregator::GetDoubleValue(vtkAbstractArray* arr, vtkIdType id)
{
  if (arr->IsA("vtkDataArray"))
    {
    double d = vtkDataArray::SafeDownCast(arr)->GetTuple1(id);
    if (d < this->MinValue)
      {
      return MinValue;
      }
    return d;
    }
  else if (arr->IsA("vtkVariantArray"))
    {
    vtkVariant v = vtkVariantArray::SafeDownCast(arr)->GetValue(id);
    if (!v.IsValid())
      {
      return this->MinValue;
      }
    bool ok;
    double d = v.ToDouble(&ok);
    if (!ok)
      {
      return this->MinValue;
      }
    if (d < this->MinValue)
      {
      return MinValue;
      }
    return d;
    }
  else if (arr->IsA("vtkStringArray"))
    {
    vtkVariant v(vtkStringArray::SafeDownCast(arr)->GetValue(id));
    bool ok;
    double d = v.ToDouble(&ok);
    if (!ok)
      {
      return this->MinValue;
      }
    if (d < this->MinValue)
      {
      return MinValue;
      }
    return d;
    }
  return this->MinValue;
}

void vtkTreeFieldAggregator::SetDoubleValue(vtkAbstractArray* arr, vtkIdType id, double value)
{
  if (arr->IsA("vtkDataArray"))
    {
    vtkDataArray::SafeDownCast(arr)->SetTuple1(id, value);
    }
  else if (arr->IsA("vtkVariantArray"))
    {
    vtkVariantArray::SafeDownCast(arr)->SetValue(id, vtkVariant(value));
    }
  else if (arr->IsA("vtkStringArray"))
    {
    vtkStringArray::SafeDownCast(arr)->SetValue(id, vtkVariant(value).ToString());
    }
}
