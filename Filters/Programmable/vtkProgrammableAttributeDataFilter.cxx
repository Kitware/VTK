// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkProgrammableAttributeDataFilter.h"

#include "vtkCellData.h"
#include "vtkDataSetCollection.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkProgrammableAttributeDataFilter);

vtkProgrammableAttributeDataFilter::vtkProgrammableAttributeDataFilter()
{
  this->ExecuteMethod = nullptr;
  this->ExecuteMethodArg = nullptr;
  this->ExecuteMethodArgDelete = nullptr;
  this->InputList = vtkDataSetCollection::New();
}

vtkProgrammableAttributeDataFilter::~vtkProgrammableAttributeDataFilter()
{
  // delete the current arg if there is one and a delete meth
  if ((this->ExecuteMethodArg) && (this->ExecuteMethodArgDelete))
  {
    (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
  }
  if (this->InputList)
  {
    this->InputList->Delete();
    this->InputList = nullptr;
  }
}

// Add a dataset to the list of data to process.
void vtkProgrammableAttributeDataFilter::AddInput(vtkDataSet* ds)
{
  if (this->InputList->IndexOfFirstOccurence(ds) < 0)
  {
    this->Modified();
    this->InputList->AddItem(ds);
  }
}

// Remove a dataset from the list of data to process.
void vtkProgrammableAttributeDataFilter::RemoveInput(vtkDataSet* ds)
{
  if (this->InputList->IndexOfFirstOccurence(ds) >= 0)
  {
    this->Modified();
    this->InputList->RemoveItem(ds);
  }
}

// Specify the function to use to operate on the point attribute data. Note
// that the function takes a single (void *) argument.
void vtkProgrammableAttributeDataFilter::SetExecuteMethod(void (*f)(void*), void* arg)
{
  if (f != this->ExecuteMethod || arg != this->ExecuteMethodArg)
  {
    // delete the current arg if there is one and a delete meth
    if ((this->ExecuteMethodArg) && (this->ExecuteMethodArgDelete))
    {
      (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
    }
    this->ExecuteMethod = f;
    this->ExecuteMethodArg = arg;
    this->Modified();
  }
}

// Set the arg delete method. This is used to free user memory.
void vtkProgrammableAttributeDataFilter::SetExecuteMethodArgDelete(void (*f)(void*))
{
  if (f != this->ExecuteMethodArgDelete)
  {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
  }
}

int vtkProgrammableAttributeDataFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Executing programmable point data filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  // Output data is the same as input data by default.
  output->GetCellData()->PassData(input->GetCellData());
  output->GetPointData()->PassData(input->GetPointData());

  // Now invoke the procedure, if specified.
  if (this->ExecuteMethod != nullptr)
  {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
  }

  return 1;
}

void vtkProgrammableAttributeDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input DataSets:\n";
  this->InputList->PrintSelf(os, indent.GetNextIndent());

  if (this->ExecuteMethod)
  {
    os << indent << "An ExecuteMethod has been defined\n";
  }
  else
  {
    os << indent << "An ExecuteMethod has NOT been defined\n";
  }
}

//------------------------------------------------------------------------------
void vtkProgrammableAttributeDataFilter ::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->InputList, "InputList");
}
VTK_ABI_NAMESPACE_END
