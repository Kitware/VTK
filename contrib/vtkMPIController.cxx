/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkMPIController.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkMPIController.h"
#include "vtkCollection.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyDataWriter.h"


// The special tag used for RMI communication.
#define VTK_MPI_CONTROLLER_RMI_TAG 315167


// helper class.  A subclass of vtkObject so that I can keep them
// in a collection
class VTK_EXPORT vtkMPIControllerRMI : public vtkObject
{
public:
  static vtkMPIControllerRMI *New() {return new vtkMPIControllerRMI;}
  const char *GetClassName() {return "vtkMPIControllerRMI";};
  
  int Tag;
  void (*Method)(void *arg, int remoteProcessId);
  void *Argument;
};




//----------------------------------------------------------------------------
vtkMPIController::vtkMPIController()
{
  this->RMIs = vtkCollection::New();
  MPI_Comm_rank(MPI_COMM_WORLD, &this->LocalProcessId);
  this->MarshalString = NULL;
  this->MarshalStringLength = 0;
  this->MarshalDataLength = 0;
}

//----------------------------------------------------------------------------
// We need to have a "GetNetReferenceCount" to avoid memory leaks.
vtkMPIController::~vtkMPIController()
{
  this->RMIs->Delete();
  this->RMIs = NULL;
  // deletes string
  this->DeleteAndSetMarshalString(NULL, 0);
}


//----------------------------------------------------------------------------
void vtkMPIController::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
  vtkMPIControllerRMI *rmi;
  vtkIndent nextIndent = indent.GetNextIndent();
  
  os << indent << "LocalProcessId: " << this->LocalProcessId << endl;
  os << indent << "RMIs: \n";
  
  this->RMIs->InitTraversal();
  while ( (rmi = (vtkMPIControllerRMI*)(this->RMIs->GetNextItemAsObject())) )
    {
    os << nextIndent << rmi->Tag << endl;
    }
}

static vtkMPIController *GLOBAL_VTK_MPI_CONTROLLER = NULL;

//----------------------------------------------------------------------------
vtkMPIController *vtkMPIController::RegisterAndGetGlobalController(
                                                         vtkObject *obj)
{
  if (GLOBAL_VTK_MPI_CONTROLLER == NULL)
    {
    GLOBAL_VTK_MPI_CONTROLLER = vtkMPIController::New();
    GLOBAL_VTK_MPI_CONTROLLER->Register(obj);
    GLOBAL_VTK_MPI_CONTROLLER->Delete();
    }
  else
    {
    GLOBAL_VTK_MPI_CONTROLLER->Register(obj);
    }
  
  return GLOBAL_VTK_MPI_CONTROLLER;
}


//----------------------------------------------------------------------------
int vtkMPIController::Send(vtkObject *data, int remoteProcessId, int tag)
{
  if (tag == VTK_MPI_CONTROLLER_RMI_TAG)
    {
    vtkWarningMacro("The tag " << tag << " is reserved for RMIs.");
    }
  if (data == NULL)
    {
    return 0;
    }
  if (this->WriteObject(data))
    {
    // First send the length of the string,
    MPI_Send( &this->MarshalDataLength, 1, MPI_INT,
	     remoteProcessId, tag, MPI_COMM_WORLD);
    // then send the string.
    MPI_Send(this->MarshalString, this->MarshalDataLength, MPI_CHAR,
	     remoteProcessId, tag, MPI_COMM_WORLD);
    
    return 1;
    }
  
  // could not marshal data
  return 0;
}

//----------------------------------------------------------------------------
int vtkMPIController::Send(int *data, int length, int remoteProcessId, 
			    int tag)
{
  if (tag == VTK_MPI_CONTROLLER_RMI_TAG)
    {
    vtkWarningMacro("The tag " << tag << " is reserved for RMIs.");
    }
  MPI_Send(data, length, MPI_INT, remoteProcessId, tag, MPI_COMM_WORLD);
  return 1;
}
//----------------------------------------------------------------------------
int vtkMPIController::Send(unsigned long *data, int length, 
			   int remoteProcessId, int tag)
{
  if (tag == VTK_MPI_CONTROLLER_RMI_TAG)
    {
    vtkWarningMacro("The tag " << tag << " is reserved for RMIs.");
    }
  MPI_Send(data, length, MPI_UNSIGNED_LONG, remoteProcessId, tag, MPI_COMM_WORLD);
  return 1;  
}


