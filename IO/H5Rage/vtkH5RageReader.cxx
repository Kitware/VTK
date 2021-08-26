/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkH5RageReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkH5RageReader.h"

#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkDirectory.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "H5RageAdaptor.h"

vtkStandardNewMacro(vtkH5RageReader);

//------------------------------------------------------------------------------
// Constructor for H5Rage Reader
//------------------------------------------------------------------------------
vtkH5RageReader::vtkH5RageReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->FileName = nullptr;
  this->CurrentTimeStep = -1;
  this->TimeSteps = nullptr;
  this->PointDataArraySelection = vtkDataArraySelection::New();

  // Setup selection callback to modify this object when array selection changes
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkH5RageReader::SelectionCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);

  // External adaptor for reading files
  this->H5rageAdaptor = nullptr;

  for (int dim = 0; dim < 3; dim++)
  {
    this->Dimension[dim] = 1;
    this->Origin[dim] = 0.0;
    this->Spacing[dim] = 1.0;
    this->WholeExtent[dim * 2] = 1;
    this->WholeExtent[dim * 2 + 1] = -1;
  }

  this->Controller = vtkMultiProcessController::GetGlobalController();
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
// Destructor for H5Rage Reader
//------------------------------------------------------------------------------
vtkH5RageReader::~vtkH5RageReader()
{
  delete[] this->FileName;
  delete[] this->TimeSteps;

  delete this->H5rageAdaptor;

  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->PointDataArraySelection->Delete();

  // Do not delete the Controller which is a singleton
  this->Controller = nullptr;
}

//------------------------------------------------------------------------------
// Verify that the file exists
//------------------------------------------------------------------------------
int vtkH5RageReader::RequestInformation(vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** vtkNotUsed(inVector), vtkInformationVector* outVector)
{
  if (!this->FileName)
  {
    vtkErrorMacro("Reader called with no filename set");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
  }

  // Get ParaView information and output pointers
  vtkInformation* outInfo = outVector->GetInformationObject(0);
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (this->H5rageAdaptor == nullptr)
  {
    // Create H5RageAdaptor which builds the ImageData
    this->H5rageAdaptor = new H5RageAdaptor(this->Controller);

    // Initialize files, variables, cycles, sizes from .h5rage metadata file
    if (!this->H5rageAdaptor->InitializeGlobal(this->FileName))
    {
      vtkErrorMacro("Error in h5rage description file");
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      delete this->H5rageAdaptor;
      this->H5rageAdaptor = nullptr;
      return 0;
    }

    // Get the sizes for ImageData
    for (int dim = 0; dim < 3; dim++)
    {
      this->Dimension[dim] = this->H5rageAdaptor->GetDimension(dim);
      this->Origin[dim] = this->H5rageAdaptor->GetOrigin(dim);
      this->Spacing[dim] = this->H5rageAdaptor->GetSpacing(dim);
    }
    for (int ext = 0; ext < 6; ext++)
    {
      this->WholeExtent[ext] = this->H5rageAdaptor->GetWholeExtent(ext);
      this->SubExtent[ext] = this->H5rageAdaptor->GetSubExtent(ext);
    }
    output->SetDimensions(this->Dimension);
    output->SetSpacing(this->Spacing);
    output->SetOrigin(this->Origin);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);

    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

    // Get the variable names and set in the selection
    int numberOfVariables = this->H5rageAdaptor->GetNumberOfVariables();
    for (int i = 0; i < numberOfVariables; i++)
    {
      this->PointDataArraySelection->AddArray(this->H5rageAdaptor->GetVariableName(i));
    }

    // Collect temporal information from H5RageAdaptor
    this->NumberOfTimeSteps = this->H5rageAdaptor->GetNumberOfTimeSteps();
    this->TimeSteps = nullptr;
    if (this->NumberOfTimeSteps > 0)
    {
      this->TimeSteps = new double[this->NumberOfTimeSteps];
    }
  }

  for (int step = 0; step < this->NumberOfTimeSteps; step++)
  {
    this->TimeSteps[step] = this->H5rageAdaptor->GetTimeStep(step);
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
  return 1;
}

