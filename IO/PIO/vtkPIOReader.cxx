// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPIOReader.h"

#include "PIOAdaptor.h"

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
#include "vtkStringArray.h"
#include "vtkUnstructuredGrid.h"

#include <iostream>
#include <set>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPIOReader);
vtkCxxSetObjectMacro(vtkPIOReader, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
// Constructor for PIO Reader
//------------------------------------------------------------------------------
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
  this->TimeSteps = nullptr;
  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->TimeDataStringArray = vtkStringArray::New();

  // Setup selection callback to modify this object when array selection changes
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkPIOReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  this->ActiveTimeDataArrayName = nullptr;
  this->SetActiveTimeDataArrayName("CycleIndex");

  // External PIO_DATA for actually reading files
  this->pioAdaptor = nullptr;

  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  if (this->Controller)
  {
    this->Rank = this->Controller->GetLocalProcessId();
    this->TotalRank = this->Controller->GetNumberOfProcesses();
  }
  else
  {
    this->Rank = 0;
    this->TotalRank = 1;
  }
}

//------------------------------------------------------------------------------
// Destructor for PIO Reader
//------------------------------------------------------------------------------
vtkPIOReader::~vtkPIOReader()
{
  delete[] this->FileName;

  delete this->pioAdaptor;
  delete[] this->TimeSteps;

  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();
  this->TimeDataStringArray->Delete();
  this->SetActiveTimeDataArrayName(nullptr);

  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
// Verify that the file exists
//------------------------------------------------------------------------------
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
  if (this->pioAdaptor == nullptr)
  {
    // Create one PIOAdaptor which builds the MultiBlockDataSet
    this->pioAdaptor = new PIOAdaptor(this->Controller);

    // Initialize sizes and file reads
    // descriptor.pio file contains information
    // otherwise a basename-dmp000000 is given and defaults are used
    if (!this->pioAdaptor->initializeGlobal(this->FileName))
    {
      vtkErrorMacro("Error in loading pio files");
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      delete this->pioAdaptor;
      this->pioAdaptor = nullptr;
      return 0;
    }

    this->HyperTreeGrid = pioAdaptor->GetHyperTreeGrid();
    this->Tracers = pioAdaptor->GetTracers();
    this->Float64 = pioAdaptor->GetFloat64();

    // Get the variable names and set in the selection
    std::set<std::string> variablesToEnableByDefault;
    for (int cc = 0, max = this->pioAdaptor->GetNumberOfDefaultVariables(); cc < max; ++cc)
    {
      variablesToEnableByDefault.insert(this->pioAdaptor->GetVariableDefault(cc));
    }
    for (int i = 0, max = this->pioAdaptor->GetNumberOfVariables(); i < max; i++)
    {
      const auto varName = this->pioAdaptor->GetVariableName(i);
      // vtkDataArraySelection::AddArray doesn't override the setting only adds it
      // (without affecting MTime) if not already present.
      this->CellDataArraySelection->AddArray(
        varName, variablesToEnableByDefault.find(varName) != variablesToEnableByDefault.end());
    }

    // Collect temporal information from PIOAdaptor's last PIO file
    this->TimeDataStringArray->Initialize();
    this->NumberOfTimeSteps = this->pioAdaptor->GetNumberOfTimeSteps();
    this->TimeDataStringArray->InsertNextValue("SimulationTime");
    this->TimeDataStringArray->InsertNextValue("CycleIndex");
    this->TimeDataStringArray->InsertNextValue("PIOFileIndex");

    this->TimeSteps = nullptr;
    if (this->NumberOfTimeSteps > 0)
    {
      this->TimeSteps = new double[this->NumberOfTimeSteps];
    }
  }

  // Set the current TIME_STEP() data based on requested TimeArrayName
  if (this->ActiveTimeDataArrayName != this->CurrentTimeDataArrayName)
  {
    this->CurrentTimeDataArrayName = this->ActiveTimeDataArrayName;
    if (strcmp(this->ActiveTimeDataArrayName, "SimulationTime") == 0)
    {
      for (int step = 0; step < this->NumberOfTimeSteps; step++)
      {
        this->TimeSteps[step] = this->pioAdaptor->GetSimulationTime(step);
      }
    }
    else if (strcmp(this->ActiveTimeDataArrayName, "CycleIndex") == 0)
    {
      for (int step = 0; step < this->NumberOfTimeSteps; step++)
      {
        this->TimeSteps[step] = this->pioAdaptor->GetCycleIndex(step);
      }
    }
    else if (strcmp(this->ActiveTimeDataArrayName, "PIOFileIndex") == 0)
    {
      for (int step = 0; step < this->NumberOfTimeSteps; step++)
      {
        this->TimeSteps[step] = this->pioAdaptor->GetPIOFileIndex(step);
      }
    }
    else
    {
      for (int step = 0; step < this->NumberOfTimeSteps; step++)
      {
        this->TimeSteps[step] = (double)step;
      }
    }

    if (this->NumberOfTimeSteps > 0)
    {
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

  // Indicate reading in parallel is supported.
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//------------------------------------------------------------------------------
// Data is read into a vtkMultiBlockDataSet
//------------------------------------------------------------------------------
int vtkPIOReader::RequestData(vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** vtkNotUsed(inVector), vtkInformationVector* outVector)
{
  // If no PIOAdaptor there was an earlier failure
  if (this->pioAdaptor == nullptr)
  {
    vtkErrorMacro("Error in loading pio files");
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
    while (timeStep < (this->NumberOfTimeSteps - 1) && this->TimeSteps[timeStep] < dTime)
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
    // Pipeline activated from python script
    if (this->CurrentTimeStep < 0 || this->CurrentTimeStep >= this->NumberOfTimeSteps)
    {
      this->CurrentTimeStep = 0;
    }
    dTime = this->TimeSteps[this->CurrentTimeStep];
  }
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), dTime);

  // Load new geometry and data if time step has changed or selection changed
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

//------------------------------------------------------------------------------
void vtkPIOReader::SelectionModifiedCallback(
  vtkObject*, unsigned long vtkNotUsed(eventid), void* clientdata, void* vtkNotUsed(calldata))
{
  static_cast<vtkPIOReader*>(clientdata)->Modified();
}

//------------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkPIOReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
int vtkPIOReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
void vtkPIOReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

//------------------------------------------------------------------------------
void vtkPIOReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

//------------------------------------------------------------------------------
const char* vtkPIOReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//------------------------------------------------------------------------------
int vtkPIOReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------------
void vtkPIOReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
    this->CellDataArraySelection->EnableArray(name);
  else
    this->CellDataArraySelection->DisableArray(name);
}

//------------------------------------------------------------------------------
int vtkPIOReader::GetNumberOfTimeDataArrays() const
{
  return static_cast<int>(this->TimeDataStringArray->GetNumberOfValues());
}

//------------------------------------------------------------------------------
const char* vtkPIOReader::GetTimeDataArray(int idx) const
{
  if (idx < 0 || idx > static_cast<int>(this->TimeDataStringArray->GetNumberOfValues()))
  {
    vtkErrorMacro("Invalid index for 'GetTimeDataArray': " << idx);
    return nullptr;
  }
  return this->TimeDataStringArray->GetValue(idx).c_str();
}

void vtkPIOReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "FileName: " << (this->FileName != nullptr ? this->FileName : "") << endl;
  os << indent << "CellDataArraySelection: " << this->CellDataArraySelection << "\n";
  os << indent << "NumberOfTimeSteps:" << this->NumberOfTimeSteps << "\n";
  os << indent << "TimeDataStringArray: " << this->TimeDataStringArray << "\n";
  os << indent << "ActiveTimeDataArrayName:"
     << (this->ActiveTimeDataArrayName ? this->ActiveTimeDataArrayName : "(null)") << "\n";

  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
