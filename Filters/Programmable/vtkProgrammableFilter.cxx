/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProgrammableFilter.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkGraph.h"
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

vtkStandardNewMacro(vtkProgrammableFilter);

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
    if (vtkDataSet::SafeDownCast(objInput))
    {
      vtkDataSet* dsInput = vtkDataSet::SafeDownCast(objInput);
      vtkDataSet* dsOutput = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (dsInput && dsOutput && dsInput->GetDataObjectType() == dsOutput->GetDataObjectType())
      {
        if (this->CopyArrays)
        {
          dsOutput->ShallowCopy(dsInput);
        }
        else
        {
          dsOutput->CopyStructure(dsInput);
        }
      }
    }
    if (vtkGraph::SafeDownCast(objInput))
    {
      vtkGraph* graphInput = vtkGraph::SafeDownCast(objInput);
      vtkGraph* graphOutput = vtkGraph::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (graphInput && graphOutput &&
        graphInput->GetDataObjectType() == graphOutput->GetDataObjectType())
      {
        if (this->CopyArrays)
        {
          graphOutput->ShallowCopy(graphInput);
        }
        else
        {
          graphOutput->CopyStructure(graphInput);
        }
      }
    }
    if (vtkMolecule::SafeDownCast(objInput))
    {
      vtkMolecule* molInput = vtkMolecule::SafeDownCast(objInput);
      vtkMolecule* molOutput =
        vtkMolecule::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (molInput && molOutput && molInput->GetDataObjectType() == molOutput->GetDataObjectType())
      {
        if (this->CopyArrays)
        {
          molOutput->ShallowCopy(molInput);
        }
        else
        {
          molOutput->CopyStructure(molInput);
        }
      }
    }
    if (vtkTable::SafeDownCast(objInput))
    {
      vtkTable* tableInput = vtkTable::SafeDownCast(objInput);
      vtkTable* tableOutput = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (tableInput && tableOutput &&
        tableInput->GetDataObjectType() == tableOutput->GetDataObjectType())
      {
        if (this->CopyArrays)
        {
          tableOutput->ShallowCopy(tableInput);
        }
      }
    }
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
            if (this->CopyArrays)
            {
              oblock->ShallowCopy(iblock);
            }
            else
            {
              vtkDataSet* iblockDS = vtkDataSet::SafeDownCast(iblock);
              vtkDataSet* oblockDS = vtkDataSet::SafeDownCast(oblock);
              if (iblockDS && oblockDS)
              {
                oblockDS->CopyStructure(iblockDS);
              }
            }
          }
          cdsOutput->SetDataSet(iter, oblock);
          oblock->Delete();
        }
        iter->Delete();
      }
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
  // This algorithm may accept a vtkDataSet or vtkGraph or vtkTable.
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMolecule");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

void vtkProgrammableFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CopyArrays: " << this->CopyArrays << endl;
}
