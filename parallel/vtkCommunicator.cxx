/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkCommunicator.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkCommunicator.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetWriter.h"
#include "vtkStructuredPointsReader.h"
#include "vtkStructuredPointsWriter.h"
#include "vtkImageClip.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"

template <class T>
static int SendDataArray(T* data, int length, int handle, int tag, vtkCommunicator *self)
{

  self->Send(data, length, handle, tag);

  return 1;
}

template <class T>
static int ReceiveDataArray(T* data, int length, int handle, int tag, vtkDataArray *array)
{
  return 1;
}

vtkCommunicator::vtkCommunicator()
{
  this->MarshalString = 0;
  this->MarshalStringLength = 0;
  this->MarshalDataLength = 0;
 
}

vtkCommunicator::~vtkCommunicator()
{
  this->DeleteAndSetMarshalString(0, 0);
}

void vtkCommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Marshal string: ";
  if ( this->MarshalString )
    {
    os << this->MarshalString << endl;
    }
  else
    {
    os << "(None)" << endl;
    }
  os << indent << "Marshal string length: " << this->MarshalStringLength
     << endl;
  os << indent << "Marshal data length: " << this->MarshalDataLength
     << endl;
}

//----------------------------------------------------------------------------
// Internal method.  Assumes responsibility for deleting the string
void vtkCommunicator::DeleteAndSetMarshalString(char *str, int strLength)
{
  // delete any previous string
  if (this->MarshalString)
    {
    delete [] this->MarshalString;
    this->MarshalString = 0;
    this->MarshalStringLength = 0;
    this->MarshalDataLength = 0;
    }
  
  this->MarshalString = str;
  this->MarshalStringLength = strLength;
}

// Need to add better error checking
int vtkCommunicator::Send(vtkDataObject* data, int remoteHandle, 
			  int tag)
{

  if (data == NULL)
    {
    this->MarshalDataLength = 0;
    this->Send( &this->MarshalDataLength, 1,      
		remoteHandle, tag);
    return 1;
    }
  if (this->WriteObject(data))
    {
    this->Send( &this->MarshalDataLength, 1,      
		remoteHandle, tag);
    // then send the string.

    this->Send( this->MarshalString, this->MarshalDataLength, 
		remoteHandle, tag);
    
    return 1;
    }
  
  // could not marshal data
  return 0;
}

int vtkCommunicator::Send(vtkDataArray* data, int remoteHandle, int tag)
{

  if (data == NULL)
    {
    this->MarshalDataLength = 0;
    this->Send( &this->MarshalDataLength, 1,      
		remoteHandle, tag);
    return 1;
    }

  int type = data->GetDataType();
  this->Send( &type, 1, remoteHandle, tag);

  vtkIdType size = data->GetSize();
  this->Send( &size, 1, remoteHandle, tag);

  // now send the raw array
  switch (type)
    {

    case VTK_CHAR:
      return SendDataArray(static_cast<char*>(data->GetVoidPointer(type)), 
			  size, remoteHandle, tag, this);

    case VTK_UNSIGNED_CHAR:
      return SendDataArray(static_cast<unsigned char*>(data->GetVoidPointer(type)), 
			  size, remoteHandle, tag, this);

    case VTK_INT:
      return SendDataArray(static_cast<int*>(data->GetVoidPointer(type)), 
			  size, remoteHandle, tag, this);

    case VTK_UNSIGNED_LONG:
      return SendDataArray(static_cast<unsigned long*>(data->GetVoidPointer(type)), 
			  size, remoteHandle, tag, this);

    case VTK_FLOAT:
      return SendDataArray(static_cast<float*>(data->GetVoidPointer(type)), 
			  size, remoteHandle, tag, this);

    case VTK_DOUBLE:
      return SendDataArray(static_cast<double*>(data->GetVoidPointer(type)), 
			  size, remoteHandle, tag, this);

    case VTK_ID_TYPE:
      return SendDataArray(static_cast<vtkIdType*>(data->GetVoidPointer(type)), 
			  size, remoteHandle, tag, this);

    default:
      vtkErrorMacro(<<"Unsupported data type!");
      return 0; // could not marshal data

    }

}


int vtkCommunicator::Receive(vtkDataObject* data, int remoteHandle, 
			     int tag)
{
  int dataLength;

  // First receive the data length.
  if (!this->Receive( &dataLength, 1, remoteHandle, tag))
    {
    vtkErrorMacro("Could not receive data!");
    return 0;
    }
  
  if (dataLength < 0)
    {
    vtkErrorMacro("Bad data length");
    return 0;
    }
  
  if (dataLength == 0)
    { // This indicates a NULL object was sent. Do nothing.
    return 1;   
    }
  
  // if we cannot reuse the string, allocate a new one.
  if (dataLength > this->MarshalStringLength)
    {
    char *str = new char[dataLength + 10]; // maybe a little extra?
    this->DeleteAndSetMarshalString(str, dataLength + 10);
    }
  
  // Receive the string
  this->Receive(this->MarshalString, dataLength, 
		remoteHandle, tag);
  this->MarshalDataLength = dataLength;
  
  this->ReadObject(data);

  // we should really look at status to determine success
  return 1;
}

