/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkMultiProcessController.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// This will be the default.
#include "vtkMultiProcessController.h"
#include "vtkThreadedController.h"

#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#endif

#include "vtkCollection.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetWriter.h"
#include "vtkStructuredPointsReader.h"
#include "vtkStructuredPointsWriter.h"
#include "vtkImageClip.h"
#include "vtkTimerLog.h"
#include "vtkObjectFactory.h"


// The special tag used for RMI communication.
#define VTK_MP_CONTROLLER_RMI_TAG 315167
#define VTK_MP_CONTROLLER_RMI_ARG_TAG 315168


// Helper class to contain the RMI information.  
// A subclass of vtkObject so that I can keep them in a collection.
class VTK_EXPORT vtkMultiProcessControllerRMI : public vtkObject
{
public:
  static vtkMultiProcessControllerRMI *New() 
    {return new vtkMultiProcessControllerRMI;}
  const char *GetClassName() 
    {return "vtkMultiProcessControllerRMI";};
  
  int Tag;
  vtkRMIFunctionType Function;
  void *LocalArgument;
};

//----------------------------------------------------------------------------
// An RMI function that will break the "ProcessRMIs" loop.
void vtkMultiProcessControllerBreakRMI(void *localArg, 
                                       void *remoteArg, int remoteArgLength, 
                                       int vtkNotUsed(remoteId))
{
  vtkMultiProcessController *controller;
  remoteArg = remoteArg;
  remoteArgLength = remoteArgLength;
  controller = (vtkMultiProcessController*)(localArg);
  controller->SetBreakFlag(1);
}

//----------------------------------------------------------------------------
vtkMultiProcessController::vtkMultiProcessController()
{
  int i;

  // If some one is using multiple processes, 
  // limit the number of threads to 1
  vtkMultiThreader::SetGlobalDefaultNumberOfThreads(1);
  
  this->LocalProcessId = 0;
  this->NumberOfProcesses = 1;
  this->MaximumNumberOfProcesses = VTK_MP_CONTROLLER_MAX_PROCESSES;
  
  this->RMIs = vtkCollection::New();
  this->MarshalString = NULL;
  this->MarshalStringLength = 0;
  this->MarshalDataLength = 0;
  
  this->SingleMethod = NULL;
  this->SingleData = NULL;   
  
  for ( i = 0; i < VTK_MAX_THREADS; i++ )
    {
    this->MultipleMethod[i] = NULL;
    this->MultipleData[i] = NULL;
    }

  this->WriteTime = 0.0;
  this->ReadTime = 0.0;

  this->SendTime = 0.0;
  this->SendWaitTime = 0.0;

  this->ReceiveTime = 0.0;
  this->ReceiveWaitTime = 0.0;

  this->BreakFlag = 0;
  this->ForceDeepCopy = 1;
  
  // Define an rmi internally to exit from the processing loop.
  this->AddRMI(vtkMultiProcessControllerBreakRMI, this,
	       VTK_BREAK_RMI_TAG);
}

//----------------------------------------------------------------------------
// This is an old comment that I do not know is true:
// (We need to have a "GetNetReferenceCount" to avoid memory leaks.)
vtkMultiProcessController::~vtkMultiProcessController()
{
  this->RMIs->Delete();
  this->RMIs = NULL;
  // deletes string
  this->DeleteAndSetMarshalString(NULL, 0);
}


  
//----------------------------------------------------------------------------
vtkMultiProcessController *vtkMultiProcessController::New()
{
  const char *temp;
  
  // first check the environment variable
  temp = getenv("VTK_CONTROLLER");
  
#ifdef VTK_USE_MPI
  if ( temp == NULL || !strcmp("MPI",temp))
    {
    return vtkMPIController::New();
    }
#endif
  if ( temp == NULL || !strcmp("Threaded",temp))
    {
    return vtkThreadedController::New();
    }

  vtkGenericWarningMacro("environment variable VTK_CONTROLLER set to unknown value "
		       << temp << ". Try MPI or Threaded");
  return NULL;
}



