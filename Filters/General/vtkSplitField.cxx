/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplitField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSplitField.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include <cctype>

vtkStandardNewMacro(vtkSplitField);

char vtkSplitField::FieldLocationNames[3][12] = { "DATA_OBJECT", "POINT_DATA", "CELL_DATA" };

char vtkSplitField::AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][10] = { { 0 } };

typedef vtkSplitField::Component Component;

vtkSplitField::vtkSplitField()
{
  this->FieldName = nullptr;
  this->FieldLocation = -1;
  this->AttributeType = -1;
  this->FieldType = -1;

  this->Head = nullptr;
  this->Tail = nullptr;

  // convert the attribute names to uppercase for local use
  if (vtkSplitField::AttributeNames[0][0] == 0)
  {
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; i++)
    {
      int l = static_cast<int>(strlen(vtkDataSetAttributes::GetAttributeTypeAsString(i)));
      for (int c = 0; c < l && c < 10; c++)
      {
        vtkSplitField::AttributeNames[i][c] =
          toupper(vtkDataSetAttributes::GetAttributeTypeAsString(i)[c]);
      }
    }
  }
}

vtkSplitField::~vtkSplitField()
{
  delete[] this->FieldName;
  this->FieldName = nullptr;
  this->DeleteAllComponents();
}

void vtkSplitField::SetInputField(const char* name, int fieldLoc)
{
  if (!name)
  {
    return;
  }

  if ((fieldLoc != vtkSplitField::DATA_OBJECT) && (fieldLoc != vtkSplitField::POINT_DATA) &&
    (fieldLoc != vtkSplitField::CELL_DATA))
  {
    vtkErrorMacro("The source for the field is wrong.");
    return;
  }

  this->Modified();
  this->FieldLocation = fieldLoc;
  this->FieldType = vtkSplitField::NAME;

  delete[] this->FieldName;
  this->FieldName = new char[strlen(name) + 1];
  strcpy(this->FieldName, name);
}

void vtkSplitField::SetInputField(int attributeType, int fieldLoc)
{
  if ((fieldLoc != vtkSplitField::POINT_DATA) && (fieldLoc != vtkSplitField::CELL_DATA))
  {
    vtkErrorMacro("The source for the field is wrong.");
    return;
  }

  this->Modified();
  this->FieldLocation = fieldLoc;
  this->FieldType = vtkSplitField::ATTRIBUTE;
  this->AttributeType = attributeType;
}

void vtkSplitField::SetInputField(const char* name, const char* fieldLoc)
{
  if (!name || !fieldLoc)
  {
    return;
  }

  int numAttr = vtkDataSetAttributes::NUM_ATTRIBUTES;
  int numFieldLocs = 3;
  int i;

  // Convert strings to ints and call the appropriate SetInputField()
  int attrType = -1;
  for (i = 0; i < numAttr; i++)
  {
    if (!strcmp(name, AttributeNames[i]))
    {
      attrType = i;
      break;
    }
  }

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

  if (attrType == -1)
  {
    this->SetInputField(name, loc);
  }
  else
  {
    this->SetInputField(attrType, loc);
  }
}

void vtkSplitField::Split(int component, const char* arrayName)
{
  if (!arrayName)
  {
    return;
  }

  this->Modified();
  Component* comp = this->FindComponent(component);
  // If component is already there, just reset the information
  if (comp)
  {
    comp->SetName(arrayName);
  }
  // otherwise add a new one
  else
  {
    comp = new Component;
    comp->SetName(arrayName);
    comp->Index = component;
    this->AddComponent(comp);
  }
}

