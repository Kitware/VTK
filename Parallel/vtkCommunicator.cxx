/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCommunicator.h"

#include "vtkCharArray.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetWriter.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageClip.h"
#include "vtkIntArray.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkStructuredPointsWriter.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedLongArray.h"

vtkCxxRevisionMacro(vtkCommunicator, "1.26");

template <class T>
int SendDataArray(T* data, int length, int handle, int tag, vtkCommunicator *self)
{

  self->Send(data, length, handle, tag);

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

int vtkCommunicator::UseCopy = 0;
void vtkCommunicator::SetUseCopy(int useCopy)
{
  vtkCommunicator::UseCopy = useCopy;
}

void vtkCommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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

  int type = -1;
  if (data == NULL)
    {
      this->MarshalDataLength = 0;
      this->Send( &type, 1, remoteHandle, tag);
      return 1;
    }

  // send array type
  type = data->GetDataType();
  this->Send( &type, 1, remoteHandle, tag);

  // send array size
  vtkIdType size = data->GetSize();
  this->Send( &size, 1, remoteHandle, tag);

  // send number of components in array
  int numComponents = data->GetNumberOfComponents();
  this->Send( &numComponents, 1, remoteHandle, tag);

  
  const char* name = data->GetName();
  int len = 0;
  if (name)
    {
    len = static_cast<int>(strlen(name)) + 1;
    }

  // send length of name
  this->Send( &len, 1, remoteHandle, tag);

  if (len > 0)
    {
    // send name
    this->Send( const_cast<char*>(name), len, remoteHandle, tag);
    }

  // now send the raw array
  switch (type)
    {

    case VTK_CHAR:
      return SendDataArray(static_cast<char*>(data->GetVoidPointer(0)), 
                          size, remoteHandle, tag, this);

    case VTK_UNSIGNED_CHAR:
      return SendDataArray(static_cast<unsigned char*>(data->GetVoidPointer(0)), 
                          size, remoteHandle, tag, this);

    case VTK_INT:
      return SendDataArray(static_cast<int*>(data->GetVoidPointer(0)), 
                          size, remoteHandle, tag, this);

    case VTK_UNSIGNED_LONG:
      return SendDataArray(static_cast<unsigned long*>(data->GetVoidPointer(0)), 
                          size, remoteHandle, tag, this);

    case VTK_FLOAT:
      return SendDataArray(static_cast<float*>(data->GetVoidPointer(0)), 
                          size, remoteHandle, tag, this);

    case VTK_DOUBLE:
      return SendDataArray(static_cast<double*>(data->GetVoidPointer(0)), 
                          size, remoteHandle, tag, this);

    case VTK_ID_TYPE:
      return SendDataArray(static_cast<vtkIdType*>(data->GetVoidPointer(0)), 
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
  vtkIdType size;
  int type;
  int numComponents;
  int nameLength;

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

  if (type == -1) 
    { // This indicates a NULL object was sent. Do nothing.
    return 1;   
    }

  // Next receive the data length.
  if (!this->Receive( &size, 1, remoteHandle, tag))
    {
    vtkErrorMacro("Could not receive data!");
    return 0;
    }

  // Next receive the number of components.
  this->Receive( &numComponents, 1, remoteHandle, tag);

  // Next receive the length of the name.
  this->Receive( &nameLength, 1, remoteHandle, tag);

  if ( nameLength > 0 )
    {
    char *str = new char[nameLength]; 
    this->DeleteAndSetMarshalString(str, nameLength);
    
    // Receive the name
    this->Receive(this->MarshalString, nameLength, remoteHandle, tag);
    this->MarshalDataLength = nameLength;
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
      static_cast<vtkCharArray*>(data)->SetArray(c, size, 0);
      break;

    case VTK_UNSIGNED_CHAR:
      uc = new unsigned char[size];
      this->Receive(uc, size, remoteHandle, tag);
      static_cast<vtkUnsignedCharArray*>(data)->SetArray(uc, size, 0);
      break;

    case VTK_INT:
      i = new int[size];
      this->Receive(i, size, remoteHandle, tag);
      static_cast<vtkIntArray*>(data)->SetArray(i, size, 0);
      break;

    case VTK_UNSIGNED_LONG:
      ul = new unsigned long[size];
      this->Receive(ul, size, remoteHandle, tag);
      static_cast<vtkUnsignedLongArray*>(data)->SetArray(ul, size, 0);
      break;

    case VTK_FLOAT:
      f = new float[size];
      this->Receive(f, size, remoteHandle, tag);
      static_cast<vtkFloatArray*>(data)->SetArray(f, size, 0);
      break;

    case VTK_DOUBLE:

      d = new double[size];
      this->Receive(d, size, remoteHandle, tag);
      static_cast<vtkDoubleArray*>(data)->SetArray(d, size, 0);
      break;

    case VTK_ID_TYPE:
      idt = new vtkIdType[size];
      this->Receive(idt, size, remoteHandle, tag);
      static_cast<vtkIdTypeArray*>(data)->SetArray(idt, size, 0);
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type!");
      return 0; // could not marshal data

    }

  if (nameLength > 0)
    {
    data->SetName(this->MarshalString);
    }
  else
    {
    data->SetName(0);
    }
  data->SetNumberOfComponents(numComponents);

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
  tmp->SetUpdateExtent(data->GetUpdateExtent());
  
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
  
  vtkCharArray* mystring = vtkCharArray::New();
  // mystring should not delete the string when it's done,
  // that's our job.
  mystring->SetArray(this->MarshalString, this->MarshalDataLength, 1);
  reader->SetInputArray(mystring);
  mystring->Delete();

  reader->GetOutput()->Update();

  object->ShallowCopy(reader->GetOutput());
  object->SetUpdateExtent(reader->GetOutput()->GetUpdateExtent());
  
  reader->Delete();

  return 1;
}

int vtkCommunicator::WriteDataSet(vtkDataSet *data)
{
  vtkDataSet *copy;
  unsigned long size;
  vtkDataSetWriter *writer = vtkDataSetWriter::New();

  copy = data->NewInstance();
  copy->ShallowCopy(data);

  // There is a problem with binary files with no data.
  if (copy->GetNumberOfCells() + copy->GetNumberOfPoints() > 0)
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
  vtkDataSetReader *reader = vtkDataSetReader::New();

  if (this->MarshalString == NULL || this->MarshalStringLength <= 0)
    {
    return 0;
    }
  
  reader->ReadFromInputStringOn();

  vtkCharArray* mystring = vtkCharArray::New();
  // mystring should not delete the string when it's done,
  // that's our job.
  mystring->SetArray(this->MarshalString, this->MarshalDataLength, 1);
  reader->SetInputArray(mystring);
  mystring->Delete();

  reader->Update();
  object->ShallowCopy(reader->GetOutput());

  reader->Delete();

  return 1;
}
