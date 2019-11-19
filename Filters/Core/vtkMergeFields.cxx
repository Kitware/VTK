/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeFields.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMergeFields.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkMergeFields);

char vtkMergeFields::FieldLocationNames[3][12] = { "DATA_OBJECT", "POINT_DATA", "CELL_DATA" };

typedef vtkMergeFields::Component Component;

vtkMergeFields::vtkMergeFields()
{
  this->FieldName = nullptr;
  this->FieldLocation = -1;
  this->NumberOfComponents = 0;

  this->Head = nullptr;
  this->Tail = nullptr;
}

vtkMergeFields::~vtkMergeFields()
{
  delete[] this->FieldName;
  this->FieldName = nullptr;
  this->DeleteAllComponents();
}

void vtkMergeFields::SetOutputField(const char* name, int fieldLoc)
{
  if (!name)
  {
    return;
  }

  if ((fieldLoc != vtkMergeFields::DATA_OBJECT) && (fieldLoc != vtkMergeFields::POINT_DATA) &&
    (fieldLoc != vtkMergeFields::CELL_DATA))
  {
    vtkErrorMacro("The source for the field is wrong.");
    return;
  }

  this->Modified();
  this->FieldLocation = fieldLoc;

  delete[] this->FieldName;
  this->FieldName = new char[strlen(name) + 1];
  strcpy(this->FieldName, name);
}

void vtkMergeFields::SetOutputField(const char* name, const char* fieldLoc)
{
  if (!name || !fieldLoc)
  {
    return;
  }

  int numFieldLocs = 3;
  int i;

  // Convert fieldLoc to int an call the other SetOutputField()
  int loc = -1;
  for (i = 0; i < numFieldLocs; i++)
  {
    if (!strcmp(fieldLoc, FieldLocationNames[i]))
    {
      loc = i;
      break;
    }
  }
  if (loc == -1)
  {
    vtkErrorMacro("Location for the field is invalid.");
    return;
  }

  this->SetOutputField(name, loc);
}

void vtkMergeFields::Merge(int component, const char* arrayName, int sourceComp)
{
  if (!arrayName)
  {
    return;
  }

  this->Modified();
  Component* comp = this->FindComponent(component);
  if (comp)
  {
    // If component already exists, replace information
    comp->SetName(arrayName);
    comp->SourceIndex = sourceComp;
  }
  else
  {
    // otherwise create a new one
    comp = new Component;
    comp->SetName(arrayName);
    comp->Index = component;
    comp->SourceIndex = sourceComp;
    this->AddComponent(comp);
  }
}

int vtkMergeFields::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // This has to be here because it initialized all field datas.
  output->CopyStructure(input);

  // Pass all. (data object's field data is passed by the
  // superclass after this method)
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  vtkFieldData* fd = nullptr;
  vtkFieldData* outputFD = nullptr;
  Component* cur = this->GetFirst();
  Component* before;

  if (!cur)
  {
    return 1;
  }

  // Get the input and output field data
  if (this->FieldLocation == vtkMergeFields::DATA_OBJECT)
  {
    fd = input->GetFieldData();
    outputFD = output->GetFieldData();
  }
  else if (this->FieldLocation == vtkMergeFields::POINT_DATA)
  {
    fd = input->GetPointData();
    outputFD = output->GetPointData();
  }
  else if (this->FieldLocation == vtkMergeFields::CELL_DATA)
  {
    fd = input->GetCellData();
    outputFD = output->GetCellData();
  }

  if (!fd || !outputFD)
  {
    vtkErrorMacro("No field data in vtkDataObject.");
    return 1;
  }

  // Check if the data types of the input fields are the same
  // Otherwise warn the user.
  // Check if the number of tuples are the same for all arrays.
  vtkDataArray* inputArray;
  int dataType = -1;
  int sameDataType = 1;
  int numTuples = -1;
  int sameNumTuples = 1;
  do
  {
    before = cur;
    cur = cur->Next;
    inputArray = fd->GetArray(before->FieldName);
    if (!inputArray)
    {
      continue;
    }
    else
    {
      if (dataType == -1)
      {
        dataType = inputArray->GetDataType();
      }
      else
      {
        if (inputArray->GetDataType() != dataType)
        {
          sameDataType = 0;
        }
      }
      if (numTuples == -1)
      {
        numTuples = inputArray->GetNumberOfTuples();
      }
      else
      {
        if (inputArray->GetNumberOfTuples() != numTuples)
        {
          sameNumTuples = 0;
        }
      }
    }
  } while (cur);
  if (!sameNumTuples)
  {
    vtkErrorMacro("The number of tuples in the input arrays do not match.");
    return 1;
  }
  if (dataType == -1)
  {
    vtkErrorMacro("No input array(s) were found.");
    return 1;
  }
  vtkDataArray* outputArray;
  if (!sameDataType)
  {
    vtkWarningMacro("The input data types do not match. The output will be "
      << "float. This will potentially cause accuracy and speed.");
    outputArray = vtkFloatArray::New();
  }
  else
  {
    outputArray = vtkDataArray::CreateDataArray(dataType);
  }

  if (this->NumberOfComponents <= 0)
  {
    vtkErrorMacro("NumberOfComponents has be set prior to the execution of "
                  "this filter");
  }

  outputArray->SetNumberOfComponents(this->NumberOfComponents);
  outputArray->SetNumberOfTuples(numTuples);
  outputArray->SetName(this->FieldName);

  // Merge
  cur = this->GetFirst();
  do
  {
    before = cur;
    cur = cur->Next;
    inputArray = fd->GetArray(before->FieldName);
    if (inputArray)
    {
      if (!this->MergeArray(inputArray, outputArray, before->SourceIndex, before->Index))
      {
        outputArray->Delete();
        return 1;
      }
    }
    else
    {
      if (before->FieldName)
      {
        vtkWarningMacro("Input array " << before->FieldName << " does not exist.");
      }
      continue;
    }
  } while (cur);
  outputFD->AddArray(outputArray);
  outputArray->Delete();

  return 1;
}