//------------------------------------------------------------------------------
// Data is read into the ImageData
//------------------------------------------------------------------------------
int vtkH5RageReader::RequestData(vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** vtkNotUsed(inVector), vtkInformationVector* outVector)
{
  // If no H5RageAdaptor there was an earlier failure
  if (this->H5rageAdaptor == nullptr)
  {
    vtkErrorMacro("Error in h5rage description file");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return 0;
  }

  vtkInformation* outInfo = outVector->GetInformationObject(0);
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Set the subextent for this processor, includes ghost layer if parallel
  output->SetExtent(this->SubExtent);

  // Allocate Scalars for ImageData required to avoid error on Contour
  output->AllocateScalars(outInfo);

  // Collect the time step requested
  double requestedTimeStep(0);
  vtkInformationDoubleKey* timeKey =
    static_cast<vtkInformationDoubleKey*>(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  double dTime = this->TimeSteps[0];
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
    // Pipeline actived from python script
    if (this->CurrentTimeStep < 0 || this->CurrentTimeStep >= this->NumberOfTimeSteps)
    {
      this->CurrentTimeStep = 0;
    }
    dTime = this->TimeSteps[this->CurrentTimeStep];
  }
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), dTime);

  // Load all variables for requested time step
  H5rageAdaptor->LoadVariableData(output, this->CurrentTimeStep, this->PointDataArraySelection);

  return 1;
}

//------------------------------------------------------------------------------
void vtkH5RageReader::SelectionCallback(
  vtkObject*, unsigned long vtkNotUsed(eventid), void* clientdata, void* vtkNotUsed(calldata))
{
  static_cast<vtkH5RageReader*>(clientdata)->Modified();
}

//------------------------------------------------------------------------------
vtkImageData* vtkH5RageReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkImageData* vtkH5RageReader::GetOutput(int idx)
{
  if (idx)
  {
    return nullptr;
  }
  else
  {
    return vtkImageData::SafeDownCast(this->GetOutputDataObject(idx));
  }
}

//------------------------------------------------------------------------------
int vtkH5RageReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
void vtkH5RageReader::EnableAllPointArrays()
{
  this->PointDataArraySelection->EnableAllArrays();
}

//------------------------------------------------------------------------------
void vtkH5RageReader::DisableAllPointArrays()
{
  this->PointDataArraySelection->DisableAllArrays();
}

//------------------------------------------------------------------------------
const char* vtkH5RageReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//------------------------------------------------------------------------------
int vtkH5RageReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------------
void vtkH5RageReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->PointDataArraySelection->EnableArray(name);
  }
  else
  {
    this->PointDataArraySelection->DisableArray(name);
  }
}

void vtkH5RageReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "FileName: " << (this->FileName != nullptr ? this->FileName : "") << endl;
  os << indent << "XExtent: {" << this->WholeExtent[0] << ", " << this->WholeExtent[1] << "}"
     << endl;
  os << indent << "YExtent: {" << this->WholeExtent[2] << ", " << this->WholeExtent[3] << "}"
     << endl;
  os << indent << "ZExtent: {" << this->WholeExtent[4] << ", " << this->WholeExtent[5] << "}"
     << endl;
  os << indent << "Dimension: {" << this->Dimension[0] << ", " << this->Dimension[1] << ", "
     << this->Dimension[2] << "}" << endl;
  os << indent << "Origin: {" << this->Origin[0] << ", " << this->Origin[1] << ", "
     << this->Origin[2] << "}" << endl;
  os << indent << "Spacing: {" << this->Spacing[0] << ", " << this->Spacing[1] << ", "
     << this->Spacing[2] << "}" << endl;

  this->Superclass::PrintSelf(os, indent);
}