//----------------------------------------------------------------------------
void vtkMultiProcessController::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
  vtkMultiProcessControllerRMI *rmi;
  vtkIndent nextIndent = indent.GetNextIndent();
  
  os << indent << "MaximumNumberOfProcesses: " 
     << this->MaximumNumberOfProcesses << endl;
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
  os << indent << "LocalProcessId: " << this->LocalProcessId << endl;
  os << indent << "MarshalStringLength: " << this->MarshalStringLength << endl;
  os << indent << "MarshalDataLength: " << this->MarshalDataLength << endl;
  os << indent << "ReceiveWaitTime: " << this->ReceiveWaitTime << endl;
  os << indent << "ReceiveTime: " << this->ReceiveTime << endl;
  os << indent << "SendWaitTime: " << this->SendWaitTime << endl;
  os << indent << "SendTime: " << this->SendTime << endl;
  os << indent << "ReadTime: " << this->ReadTime << endl;
  os << indent << "WriteTime: " << this->WriteTime << endl;
  os << indent << "RMIs: \n";
  
  this->RMIs->InitTraversal();
  while ( (rmi = (vtkMultiProcessControllerRMI*)(this->RMIs->GetNextItemAsObject())) )
    {
    os << nextIndent << rmi->Tag << endl;
    }
  
  os << indent << "SendWaitTime: " << this->SendWaitTime << endl;
  os << indent << "SendTime: " << this->SendTime << endl;
  os << indent << "ReceiveWaitTime: " << this->ReceiveWaitTime << endl;
  os << indent << "ReceiveTime: " << this->ReceiveTime << endl;
  os << indent << "ReadTime: " << this->ReadTime << endl;
  os << indent << "WriteTime: " << this->WriteTime << endl;
  os << indent << "BreakFlag: " << this->BreakFlag << endl;
  os << indent << "ForceDeepCopy: " << this->ForceDeepCopy << endl;
}

static vtkMultiProcessController *GLOBAL_VTK_MP_CONTROLLER = NULL;



//----------------------------------------------------------------------------
void vtkMultiProcessController::SetNumberOfProcesses(int num)
{
  if (num == this->NumberOfProcesses)
    {
    return;
    }
  
  if (num < 1 || num > this->MaximumNumberOfProcesses)
    {
    vtkErrorMacro( << num 
	  << "is an invalid number of processes try a number from 1 to " 
	  << (this->NumberOfProcesses - 1));
    return;
    }
  
  this->NumberOfProcesses = num;
  this->Modified();
}


//----------------------------------------------------------------------------
// Set the user defined method that will be run on NumberOfThreads threads
// when SingleMethodExecute is called.
void vtkMultiProcessController::SetSingleMethod( vtkProcessFunctionType f, 
						 void *data )
{
  this->SingleMethod = f;
  this->SingleData   = data;
}

//----------------------------------------------------------------------------
// Set one of the user defined methods that will be run on NumberOfProcesses
// processes when MultipleMethodExecute is called. This method should be
// called with index = 0, 1, ..,  NumberOfProcesses-1 to set up all the
// required user defined methods
void vtkMultiProcessController::SetMultipleMethod( int index,
				 vtkProcessFunctionType f, void *data )
{ 
  // You can only set the method for 0 through NumberOfProcesses-1
  if ( index >= this->NumberOfProcesses ) {
    vtkErrorMacro( << "Can't set method " << index << 
    " with a processes count of " << this->NumberOfProcesses );
    }
  else
    {
    this->MultipleMethod[index] = f;
    this->MultipleData[index]   = data;
    }
}




