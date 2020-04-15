/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPIOReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPIOReader.h"

#include <iostream>

#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkToolkits.h"
#include "vtkUnstructuredGrid.h"

#include "PIOAdaptor.h"

vtkStandardNewMacro(vtkPIOReader);

//----------------------------------------------------------------------------
// Constructor for PIO Reader
//----------------------------------------------------------------------------
vtkPIOReader::vtkPIOReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->FileName = nullptr;
  this->HyperTreeGrid = false;
  this->Tracers = false;
  this->Float64 = false;
  this->NumberOfVariables = 0;
  this->CurrentTimeStep = -1;
  this->LastTimeStep = -1;
  this->TimeSteps = 0;
  this->CellDataArraySelection = vtkDataArraySelection::New();

  // Setup selection callback to modify this object when array selection changes
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkPIOReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  // External PIO_DATA for actually reading files
  this->pioAdaptor = 0;

  this->MPIController = vtkMultiProcessController::GetGlobalController();
  if (this->MPIController)
  {
    this->Rank = this->MPIController->GetLocalProcessId();
    this->TotalRank = this->MPIController->GetNumberOfProcesses();
  }
  else
  {
    this->Rank = 0;
    this->TotalRank = 1;
  }
}

//----------------------------------------------------------------------------
// Destructor for PIO Reader
//----------------------------------------------------------------------------
vtkPIOReader::~vtkPIOReader()
{
  delete[] this->FileName;

  delete this->pioAdaptor;
  delete[] this->TimeSteps;

  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();

  // Do not delete the MPIContoroller which is a singleton
  this->MPIController = nullptr;
}

