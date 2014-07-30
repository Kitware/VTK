/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkADIOSWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <cstring>
#include <limits>
#include <stdexcept>
#include <iostream>
#include <sstream>

#include "ADIOSDefs.h"
#include "ADIOSWriter.h"

#include "vtkADIOSWriter.h"
#include <vtkObjectFactory.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkDemandDrivenPipeline.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkMPIController.h>
#include <vtkMPI.h>

#include <vtkDataObject.h>
#include <vtkAbstractArray.h>
#include <vtkLookupTable.h>
#include <vtkDataArray.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkFieldData.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkDataSet.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkUnstructuredGrid.h>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkADIOSWriter);

//----------------------------------------------------------------------------
vtkADIOSWriter::vtkADIOSWriter()
: FileName(NULL), TransportMethod(vtkADIOSWriter::POSIX),
  TransportMethodArguments(NULL), Transform(vtkADIOSWriter::None),
  WriteMode(vtkADIOSWriter::Always), CurrentStep(-1), Controller(NULL),
  Writer(NULL),
  NumberOfPieces(-1), RequestPiece(-1), NumberOfGhostLevels(-1),
  WriteAllTimeSteps(true), TimeSteps(), CurrentTimeStepIndex(-1)
{
  std::memset(this->RequestExtent, 0, 6*sizeof(int));
  std::memset(this->WholeExtent, 0, 6*sizeof(int));
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkADIOSWriter::~vtkADIOSWriter()
{
  if(this->Writer)
    {
    delete this->Writer;
    }
  this->SetFileName(NULL);
  this->SetTransportMethodArguments(NULL);
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " << this->FileName << std::endl;
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::SetController(vtkMultiProcessController *controller)
{
  vtkMPIController *mpiController = vtkMPIController::SafeDownCast(controller);
  if(controller && !mpiController)
    {
    vtkErrorMacro("ADIOS Writer can only be used with an MPI controller");
    return;
    }

  vtkSetObjectBodyMacro(Controller, vtkMultiProcessController, controller);

  if(mpiController)
    {
    vtkMPICommunicator *comm = static_cast<vtkMPICommunicator *>(
      this->Controller->GetCommunicator());
    ADIOSWriter::SetCommunicator(*comm->GetMPIComm()->GetHandle());

    this->NumberOfPieces = this->Controller->GetNumberOfProcesses();
    this->RequestPiece = this->Controller->GetLocalProcessId();
    }
  else
    {
    this->NumberOfPieces = -1;
    this->RequestPiece = -1;
    }
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::OpenFile(void)
{
  this->Writer->Open(this->FileName, this->CurrentStep > 0);
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::CloseFile(void)
{
  this->Writer->Close();
}

//----------------------------------------------------------------------------
template<typename T>
bool vtkADIOSWriter::DefineAndWrite(vtkDataObject *input)
{
  const T *data = T::SafeDownCast(input);
  if(!data)
    {
    return false;
    }

  const int localProc = this->Controller->GetLocalProcessId();

  try
    {
    ++this->CurrentStep;

    // Make sure we're within time bounds
    if(this->CurrentTimeStepIndex >= static_cast<int>(this->TimeSteps.size()))
      {
      vtkErrorMacro(<< "All timesteps have been exhausted");
      return false;
      }

    // Things to do on the first step, before writing any data
    if(this->CurrentStep == 0)
      {
      // Before any data can be writen, it's structure must be declared
      this->Define("", data);

      if(this->WriteMode == vtkADIOSWriter::OnChange)
        {
        // Set up the index for independently array stepping
        this->BlockStepIndex.clear();
        this->BlockStepIndex.resize(this->BlockStepIndexIdMap.size());

        std::vector<size_t> indexDims;
        indexDims.push_back(this->BlockStepIndexIdMap.size());
        this->Writer->DefineArray<int>("::BlockStepIndex", indexDims);

        // Gather all the block step index id maps to Rank 0
        std::string BlockStepIndexIdMapAttr = this->GatherBlockStepIdMap();

        if(localProc == 0)
          {
          this->Writer->DefineAttribute<std::string>("::BlockStepIndexIdMap",
            BlockStepIndexIdMapAttr);
          }
        }

      if(localProc == 0)
        {
        // Global time step is only used by Rank 0
        this->Writer->DefineScalar<double>("/TimeStamp");

        // Define all appropriate attributes
        this->Writer->DefineAttribute<int>("::NumberOfPieces",
          this->NumberOfPieces);
        }
      }

    this->OpenFile();
    if(localProc == 0 && this->CurrentTimeStepIndex >= 0)
      {
      this->Writer->WriteScalar<double>("/TimeStamp",
        this->TimeSteps[this->CurrentTimeStepIndex]);
      }

    std::memset(&*this->BlockStepIndex.begin(), 0xFF,
      sizeof(vtkTypeInt64)*this->BlockStepIndex.size());
    this->Write("", data);

    if(this->WriteMode == vtkADIOSWriter::OnChange)
      {
      this->Writer->WriteArray("::BlockStepIndex", &this->BlockStepIndex[0]);
      }
    this->CloseFile();
    }
  catch(const std::runtime_error &err)
    {
    vtkErrorMacro(<< err.what());
    return false;
    }
  return true;
}

//----------------------------------------------------------------------------
std::string vtkADIOSWriter::GatherBlockStepIdMap(void)
{
  const int numProcs = this->Controller->GetNumberOfProcesses();
  const int localProc = this->Controller->GetLocalProcessId();

  // Encode into string containing:
  // Block0_Id Var0_Id Var0_Name
  // Block0_Id Var1_Id Var1_Name
  // ...
  // BlockN_Id VarM_Id VarM_Name
  std::stringstream ss;
  for(NameIdMap::const_iterator i = this->BlockStepIndexIdMap.begin();
    i != this->BlockStepIndexIdMap.end(); ++i)
    {
    ss << localProc << ' ' << i->second << ' ' << i->first << '\n';
    }
  std::string sendBuf = ss.str();
  vtkIdType sendBufLen = sendBuf.length();

  // Gather the variable length buffer sizes
  vtkIdType *recvLengths = localProc == 0 ? new vtkIdType[numProcs] : NULL;
  this->Controller->Gather(&sendBufLen, recvLengths, 1, 0);

  // Compute the recieving buffer sizes and offsets
  vtkIdType fullLength = 0;
  vtkIdType *recvOffsets = NULL;
  char *recvBuffer = NULL;
  if(localProc == 0)
    {
    recvOffsets = new vtkIdType[numProcs];
    for(int p = 0; p < numProcs; ++p)
      {
      recvOffsets[p] = fullLength;
      fullLength += recvLengths[p];
      }
    recvBuffer = new char[fullLength];
    }

  // Gather the index id maps from all processes
  this->Controller->GatherV(sendBuf.c_str(), recvBuffer, sendBufLen,
    recvLengths, recvOffsets, 0);

  std::string recv;
  if(localProc == 0)
    {
    // Strip the trailing \n to make null terminated and parse as an std::string
    recvBuffer[fullLength-1] = '\0';
    recv = recvBuffer;

    // Cleanup
    delete[] recvBuffer;
    delete[] recvOffsets;
    delete[] recvLengths;
    }
  return recv;
}

//----------------------------------------------------------------------------
bool vtkADIOSWriter::WriteInternal(void)
{
  vtkDataObject *input = this->GetInputDataObject(0, 0);
  if(!input)
    {
    return false;
    }

  switch(input->GetDataObjectType())
    {
    case VTK_IMAGE_DATA:
      return this->DefineAndWrite<vtkImageData>(input);
    case VTK_POLY_DATA:
      return this->DefineAndWrite<vtkPolyData>(input);
    case VTK_UNSTRUCTURED_GRID:
      return this->DefineAndWrite<vtkUnstructuredGrid>(input);
    default:
      vtkErrorMacro("Input vtkDataObject type not supported by ADIOS writer");
      return false;
    }
}

//----------------------------------------------------------------------------
int vtkADIOSWriter::FillInputPortInformation(int port, vtkInformation *info)
{
  // Only 1 port
  if(port != 0)
    {
    return 0;
    }

  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkADIOSWriter::ProcessRequest(vtkInformation* request,
  vtkInformationVector** input, vtkInformationVector* output)
{
  // Make sure the ADIOS subsystem is initialized before processing any
  // sort of request.
  if(!this->Writer)
    {
    this->Writer = new ADIOSWriter(
      static_cast<ADIOS::TransportMethod>(this->TransportMethod),
      this->TransportMethodArguments ? this->TransportMethodArguments : "");
    }

  // Now process the request

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, input, output) ? 1 : 0;
    }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->RequestUpdateExtent(request, input, output) ? 1 : 0;
    }

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, input, output) ? 1 : 0;
    }

  return this->Superclass::ProcessRequest(request, input, output);
}