namespace {

struct MergeFieldsWorker
{
  template <typename SrcArrayT, typename DstArrayT>
  void operator()(SrcArrayT* input, DstArrayT* output,
                  int inComp, int outComp) const
  {
    const auto srcRange = vtk::DataArrayTupleRange(input);
    auto dstRange = vtk::DataArrayTupleRange(output);

    for (vtkIdType tupleIdx = 0; tupleIdx < srcRange.size(); ++tupleIdx)
    {
      dstRange[tupleIdx][outComp] = srcRange[tupleIdx][inComp];
    }
  }
};

} // end anon namespace

int vtkMergeFields::MergeArray(vtkDataArray* in, vtkDataArray* out, int inComp, int outComp)
{
  if ((inComp < 0) || (inComp > in->GetNumberOfComponents()) || (outComp < 0) ||
    (outComp > out->GetNumberOfComponents()))
  {
    vtkErrorMacro("Invalid component. Can not merge.");
    return 0;
  }

  // If data types match, use templated, fast method
  using Dispatcher = vtkArrayDispatch::Dispatch2SameValueType;

  MergeFieldsWorker worker;
  if (!Dispatcher::Execute(in, out, worker, inComp, outComp))
  { // otherwise fallback to vtkDataArray API:
    worker(in, out, inComp, outComp);
  }

  return 1;
}

// linked list methods
void vtkMergeFields::AddComponent(Component* op)
{
  op->Next = nullptr;

  if (!this->Head)
  {
    this->Head = op;
    this->Tail = op;
    return;
  }
  this->Tail->Next = op;
  this->Tail = op;
}

Component* vtkMergeFields::FindComponent(int index)
{
  Component* cur = this->GetFirst();
  if (!cur)
  {
    return nullptr;
  }

  if (cur->Index == index)
  {
    return cur;
  }
  while (cur->Next)
  {
    if (cur->Next->Index == index)
    {
      return cur->Next;
    }
    cur = cur->Next;
  }
  return nullptr;
}

void vtkMergeFields::DeleteAllComponents()
{
  Component* cur = this->GetFirst();
  if (!cur)
  {
    return;
  }
  Component* before;
  do
  {
    before = cur;
    cur = cur->Next;
    delete before;
  } while (cur);
  this->Head = nullptr;
  this->Tail = nullptr;
}

void vtkMergeFields::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Field name: ";
  if (this->FieldName)
  {
    os << this->FieldName << endl;
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "Field location: " << this->FieldLocation << endl;
  os << indent << "Linked list head: " << this->Head << endl;
  os << indent << "Linked list tail: " << this->Tail << endl;
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  os << indent << "Components: " << endl;
  this->PrintAllComponents(os, indent.GetNextIndent());
}

void vtkMergeFields::PrintComponent(Component* op, ostream& os, vtkIndent indent)
{
  os << indent << "Field name: " << op->FieldName << endl;
  os << indent << "Component index: " << op->Index << endl;
  os << indent << "Source component index: " << op->SourceIndex << endl;
}

void vtkMergeFields::PrintAllComponents(ostream& os, vtkIndent indent)
{
  Component* cur = this->GetFirst();
  if (!cur)
  {
    return;
  }
  Component* before;
  do
  {
    before = cur;
    cur = cur->Next;
    os << endl;
    this->PrintComponent(before, os, indent);
  } while (cur);
}