//----------------------------------------------------------------------------
int vtkMPIController::Receive(vtkObject *data, 
			      int remoteProcessId, int tag)
{
  MPI_Status status;
  int dataLength;
  
  // First receive the data length.
  MPI_Recv( &dataLength, 1, MPI_INT, remoteProcessId, tag, 
	    MPI_COMM_WORLD, &status);
  
  if (dataLength <= 0)
    {
    vtkErrorMacro("Bad data length");
    return 0;
    }
  
  // if we cannot reuse the string, allocate a new one.
  if (dataLength > this->MarshalStringLength)
    {
    char *str = new char[dataLength + 10]; // maybe a little extra?
    this->DeleteAndSetMarshalString(str, dataLength + 10);
    }
  
  // Receive the string
  MPI_Recv(this->MarshalString, dataLength, MPI_CHAR, 
	   remoteProcessId, tag, MPI_COMM_WORLD, &status);
  this->MarshalDataLength = dataLength;
  
  this->ReadObject(data);

  // we should really look at status to determine success
  return 1;
}
//----------------------------------------------------------------------------
int vtkMPIController::Receive(int *data, int length, int remoteProcessId, 
			      int tag)
{
  MPI_Status status; 
  MPI_Recv(data, length, MPI_INT, remoteProcessId, tag, MPI_COMM_WORLD,
	   &status);
  // we should really look at status to determine success
  return 1;
}
//----------------------------------------------------------------------------
int vtkMPIController::Receive(unsigned long *data, int length, 
			      int remoteProcessId, int tag)
{
  MPI_Status status; 
  MPI_Recv(data, length, MPI_UNSIGNED_LONG, remoteProcessId, tag, MPI_COMM_WORLD,
	   &status);
  // we should really look at status to determine success
  return 1;
}



//----------------------------------------------------------------------------
void vtkMPIController::AddRMI(void (*f)(void *, int), void *arg, int tag)
{
  //vtkMPIControllerRMI *rmi = vtkMPIControllerRMI::New();
  vtkMPIControllerRMI *rmi = new vtkMPIControllerRMI;

  rmi->Tag = tag;
  rmi->Method = f;
  rmi->Argument = arg;
  this->RMIs->AddItem(rmi);
  rmi->Delete();
}

//----------------------------------------------------------------------------
void vtkMPIController::TriggerRMI(int remoteProcessId, int rmiTag)
{
  int message[2];
  
  // It is important for the remote process to know what process invoked it.
  // Multiple processes might try to invoke the method at the same time.
  // The remote method will know where to get additional args.
  message[0] = rmiTag;
  message[1] = this->LocalProcessId;
  
  MPI_Send(message, 2, MPI_INT, remoteProcessId, VTK_MPI_CONTROLLER_RMI_TAG,
	   MPI_COMM_WORLD);
}

//----------------------------------------------------------------------------
void vtkMPIController::ProcessRMIs()
{
  vtkMPIControllerRMI *rmi;
  MPI_Status status;
  int message[2];
  int rmiTag, remoteProcessId, found;
  
  while (1)
    {
    MPI_Recv(message, 2, MPI_INT, MPI_ANY_SOURCE, VTK_MPI_CONTROLLER_RMI_TAG,
	     MPI_COMM_WORLD, &status );
    rmiTag = message[0];
    remoteProcessId = message[1];
    
    // find the rmi
    found = 0;
    this->RMIs->InitTraversal();
    while ( !found &&
	   (rmi = (vtkMPIControllerRMI*)(this->RMIs->GetNextItemAsObject())) )
      {
      if (rmi->Tag == rmiTag)
	{
	found = 1;
	}
      }
    
    if ( ! found)
      {
      vtkErrorMacro("Process " << this->LocalProcessId << 
		    " Could not find RMI with tag " << rmiTag);
      }
    else
      {
      if ( rmi->Method )
	{
	(*rmi->Method)(rmi->Argument, remoteProcessId);
	}     
      }
    }
}

  
//----------------------------------------------------------------------------
int vtkMPIController::WriteObject(vtkObject *data)
{
  if (strcmp(data->GetClassName(), "vtkPolyData"))
    {
    return this->WritePolyData((vtkPolyData*)data);
    }  
  if (strcmp(data->GetClassName(), "vtkUnstructuredExtent"))
    {
    return this->WriteUnstructuredExtent((vtkUnstructuredExtent*)data);
    }
  
  vtkErrorMacro("Cannot marshal object of type "
		<< data->GetClassName());
  return 0;
}