//----------------------------------------------------------------------------
bool vtkADIOSWriter::RequestInformation(vtkInformation *vtkNotUsed(req),
  vtkInformationVector **input, vtkInformationVector *vtkNotUsed(output))
{
  vtkInformation *inInfo = input[0]->GetInformationObject(0);

  this->TimeSteps.clear();
  this->CurrentTimeStepIndex = -1;
  if(inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
    int len = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    double *steps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->TimeSteps.reserve(len);
    this->TimeSteps.insert(this->TimeSteps.begin(), steps, steps+len);
    this->CurrentTimeStepIndex = 0;
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkADIOSWriter::RequestUpdateExtent(vtkInformation *vtkNotUsed(req),
  vtkInformationVector **input, vtkInformationVector *vtkNotUsed(output))
{
  vtkInformation* inInfo = input[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    this->NumberOfPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    this->RequestPiece);
  if(this->CurrentTimeStepIndex >= 0 &&
    this->CurrentTimeStepIndex < static_cast<int>(this->TimeSteps.size()))
    {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
      this->TimeSteps[this->CurrentTimeStepIndex]);
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkADIOSWriter::RequestData(vtkInformation *req,
  vtkInformationVector **vtkNotUsed(input),
  vtkInformationVector *vtkNotUsed(output))
{
  int numSteps = static_cast<int>(this->TimeSteps.size());

  // Continue looping if we're not at the end
  if(this->CurrentTimeStepIndex >= 0 && // index of -1 means no steps
    this->WriteAllTimeSteps && this->CurrentTimeStepIndex < numSteps)
    {
    req->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }

  if(!this->WriteInternal())
    {
    return false;
    }

  if(this->CurrentTimeStepIndex >= 0)
    {
    ++this->CurrentTimeStepIndex;

    // End looping if we're at the end
    if(this->WriteAllTimeSteps && this->CurrentTimeStepIndex >= numSteps)
      {
      req->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      }
    }

  return true;
}
