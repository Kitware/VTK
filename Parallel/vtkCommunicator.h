/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCommunicator - Used to send/receive messages in a multiprocess environment.
// .SECTION Description
// This is an abstact class which contains functionality for sending
// and receiving inter-process messages. It contains methods for marshaling
// an object into a string (currently used by the MPI communicator but
// not the shared memory communicator).

// .SECTION Caveats
// Communication between systems with different vtkIdTypes is not
// supported. All machines have to have the same vtkIdType.

// .SECTION see also
// vtkMPICommunicator

#ifndef __vtkCommunicator_h
#define __vtkCommunicator_h

#include "vtkObject.h"

class vtkDataSet;
class vtkImageData;
class vtkDataObject;
class vtkDataArray;
class vtkBoundingBox;
class vtkCharArray;

class VTK_PARALLEL_EXPORT vtkCommunicator : public vtkObject
{

public:

  vtkTypeRevisionMacro(vtkCommunicator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method sends a data object to a destination.  
  // Tag eliminates ambiguity
  // and is used to match sends to receives.
  int Send(vtkDataObject* data, int remoteHandle, int tag);

  // Description:
  // This method sends a data array to a destination.  
  // Tag eliminates ambiguity
  // and is used to match sends to receives.
  int Send(vtkDataArray* data, int remoteHandle, int tag);

  // Description:
  // Subclasses have to supply this method to send various arrays of data.
  // The \c type arg is one of the VTK type constants recognized by the
  // vtkTemplateMacro (VTK_FLOAT, VTK_INT, etc.).  \c length is measured
  // in number of values (as opposed to number of bytes).
  virtual int SendVoidArray(const void *data, vtkIdType length, int type,
                            int remoteHandle, int tag) = 0;
  
  // Description:
  // Convenience methods for sending data arrays.
  int Send(const int* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_INT, remoteHandle, tag);
  }
  int Send(const unsigned long* data, vtkIdType length,
           int remoteHandle, int tag) {
    return this->SendVoidArray(data, length,VTK_UNSIGNED_LONG,remoteHandle,tag);
  }
  int Send(const unsigned char* data, vtkIdType length,
           int remoteHandle, int tag) {
    return this->SendVoidArray(data, length,VTK_UNSIGNED_CHAR,remoteHandle,tag);
  }
  int Send(const char* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_CHAR, remoteHandle, tag);
  }
  int Send(const float* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_FLOAT, remoteHandle, tag);
  }
  int Send(const double* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_DOUBLE, remoteHandle, tag);
  }
#ifdef VTK_USE_64BIT_IDS
  int Send(const vtkIdType* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_ID_TYPE, remoteHandle, tag);
  }
#endif


  // Description:
  // This method receives a data object from a corresponding send. It blocks
  // until the receive is finished. 
  int Receive(vtkDataObject* data, int remoteHandle, int tag);

  // Description:
  // This method receives a data array from a corresponding send. It blocks
  // until the receive is finished. 
  int Receive(vtkDataArray* data, int remoteHandle, int tag);

  // Description:
  // Subclasses have to supply this method to receive various arrays of data.
  // The \c type arg is one of the VTK type constants recognized by the
  // vtkTemplateMacro (VTK_FLOAT, VTK_INT, etc.).  \c length is measured
  // in number of values (as opposed to number of bytes).
  virtual int ReceiveVoidArray(void *data, vtkIdType length, int type,
                               int remoteHandle, int tag) = 0;

  // Description:
  // Convenience methods for receiving data arrays.
  int Receive(int* data, vtkIdType length, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, length, VTK_INT, remoteHandle, tag);
  }
  int Receive(unsigned long* data, vtkIdType length, int remoteHandle, int tag){
    return this->ReceiveVoidArray(data, length, VTK_UNSIGNED_LONG, remoteHandle,
                                  tag);
  }
  int Receive(unsigned char* data, vtkIdType length, int remoteHandle, int tag){
    return this->ReceiveVoidArray(data, length, VTK_UNSIGNED_CHAR, remoteHandle,
                                  tag);
  }
  int Receive(char* data, vtkIdType length, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, length, VTK_CHAR, remoteHandle, tag);
  }
  int Receive(float* data, vtkIdType length, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, length, VTK_FLOAT, remoteHandle, tag);
  }
  int Receive(double* data, vtkIdType length, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, length, VTK_DOUBLE, remoteHandle, tag);
  }
#ifdef VTK_USE_64BIT_IDS
  int Receive(vtkIdType* data, vtkIdType length, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, length, VTK_ID_TYPE, remoteHandle, tag);
  }
#endif

  static void SetUseCopy(int useCopy);

  // Description:
  // Determine the global bounds for a set of processes.  BBox is 
  // initially set (outside of the call to the local bounds of the process 
  // and will be modified to be the global bounds - this default implementation
  // views the processors as a heap tree with the root being processId = 0
  // If either rightHasBounds or leftHasBounds is not 0 then the 
  // corresponding int will be set to 1 if the right/left processor has
  // bounds else it will be set to 0
  // The last three arguements are the tags to be used when performing
  // the operation
  virtual int ComputeGlobalBounds(int processorId, int numProcesses,
                                  vtkBoundingBox *bounds,
                                  int *rightHasBounds = 0,
                                  int *leftHasBounds = 0,
                                  int hasBoundsTag = 288402, 
                                  int localBoundsTag = 288403,
                                  int globalBoundsTag = 288404);

  // Description: 
  // Some helper functions when dealing with heap tree - based
  // algorthims - we don't need a function for getting the right
  // processor since it is 1 + theLeftProcessor
  static int GetParentProcessor(int pid);
  static int GetLeftChildProcessor(int pid);

  // Description:
  // Convert a data object into a string that can be transmitted and vice versa.
  // Returns 1 for success and 0 for failure.
  static int MarshalDataObject(vtkDataObject *object, vtkCharArray *buffer);
  static int UnMarshalDataObject(vtkCharArray *buffer, vtkDataObject *object);

protected:
  
  int WriteDataArray(vtkDataArray *object);
  int ReadDataArray(vtkDataArray *object);

  vtkCommunicator();
  ~vtkCommunicator();

  static int UseCopy;

private:
  vtkCommunicator(const vtkCommunicator&);  // Not implemented.
  void operator=(const vtkCommunicator&);  // Not implemented.
};

#endif // __vtkCommunicator_h