//----------------------------------------------------------------------------
// Verify that the file exists
//----------------------------------------------------------------------------
int vtkPIOReader::RequestInformation(vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** vtkNotUsed(inVector), vtkInformationVector* outVector)
{
  // Verify that file exists
  if (!this->FileName)
  {
    vtkErrorMacro("Reader called with no filename set");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
  }

  // Get ParaView information and output pointers
  vtkInformation* outInfo = outVector->GetInformationObject(0);
  if (this->pioAdaptor == 0)
  {

    // Create one PIOAdaptor which builds the MultiBlockDataSet
    this->pioAdaptor = new PIOAdaptor(this->Rank, this->TotalRank);

    // Initialize sizes and file reads
    // descriptor.pio file contains information
    // otherwise a basename-dmp000000 is given and defaults are used
    if (!this->pioAdaptor->initializeGlobal(this->FileName))
    {
      vtkErrorMacro("Error in pio description file");
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      delete this->pioAdaptor;
      this->pioAdaptor = 0;
      return 0;
    }

    this->HyperTreeGrid = pioAdaptor->GetHyperTreeGrid();
    this->Tracers = pioAdaptor->GetTracers();
    this->Float64 = pioAdaptor->GetFloat64();

    // Get the variable names and set in the selection
    int numberOfVariables = this->pioAdaptor->GetNumberOfVariables();
    for (int i = 0; i < numberOfVariables; i++)
    {
      this->CellDataArraySelection->AddArray(this->pioAdaptor->GetVariableName(i));
    }
    this->DisableAllCellArrays();

    // Set the variable names loaded by default
    for (int i = 0; i < this->pioAdaptor->GetNumberOfDefaultVariables(); i++)
    {
      this->SetCellArrayStatus(this->pioAdaptor->GetVariableDefault(i), 1);
    }

    // Collect temporal information
    this->NumberOfTimeSteps = this->pioAdaptor->GetNumberOfTimeSteps();
    this->TimeSteps = nullptr;

    if (this->NumberOfTimeSteps > 0)
    {
      this->TimeSteps = new double[this->NumberOfTimeSteps];

      for (int step = 0; step < this->NumberOfTimeSteps; step++)
      {
        this->TimeSteps[step] = (double)this->pioAdaptor->GetTimeStep(step);
      }

      // Tell the pipeline what steps are available
      outInfo->Set(
        vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeSteps, this->NumberOfTimeSteps);

      // Range is required to get GUI to show things
      double tRange[2];
      tRange[0] = this->TimeSteps[0];
      tRange[1] = this->TimeSteps[this->NumberOfTimeSteps - 1];
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
    }
    else
    {
      outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      outInfo->Set(
        vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeSteps, this->NumberOfTimeSteps);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
// Data is read into a vtkMultiBlockDataSet
//----------------------------------------------------------------------------
int vtkPIOReader::RequestData(vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** vtkNotUsed(inVector), vtkInformationVector* outVector)
{
  // If no PIOAdaptor there was an earlier failure
  if (this->pioAdaptor == 0)
  {
    vtkErrorMacro("Error in pio description file");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return 0;
  }

  vtkInformation* outInfo = outVector->GetInformationObject(0);
  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Collect the time step requested
  double requestedTimeStep(0);
  vtkInformationDoubleKey* timeKey =
    static_cast<vtkInformationDoubleKey*>(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  double dTime = 0;
  int timeStep = 0;

  // RequestData can be called from GUI pipeline or python script
  if (outInfo->Has(timeKey))
  {
    // Pipeline activated from GUI will have timeKey
    requestedTimeStep = outInfo->Get(timeKey);
    dTime = requestedTimeStep;

    // Index of the time step to request
    while (timeStep < this->NumberOfTimeSteps && this->TimeSteps[timeStep] < dTime)
    {
      timeStep++;
    }
    if (this->CurrentTimeStep != timeStep)
    {
      this->CurrentTimeStep = timeStep;
    }
  }
  else
  {
    // Pipeline actived from python script
    if (this->CurrentTimeStep < 0 || this->CurrentTimeStep >= this->NumberOfTimeSteps)
    {
      this->CurrentTimeStep = 0;
    }
    dTime = this->TimeSteps[this->CurrentTimeStep];
  }
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), dTime);

  // Load new geometry and data if time step has changed or selection changed
  this->LastTimeStep = this->CurrentTimeStep;

  // Initialize the PIOAdaptor for reading the requested dump file
  if (!this->pioAdaptor->initializeDump(this->CurrentTimeStep))
  {
    vtkErrorMacro("PIO dump file cannot be opened");
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return 0;
  }

  // Set parameters for the file read
  this->pioAdaptor->SetHyperTreeGrid(this->HyperTreeGrid);
  this->pioAdaptor->SetTracers(this->Tracers);
  this->pioAdaptor->SetFloat64(this->Float64);

  // Create the geometry requested in the pio descriptor file
  this->pioAdaptor->create_geometry(output);

  // Load the requested data in the correct ordering based on PIO daughters
  this->pioAdaptor->load_variable_data(output, this->CellDataArraySelection);

  return 1;
}

//----------------------------------------------------------------------------
void vtkPIOReader::SelectionModifiedCallback(
  vtkObject*, unsigned long vtkNotUsed(eventid), void* clientdata, void* vtkNotUsed(calldata))
{
  static_cast<vtkPIOReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkPIOReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkPIOReader::GetOutput(int idx)
{
  if (idx)
  {
    return nullptr;
  }
  else
  {
    return vtkMultiBlockDataSet::SafeDownCast(this->GetOutputDataObject(idx));
  }
}

//----------------------------------------------------------------------------
int vtkPIOReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkPIOReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkPIOReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
const char* vtkPIOReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkPIOReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkPIOReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
    this->CellDataArraySelection->EnableArray(name);
  else
    this->CellDataArraySelection->DisableArray(name);
}

void vtkPIOReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "FileName: " << (this->FileName != nullptr ? this->FileName : "") << endl;
  os << indent << "CellDataArraySelection: " << this->CellDataArraySelection << "\n";
  this->Superclass::PrintSelf(os, indent);
}