//----------------------------------------------------------------------------
int vtkMPIController::ReadObject(vtkObject *data)
{
  if (strcmp(data->GetClassName(), "vtkPolyData"))
    {
    return this->ReadPolyData((vtkPolyData*)data);
    }
  if (strcmp(data->GetClassName(), "vtkUnstructuredExtent"))
    {
    return this->ReadUnstructuredExtent((vtkUnstructuredExtent*)data);
    }
  
  vtkErrorMacro("Cannot marshal object of type "
		<< data->GetClassName());
}



//----------------------------------------------------------------------------
int vtkMPIController::WritePolyData(vtkPolyData *data)
{
  unsigned long size;
  char *str;
  vtkPolyDataWriter *writer = vtkPolyDataWriter::New();
  
  writer->SetFileTypeToBinary();
  writer->WriteToOutputStringOn();
  writer->SetInput(data);
  
  // Guess the size from information.
  size = data->GetEstimatedUpdateMemorySize();
  // In case it was not set, use default.
  if (size <= 0)
    {
    // 5 kilo bytes
    size = 5;
    }
  // convert to bytes
  size = size * 1000;
  
  
  // use the previous string if it is long enough.
  if (this->MarshalStringLength < size)
    {
    // otherwise allocate a new string
    str = new char[size];
    this->DeleteAndSetMarshalString(str, size);
    writer->SetOutputString(str, size);
    str = NULL;
    }
  
  writer->Write();
  size = writer->GetOutputStringLength();
  while (size == this->MarshalStringLength)
    { // Write took all of string. Assume write was truncated.
    size = size * 2;
    str = new char[size];
    this->DeleteAndSetMarshalString(str, size);
    writer->SetOutputString(str, size);
    str = NULL;

    vtkWarningMacro("Expanding string length to " << size);
    writer->Write();
    size = writer->GetOutputStringLength();
    }
  
  // save the actual length of the data string.
  this->MarshalDataLength = size;
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkMPIController::ReadPolyData(vtkPolyData *object)
{
  vtkPolyDataReader *reader = vtkPolyDataReader::New();
  
  if (this->MarshalString == NULL || this->MarshalStringLength <= 0)
    {
    return 0;
    }
  
  reader->ReadFromInputStringOn();
  reader->SetInputString(this->MarshalString, this->MarshalDataLength);
  reader->Update();
  
  this->CopyPolyData(reader->GetOutput(), object);

  return 1;
}

//----------------------------------------------------------------------------
void vtkMPIController::CopyPolyData(vtkPolyData *src, vtkPolyData *dest)
{
  dest->CopyStructure(src);
  
  dest->GetPointData()->ShallowCopy(src->GetPointData());  
  dest->GetCellData()->ShallowCopy(src->GetCellData());  
}












//----------------------------------------------------------------------------
int vtkMPIController::WriteUnstructuredExtent(vtkUnstructuredExtent *ext)
{
  ostrstream *fptr = new ostrstream();
  
  *fptr << ext->GetClassName() << endl;
  *fptr << ext->GetExtentType() << endl;
  *fptr << ext->GetPiece() << " ";
  *fptr << ext->GetNumberOfPieces() << endl;
  
  // I am responsible for deleting this string.
  this->DeleteAndSetMarshalString(fptr->str(), fptr->pcount());
  delete fptr;
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkMPIController::ReadUnstructuredExtent(vtkUnstructuredExtent *object)
{
  istrstream *fptr;
  char name[100];
  int t1, t2;
  
  if (this->MarshalString == NULL || this->MarshalStringLength == 0)
    {
    return;
    }
  
  fptr = new istrstream(this->MarshalSrting, this->MarshalStringLength);
  
  *fptr >> name;
  if (strcmp(name, "vtkUnstructuredExtent") != 0)
    {
    vtkErrorMacro("Expecting vtkUnstructuredExtent: got " << name);
    delete fptr;
    return 0;
    }
  
  *fptr >> t1;
  object->SetExtentType(t1);
  *fptr >> t1 >> t2;
  object->SetExtent(t1, t2);
  
  delete fptr;
  
  return 1;
}






//----------------------------------------------------------------------------
// Internal method.  Assumes responsibility for deleting the string
void vtkMPIController::DeleteAndSetMarshalString(char *str, int strLength)
{
  // delete any previous srting
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







