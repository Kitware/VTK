// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkProgrammableFilter.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkGraph.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMolecule.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkProgrammableFilter);
VTK_ABI_NAMESPACE_END

namespace details
{
VTK_ABI_NAMESPACE_BEGIN
// CopyStructure is not defined at vtkDataObject level.
// Use template to downcast and forward call.
template <class DataType>
void copyStructure(vtkDataObject* dataIn, vtkDataObject* dataOut)
{
  DataType* in = DataType::SafeDownCast(dataIn);
  DataType* out = DataType::SafeDownCast(dataOut);
  if (in && out)
  {
    out->CopyStructure(in);
  }
}

void initializeOutput(vtkDataObject* objInput, vtkDataObject* objOutput, bool copyArrays)
{
  if (objInput && objOutput && objInput->GetDataObjectType() == objOutput->GetDataObjectType())
  {
    if (copyArrays)
    {
      // Shallow copy is defined at vtkDataObject level. If output is of same concrete type than
      // input and copy if data arrays are requested, we can directly call ShallowCopy.
      objOutput->ShallowCopy(objInput);
      return;
    }

    // if not copyArrays, copy only the structure for relevant classes.
    if (vtkDataSet::SafeDownCast(objInput))
    {
      details::copyStructure<vtkDataSet>(objInput, objOutput);
    }
    else if (vtkGraph::SafeDownCast(objInput))
    {
      details::copyStructure<vtkGraph>(objInput, objOutput);
    }
    else if (vtkHyperTreeGrid::SafeDownCast(objInput))
    {
      details::copyStructure<vtkHyperTreeGrid>(objInput, objOutput);
    }
  }
}
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN

// Construct programmable filter with empty execute method.
vtkProgrammableFilter::vtkProgrammableFilter()
{
  this->ExecuteMethod = nullptr;
  this->ExecuteMethodArg = nullptr;
  this->ExecuteMethodArgDelete = nullptr;
  this->CopyArrays = false;
}

vtkProgrammableFilter::~vtkProgrammableFilter()
{
  // delete the current arg if there is one and a delete meth
  if ((this->ExecuteMethodArg) && (this->ExecuteMethodArgDelete))
  {
    (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
  }
}

// Get the input as a concrete type. This method is typically used by the
// writer of the filter function to get the input as a particular type (i.e.,
// it essentially does type casting). It is the users responsibility to know
// the correct type of the input data.
vtkPolyData* vtkProgrammableFilter::GetPolyDataInput()
{
  return static_cast<vtkPolyData*>(this->GetInput());
}

// Get the input as a concrete type.
vtkStructuredPoints* vtkProgrammableFilter::GetStructuredPointsInput()
{
  return static_cast<vtkStructuredPoints*>(this->GetInput());
}

// Get the input as a concrete type.
vtkStructuredGrid* vtkProgrammableFilter::GetStructuredGridInput()
{
  return static_cast<vtkStructuredGrid*>(this->GetInput());
}

// Get the input as a concrete type.
vtkUnstructuredGrid* vtkProgrammableFilter::GetUnstructuredGridInput()
{
  return static_cast<vtkUnstructuredGrid*>(this->GetInput());
}

// Get the input as a concrete type.
vtkRectilinearGrid* vtkProgrammableFilter::GetRectilinearGridInput()
{
  return static_cast<vtkRectilinearGrid*>(this->GetInput());
}

// Get the input as a concrete type.
vtkGraph* vtkProgrammableFilter::GetGraphInput()
{
  return static_cast<vtkGraph*>(this->GetInput());
}

// Get the input as a concrete type.
vtkMolecule* vtkProgrammableFilter::GetMoleculeInput()
{
  return static_cast<vtkMolecule*>(this->GetInput());
}

// Get the input as a concrete type.
vtkTable* vtkProgrammableFilter::GetTableInput()
{
  return static_cast<vtkTable*>(this->GetInput());
}

// Get the input as a concrete type.
vtkHyperTreeGrid* vtkProgrammableFilter::GetHyperTreeGridInput()
{
  return static_cast<vtkHyperTreeGrid*>(this->GetInput());
}

// Specify the function to use to operate on the point attribute data. Note
// that the function takes a single (void *) argument.
void vtkProgrammableFilter::SetExecuteMethod(void (*f)(void*), void* arg)
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
void vtkProgrammableFilter::SetExecuteMethodArgDelete(void (*f)(void*))
{
  if (f != this->ExecuteMethodArgDelete)
  {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
  }
}

int vtkProgrammableFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = nullptr;
  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    inInfo = inputVector[0]->GetInformationObject(0);
  }
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  if (inInfo)
  {
    vtkDataObject* objInput = inInfo->Get(vtkDataObject::DATA_OBJECT());
    vtkDataObject* objOutput = outInfo->Get(vtkDataObject::DATA_OBJECT());

    if (vtkCompositeDataSet::SafeDownCast(objInput))
    {
      vtkCompositeDataSet* cdsInput = vtkCompositeDataSet::SafeDownCast(objInput);
      vtkCompositeDataSet* cdsOutput =
        vtkCompositeDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (cdsInput && cdsOutput && cdsInput->GetDataObjectType() == cdsOutput->GetDataObjectType())
      {
        cdsOutput->CopyStructure(cdsInput);
        vtkCompositeDataIterator* iter = cdsInput->NewIterator();
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
          vtkDataObject* iblock = iter->GetCurrentDataObject();
          vtkDataObject* oblock = iblock->NewInstance();
          if (iblock)
          {
            details::initializeOutput(iblock, oblock, this->CopyArrays);
          }
          cdsOutput->SetDataSet(iter, oblock);
          oblock->Delete();
        }
        iter->Delete();
      }
    }
    else
    {
      details::initializeOutput(objInput, objOutput, this->CopyArrays);
    }
  }

  vtkDebugMacro(<< "Executing programmable filter");

  // Now invoke the procedure, if specified.
  if (this->ExecuteMethod != nullptr)
  {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
  }

  return 1;
}

int vtkProgrammableFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

void vtkProgrammableFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CopyArrays: " << this->CopyArrays << endl;
}
VTK_ABI_NAMESPACE_END