int vtkSplitField::RequestData(vtkInformation* vtkNotUsed(request),
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

  vtkDataArray* outputArray;
  vtkDataArray* inputArray = nullptr;
  vtkFieldData* fd = nullptr;
  vtkFieldData* outputFD = nullptr;
  Component* cur = this->GetFirst();
  Component* before;

  if (!cur)
  {
    return 1;
  }

  // find the input and output field data
  if (this->FieldLocation == vtkSplitField::DATA_OBJECT)
  {
    fd = input->GetFieldData();
    outputFD = output->GetFieldData();
    if (!fd || !outputFD)
    {
      vtkErrorMacro("No field data in vtkDataObject.");
      return 1;
    }
  }
  else if (this->FieldLocation == vtkSplitField::POINT_DATA)
  {
    fd = input->GetPointData();
    outputFD = output->GetPointData();
  }
  else if (this->FieldLocation == vtkSplitField::CELL_DATA)
  {
    fd = input->GetCellData();
    outputFD = output->GetCellData();
  }

  if (this->FieldType == vtkSplitField::NAME)
  {
    inputArray = fd->GetArray(this->FieldName);
  }
  else if (this->FieldType == vtkSplitField::ATTRIBUTE)
  {
    // If we are working with attributes, we also need to have
    // access to vtkDataSetAttributes methods.
    vtkDataSetAttributes* dsa = vtkDataSetAttributes::SafeDownCast(fd);
    if (!dsa)
    {
      vtkErrorMacro("Sanity check failed, returning.");
      return 1;
    }
    inputArray = dsa->GetAttribute(this->AttributeType);
  }

  if (!inputArray)
  {
    vtkErrorMacro("Sanity check failed, returning.");
    return 1;
  }

  // iterate over all components in the linked list and
  // generate them
  do
  {
    before = cur;
    cur = cur->Next;
    if (before->FieldName)
    {
      outputArray = this->SplitArray(inputArray, before->Index);
      if (outputArray)
      {
        outputArray->SetName(before->FieldName);
        outputFD->AddArray(outputArray);
        outputArray->UnRegister(this);
      }
    }
  } while (cur);

  return 1;
}

namespace
{

struct ExtractComponentWorker
{
  template <typename ArrayT>
  void operator()(ArrayT* input, vtkDataArray* outputDA, vtk::ComponentIdType comp)
  {
    // We know these are the same array type:
    ArrayT* output = vtkArrayDownCast<ArrayT>(outputDA);
    assert(output);

    const auto inTuples = vtk::DataArrayTupleRange(input);
    auto outComps = vtk::DataArrayValueRange<1>(output);

    using TupleCRefT = typename decltype(inTuples)::ConstTupleReferenceType;
    using CompT = typename decltype(outComps)::ValueType;

    std::transform(inTuples.cbegin(), inTuples.cend(), outComps.begin(),
      [&](const TupleCRefT tuple) -> CompT { return tuple[comp]; });
  }
};

} // end anon namespace

vtkDataArray* vtkSplitField::SplitArray(vtkDataArray* da, int component)
{
  if ((component < 0) || (component > da->GetNumberOfComponents()))
  {
    vtkErrorMacro("Invalid component. Can not split");
    return nullptr;
  }

  vtkDataArray* output = da->NewInstance();
  output->SetNumberOfComponents(1);
  vtkIdType numTuples = da->GetNumberOfTuples();
  output->SetNumberOfTuples(numTuples);

  using Dispatcher = vtkArrayDispatch::Dispatch;
  ExtractComponentWorker worker;
  if (!Dispatcher::Execute(da, worker, output, component))
  { // fallback:
    worker(da, output, component);
  }

  return output;
}

// Linked list methods
void vtkSplitField::AddComponent(Component* op)
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

Component* vtkSplitField::FindComponent(int index)
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

void vtkSplitField::DeleteAllComponents()
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

void vtkSplitField::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "Field type: " << this->FieldType << endl;
  os << indent << "Attribute type: " << this->AttributeType << endl;
  os << indent << "Field location: " << this->FieldLocation << endl;
  os << indent << "Linked list head: " << this->Head << endl;
  os << indent << "Linked list tail: " << this->Tail << endl;
  os << indent << "Components: " << endl;
  this->PrintAllComponents(os, indent.GetNextIndent());
}

void vtkSplitField::PrintComponent(Component* op, ostream& os, vtkIndent indent)
{
  os << indent << "Field name: " << op->FieldName << endl;
  os << indent << "Component index: " << op->Index << endl;
}

void vtkSplitField::PrintAllComponents(ostream& os, vtkIndent indent)
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
