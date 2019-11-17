/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProgrammableSource.h"

#include "vtkCommand.h"
#include "vtkExecutive.h"
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

vtkStandardNewMacro(vtkProgrammableSource);

// Construct programmable filter with empty execute method.
vtkProgrammableSource::vtkProgrammableSource()
{
  this->ExecuteMethod = nullptr;
  this->ExecuteMethodArg = nullptr;
  this->ExecuteMethodArgDelete = nullptr;
  this->RequestInformationMethod = nullptr;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(8);

  vtkDataObject* output;
  output = vtkPolyData::New();
  this->GetExecutive()->SetOutputData(0, output);
  output->Delete();

  output = vtkStructuredPoints::New();
  this->GetExecutive()->SetOutputData(1, output);
  output->Delete();

  output = vtkStructuredGrid::New();
  this->GetExecutive()->SetOutputData(2, output);
  output->Delete();

  output = vtkUnstructuredGrid::New();
  this->GetExecutive()->SetOutputData(3, output);
  output->Delete();

  output = vtkRectilinearGrid::New();
  this->GetExecutive()->SetOutputData(4, output);
  output->Delete();

  output = vtkGraph::New();
  this->GetExecutive()->SetOutputData(5, output);
  output->Delete();

  output = vtkMolecule::New();
  this->GetExecutive()->SetOutputData(6, output);
  output->Delete();

  output = vtkTable::New();
  this->GetExecutive()->SetOutputData(7, output);
  output->Delete();

  this->RequestedDataType = VTK_POLY_DATA;
}

vtkProgrammableSource::~vtkProgrammableSource()
{
  // delete the current arg if there is one and a delete meth
  if ((this->ExecuteMethodArg) && (this->ExecuteMethodArgDelete))
  {
    (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
  }
}

// Specify the function to use to generate the source data. Note
// that the function takes a single (void *) argument.
void vtkProgrammableSource::SetExecuteMethod(void (*f)(void*), void* arg)
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
void vtkProgrammableSource::SetExecuteMethodArgDelete(void (*f)(void*))
{
  if (f != this->ExecuteMethodArgDelete)
  {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
  }
}

void vtkProgrammableSource::SetRequestInformationMethod(void (*f)(void*))
{
  if (f != this->RequestInformationMethod)
  {
    this->RequestInformationMethod = f;
    this->Modified();
  }
}

// Get the output as a concrete type. This method is typically used by the
// writer of the source function to get the output as a particular type (i.e.,
// it essentially does type casting). It is the users responsibility to know
// the correct type of the output data.
vtkPolyData* vtkProgrammableSource::GetPolyDataOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = VTK_POLY_DATA;
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(0));
}

// Get the output as a concrete type.
vtkStructuredPoints* vtkProgrammableSource::GetStructuredPointsOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = VTK_STRUCTURED_POINTS;
  return vtkStructuredPoints::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

// Get the output as a concrete type.
vtkStructuredGrid* vtkProgrammableSource::GetStructuredGridOutput()
{
  if (this->GetNumberOfOutputPorts() < 5)
  {
    return nullptr;
  }

  this->RequestedDataType = VTK_STRUCTURED_GRID;
  return vtkStructuredGrid::SafeDownCast(this->GetExecutive()->GetOutputData(2));
}

// Get the output as a concrete type.
vtkUnstructuredGrid* vtkProgrammableSource::GetUnstructuredGridOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = VTK_UNSTRUCTURED_GRID;
  return vtkUnstructuredGrid::SafeDownCast(this->GetExecutive()->GetOutputData(3));
}

// Get the output as a concrete type.
vtkRectilinearGrid* vtkProgrammableSource::GetRectilinearGridOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = VTK_RECTILINEAR_GRID;
  return vtkRectilinearGrid::SafeDownCast(this->GetExecutive()->GetOutputData(4));
}

// Get the output as a concrete type.
vtkGraph* vtkProgrammableSource::GetGraphOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = VTK_GRAPH;
  return vtkGraph::SafeDownCast(this->GetExecutive()->GetOutputData(5));
}

// Get the output as a concrete type.
vtkMolecule* vtkProgrammableSource::GetMoleculeOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = VTK_MOLECULE;
  return vtkMolecule::SafeDownCast(this->GetExecutive()->GetOutputData(6));
}

// Get the output as a concrete type.
vtkTable* vtkProgrammableSource::GetTableOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = VTK_TABLE;
  return vtkTable::SafeDownCast(this->GetExecutive()->GetOutputData(7));
}

int vtkProgrammableSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkDebugMacro(<< "Executing programmable source");

  // Now invoke the procedure, if specified.
  if (this->ExecuteMethod != nullptr)
  {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
  }

  return 1;
}

int vtkProgrammableSource::RequestDataObject(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo;
  vtkDataObject* output = nullptr;
  switch (this->RequestedDataType)
  {
    case VTK_POLY_DATA:
      outInfo = outputVector->GetInformationObject(0);
      if (!outInfo)
      {
        output = vtkPolyData::New();
      }
      else
      {
        output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = vtkPolyData::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(0, output);
      output->Delete();
      break;
    case VTK_STRUCTURED_POINTS:
      outInfo = outputVector->GetInformationObject(1);
      if (!outInfo)
      {
        output = vtkStructuredPoints::New();
      }
      else
      {
        output = vtkStructuredPoints::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = vtkStructuredPoints::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(1, output);
      output->Delete();
      break;
    case VTK_STRUCTURED_GRID:
      outInfo = outputVector->GetInformationObject(2);
      if (!outInfo)
      {
        output = vtkStructuredGrid::New();
      }
      else
      {
        output = vtkStructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = vtkStructuredGrid::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(2, output);
      output->Delete();
      break;
    case VTK_UNSTRUCTURED_GRID:
      outInfo = outputVector->GetInformationObject(3);
      if (!outInfo)
      {
        output = vtkUnstructuredGrid::New();
      }
      else
      {
        output = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = vtkUnstructuredGrid::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(3, output);
      output->Delete();
      break;
    case VTK_RECTILINEAR_GRID:
      outInfo = outputVector->GetInformationObject(4);
      if (!outInfo)
      {
        output = vtkRectilinearGrid::New();
      }
      else
      {
        output = vtkRectilinearGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = vtkRectilinearGrid::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(4, output);
      output->Delete();
      break;
    case VTK_GRAPH:
      outInfo = outputVector->GetInformationObject(5);
      if (!outInfo)
      {
        output = vtkGraph::New();
      }
      else
      {
        output = vtkGraph::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = vtkGraph::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(5, output);
      output->Delete();
      break;
    case VTK_MOLECULE:
      outInfo = outputVector->GetInformationObject(6);
      if (!outInfo)
      {
        output = vtkMolecule::New();
      }
      else
      {
        output = vtkMolecule::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = vtkMolecule::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(6, output);
      output->Delete();
      break;
    case VTK_TABLE:
      outInfo = outputVector->GetInformationObject(7);
      if (!outInfo)
      {
        output = vtkTable::New();
      }
      else
      {
        output = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = vtkTable::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(7, output);
      output->Delete();
      break;
    default:
      return 0;
  }
  return 1;
}

int vtkProgrammableSource::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  vtkDebugMacro(<< "requesting information");

  // Now invoke the procedure, if specified.
  if (this->RequestInformationMethod != nullptr)
  {
    (*this->RequestInformationMethod)(nullptr);
  }

  return 1;
}