//----------------------------------------------------------------------------
int vtkMultiProcessController::Send(vtkDataObject *data, int remoteProcessId, 
				    int tag)
{
  vtkTimerLog *log;

  if (tag == VTK_MP_CONTROLLER_RMI_TAG)
    {
    vtkWarningMacro("The tag " << tag << " is reserved for RMIs.");
    }
  if (data == NULL)
    {
    this->MarshalDataLength = 0;
    this->Send( &this->MarshalDataLength, 1,      
		remoteProcessId, tag);
    return 1;
    }
  if (this->WriteObject(data))
    {
    log = vtkTimerLog::New();
    // First send the length of the string,
    log->StartTimer();
    this->Send( &this->MarshalDataLength, 1,      
		remoteProcessId, tag);
    log->StopTimer();
    this->SendWaitTime = log->GetElapsedTime();
    // then send the string.
    log->StartTimer();
    this->Send( this->MarshalString, this->MarshalDataLength, 
		remoteProcessId, tag);
    log->StopTimer();
    this->SendTime = log->GetElapsedTime();
    
    log->Delete();
    return 1;
    }
  
  // could not marshal data
  return 0;
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::Receive(vtkDataObject *data, 
				       int remoteProcessId, int tag)
{
  int dataLength;
  vtkTimerLog *log = vtkTimerLog::New();

  // First receive the data length.
  log->StartTimer();
  this->Receive( &dataLength, 1, remoteProcessId, tag);
  
  log->StopTimer();
  this->ReceiveWaitTime = log->GetElapsedTime();

  if (dataLength < 0)
    {
    vtkErrorMacro("Bad data length");
    log->Delete();
    return 0;
    }
  
  if (dataLength == 0)
    { // This indicates a NULL object was sent. Do nothing.
    log->Delete();
    return 1;   
    }
  
  // if we cannot reuse the string, allocate a new one.
  if (dataLength > this->MarshalStringLength)
    {
    char *str = new char[dataLength + 10]; // maybe a little extra?
    this->DeleteAndSetMarshalString(str, dataLength + 10);
    }
  
  // Receive the string
  log->StartTimer();
  this->Receive(this->MarshalString, dataLength, 
		remoteProcessId, tag);
  this->MarshalDataLength = dataLength;
  log->StopTimer();
  this->ReceiveTime = log->GetElapsedTime();
  
  this->ReadObject(data);

  // we should really look at status to determine success
  log->Delete();
  return 1;
}


//----------------------------------------------------------------------------
void vtkMultiProcessController::AddRMI(vtkRMIFunctionType f, 
                                       void *localArg, int tag)
{
  //vtkMultiProcessControllerRMI *rmi = vtkMultiProcessControllerRMI::New();
  vtkMultiProcessControllerRMI *rmi = new vtkMultiProcessControllerRMI;

  rmi->Tag = tag;
  rmi->Function = f;
  rmi->LocalArgument = localArg;
  this->RMIs->AddItem(rmi);
  rmi->Delete();
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::TriggerRMI(int remoteProcessId, 
                                           void *arg, int argLength,
                                           int rmiTag)
{
  int triggerMessage[3];

  // Deal with sending RMI to ourself here for now.
  if (remoteProcessId == this->GetLocalProcessId())
    {
    this->ProcessRMI(remoteProcessId, arg, argLength, rmiTag);
    return;
    }
  
  triggerMessage[0] = rmiTag;
  triggerMessage[1] = argLength;
  // It is important for the remote process to know what process invoked it.
  // Multiple processes might try to invoke the method at the same time.
  // The remote method will know where to get additional args.
  triggerMessage[2] = this->GetLocalProcessId();
  
  this->Send(triggerMessage, 3, remoteProcessId, VTK_MP_CONTROLLER_RMI_TAG);
  if (argLength > 0)
    {
    this->Send((char*)arg, argLength, remoteProcessId,  
               VTK_MP_CONTROLLER_RMI_ARG_TAG);
    } 
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::ProcessRMIs()
{
  int triggerMessage[3];
  unsigned char *arg = NULL;
  
  while (1)
    {
    this->Receive(triggerMessage, 3, 
		  VTK_MP_CONTROLLER_ANY_SOURCE, VTK_MP_CONTROLLER_RMI_TAG);
    if (triggerMessage[1] > 0)
      {
      arg = new unsigned char[triggerMessage[1]];
      this->Receive((char*)(arg), triggerMessage[1], 
		    triggerMessage[2], VTK_MP_CONTROLLER_RMI_ARG_TAG);
      }
    this->ProcessRMI(triggerMessage[2], arg, triggerMessage[1], 
                     triggerMessage[0]);
    if (arg)
      {
      delete [] arg;
      arg = NULL;
      }
    
    // check for break
    if (this->BreakFlag)
      {
      this->BreakFlag = 0;
      return;
      }
    }
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::ProcessRMI(int remoteProcessId, 
                                           void *arg, int argLength,
                                           int rmiTag)
{
  vtkMultiProcessControllerRMI *rmi;
  int found = 0;

  // find the rmi
  this->RMIs->InitTraversal();
  while ( !found &&
   (rmi = (vtkMultiProcessControllerRMI*)(this->RMIs->GetNextItemAsObject())) )
    {
    if (rmi->Tag == rmiTag)
      {
      found = 1;
      }
    }
  
  if ( ! found)
    {
    vtkErrorMacro("Process " << this->GetLocalProcessId() << 
		  " Could not find RMI with tag " << rmiTag);
    }
  else
    {
    if ( rmi->Function )
      {
      (*rmi->Function)(rmi->LocalArgument, arg, argLength, remoteProcessId);
      }     
    }
}

//----------------------------------------------------------------------------
// Internal method.  Assumes responsibility for deleting the string
void vtkMultiProcessController::DeleteAndSetMarshalString(char *str, 
							  int strLength)
{
  // delete any previous string
  if (this->MarshalString)
    {
    delete [] this->MarshalString;
    this->MarshalString = NULL;
    this->MarshalStringLength = 0;
    this->MarshalDataLength = 0;
    }
  
  this->MarshalString = str;
  this->MarshalStringLength = strLength;
}


  
//----------------------------------------------------------------------------
int vtkMultiProcessController::WriteObject(vtkDataObject *data)
{
  if (strcmp(data->GetClassName(), "vtkPolyData") == 0          ||
      strcmp(data->GetClassName(), "vtkUnstructuredGrid") == 0  ||
      strcmp(data->GetClassName(), "vtkStructuredGrid") == 0    ||
      strcmp(data->GetClassName(), "vtkRectilinearGrid") == 0   ||
      strcmp(data->GetClassName(), "vtkStructuredPoints") == 0)
    {
    return this->WriteDataSet((vtkDataSet*)data);
    }  
  if (strcmp(data->GetClassName(), "vtkImageData") == 0)
    {
    return this->WriteImageData((vtkImageData*)data);
    }
  
  vtkErrorMacro("Cannot marshal object of type "
		<< data->GetClassName());
  return 0;
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::ReadObject(vtkDataObject *data)
{
  if (strcmp(data->GetClassName(), "vtkPolyData") == 0          ||
      strcmp(data->GetClassName(), "vtkUnstructuredGrid") == 0  ||
      strcmp(data->GetClassName(), "vtkStructuredGrid") == 0    ||
      strcmp(data->GetClassName(), "vtkRectilinearGrid") == 0   ||
      strcmp(data->GetClassName(), "vtkStructuredPoints") == 0)
    {
    return this->ReadDataSet((vtkDataSet*)data);
    }  
  if (strcmp(data->GetClassName(), "vtkImageData") == 0)
    {
    return this->ReadImageData((vtkImageData*)data);
    }
  
  vtkErrorMacro("Cannot marshal object of type "
		<< data->GetClassName());

  return 1;
}



//----------------------------------------------------------------------------
int vtkMultiProcessController::WriteImageData(vtkImageData *data)
{
  vtkImageClip *clip;
  vtkStructuredPointsWriter *writer;
  int size;
  
  // keep Update from propagating
  vtkImageData *tmp = vtkImageData::New();
  tmp->ShallowCopy(data);
  
  clip = vtkImageClip::New();
  clip->SetInput(tmp);
  clip->SetOutputWholeExtent(data->GetExtent());
  writer = vtkStructuredPointsWriter::New();
  writer->SetFileTypeToBinary();
  writer->WriteToOutputStringOn();
  writer->SetInput(clip->GetOutput());
  writer->Write();
  size = writer->GetOutputStringLength();
  
  this->DeleteAndSetMarshalString(writer->RegisterAndGetOutputString(), size);
  this->MarshalDataLength = size;
  clip->Delete();
  writer->Delete();
  tmp->Delete();
  
  return 1;
}
//----------------------------------------------------------------------------
int vtkMultiProcessController::ReadImageData(vtkImageData *object)
{
  vtkStructuredPointsReader *reader = vtkStructuredPointsReader::New();

  if (this->MarshalString == NULL || this->MarshalStringLength <= 0)
    {
    return 0;
    }
  
  reader->ReadFromInputStringOn();
  reader->SetInputString(this->MarshalString, this->MarshalDataLength);
  reader->GetOutput()->Update();

  object->ShallowCopy(reader->GetOutput());
  
  reader->Delete();

  return 1;
}


//----------------------------------------------------------------------------
int vtkMultiProcessController::WriteDataSet(vtkDataSet *data)
{
  unsigned long size;
  vtkDataSetWriter *writer = vtkDataSetWriter::New();
  vtkTimerLog *log = vtkTimerLog::New();

  log->StartTimer();

  // There is a problem with binary files with no data.
  if (data->GetNumberOfCells() > 0)
    {
    writer->SetFileTypeToBinary();
    }
  writer->WriteToOutputStringOn();
  writer->SetInput(data);
  
  writer->Write();
  size = writer->GetOutputStringLength();
  this->DeleteAndSetMarshalString(writer->RegisterAndGetOutputString(), size);
  this->MarshalDataLength = size;
  writer->Delete();

  log->StopTimer();
  this->WriteTime = log->GetElapsedTime();
  log->Delete();
  
  return 1;
}
//----------------------------------------------------------------------------
int vtkMultiProcessController::ReadDataSet(vtkDataSet *object)
{
  vtkDataSet *output;
  vtkDataSetReader *reader = vtkDataSetReader::New();
  vtkTimerLog *log;

  if (this->MarshalString == NULL || this->MarshalStringLength <= 0)
    {
    return 0;
    }
  
  log = vtkTimerLog::New();
  log->StartTimer();
  reader->ReadFromInputStringOn();
  reader->SetInputString(this->MarshalString, this->MarshalDataLength);
  output = reader->GetOutput();
  output->Update();

  object->ShallowCopy(output);
  //object->DataHasBeenGenerated();

  reader->Delete();

  log->StopTimer();
  this->ReadTime = log->GetElapsedTime();
  log->Delete();

  return 1;
}