int vtkCommunicator::Receive(vtkDataArray* data, int remoteHandle, 
			     int tag)
{
  int size;
  int type;
  char *c = 0;
  unsigned char *uc = 0;
  int *i = 0;
  unsigned long *ul = 0;
  float *f = 0;
  double *d = 0;
  vtkIdType *idt = 0;
  

  // First receive the data type.
  if (!this->Receive( &type, 1, remoteHandle, tag))
    {
    vtkErrorMacro("Could not receive data!");
    return 0;
    }

  // Next receive the data length.
  if (!this->Receive( &size, 1, remoteHandle, tag))
    {
    vtkErrorMacro("Could not receive data!");
    return 0;
    }
  
  if (size < 0)
    {
    vtkErrorMacro("Bad data length");
    return 0;
    }
  
  if (size == 0)
    { // This indicates a NULL object was sent. Do nothing.
    return 1;   
    }
  
  // Receive the raw data array
  switch (type)
    {

    case VTK_CHAR:
      c = new char[size];
      this->Receive(c, size, remoteHandle, tag);
      static_cast<vtkCharArray*>(data)->SetArray(c, size, 1);
      break;

    case VTK_UNSIGNED_CHAR:
      uc = new unsigned char[size];
      this->Receive(uc, size, remoteHandle, tag);
      static_cast<vtkUnsignedCharArray*>(data)->SetArray(uc, size, 1);
      break;

    case VTK_INT:
      i = new int[size];
      this->Receive(i, size, remoteHandle, tag);
      static_cast<vtkIntArray*>(data)->SetArray(i, size, 1);
      break;

    case VTK_UNSIGNED_LONG:
      ul = new unsigned long[size];
      this->Receive(ul, size, remoteHandle, tag);
      static_cast<vtkUnsignedLongArray*>(data)->SetArray(ul, size, 1);
      break;

    case VTK_FLOAT:
      f = new float[size];
      this->Receive(f, size, remoteHandle, tag);
      static_cast<vtkFloatArray*>(data)->SetArray(f, size, 1);
      break;

    case VTK_DOUBLE:
      d = new double[size];
      this->Receive(d, size, remoteHandle, tag);
      static_cast<vtkDoubleArray*>(data)->SetArray(d, size, 1);
      break;

    case VTK_ID_TYPE:
      idt = new vtkIdType[size];
      this->Receive(idt, size, remoteHandle, tag);
      static_cast<vtkIdTypeArray*>(data)->SetArray(idt, size, 1);
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type!");
      return 0; // could not marshal data

    }

  return 1;

}

int vtkCommunicator::WriteObject(vtkDataObject *data)
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

int vtkCommunicator::ReadObject(vtkDataObject *data)
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


int vtkCommunicator::WriteImageData(vtkImageData *data)
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

int vtkCommunicator::ReadImageData(vtkImageData *object)
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

int vtkCommunicator::WriteDataSet(vtkDataSet *data)
{
  vtkDataSet *copy;
  unsigned long size;
  vtkDataSetWriter *writer = vtkDataSetWriter::New();

  copy = (vtkDataSet*)(data->MakeObject());
  copy->ShallowCopy(data);

  // There is a problem with binary files with no data.
  if (copy->GetNumberOfCells() > 0)
    {
    writer->SetFileTypeToBinary();
    }
  writer->WriteToOutputStringOn();
  writer->SetInput(copy);
  
  writer->Write();
  size = writer->GetOutputStringLength();
  this->DeleteAndSetMarshalString(writer->RegisterAndGetOutputString(), size);
  this->MarshalDataLength = size;
  writer->Delete();
  copy->Delete();

  return 1;
}

int vtkCommunicator::ReadDataSet(vtkDataSet *object)
{
  vtkDataSet *output;
  vtkDataSetReader *reader = vtkDataSetReader::New();

  if (this->MarshalString == NULL || this->MarshalStringLength <= 0)
    {
    return 0;
    }
  
  reader->ReadFromInputStringOn();
  reader->SetInputString(this->MarshalString, this->MarshalDataLength);
  output = reader->GetOutput();
  output->Update();

  object->ShallowCopy(output);
  //object->DataHasBeenGenerated();

  reader->Delete();

  return 1;
}
