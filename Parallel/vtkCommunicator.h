/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkCommunicator.h
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
// .NAME vtkCommunicator - Used to send/receive messages in a multiprocess/thread environment.
// .SECTION Description
// This is an abstact class which contains functionality for sending
// and receiving inter-process messages. It contains methods for marshaling
// an object into a string (currently used by the MPI communicator but
// not the shared memory communicator).

// .SECTION Caveats
// Communication between systems with different vtkIdTypes is not
// supported. All machines have to have the same vtkIdType.

// .SECTION see also
// vtkSharedMemoryCommunicator vtkMPICommunicator

#ifndef __vtkCommunicator_h
#define __vtkCommunicator_h

#include "vtkObject.h"
#include "vtkDataObject.h"
#include "vtkDataArray.h"

class vtkDataSet;
class vtkImageData;

class VTK_EXPORT vtkCommunicator : public vtkObject
{

public:

  vtkTypeMacro(vtkCommunicator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method sends a data object to a destination.  
  // Tag eliminates ambiguity
  // and is used to match sends to receives.
  virtual int Send(vtkDataObject* data, int remoteHandle, int tag);

  // Description:
  // This method sends a data array to a destination.  
  // Tag eliminates ambiguity
  // and is used to match sends to receives.
  virtual int Send(vtkDataArray* data, int remoteHandle, int tag);
  
  // Description:
  // Subclass have to supply these methods to send various arrays of data.
  virtual int Send(int* data, int length, int remoteHandle, int tag) = 0;
  virtual int Send(unsigned long* data, int length, int remoteHandle, 
		   int tag) = 0;
  virtual int Send(unsigned char* data, int length, int remoteHandle, 
		   int tag) = 0;
  virtual int Send(char* data, int length, int remoteHandle, 
		   int tag) = 0;
  virtual int Send(float* data, int length, int remoteHandle, 
		   int tag) = 0;
  virtual int Send(double* data, int length, int remoteHandle, 
		   int tag) = 0;
  virtual int Send(vtkIdType* data, int length, int remoteHandle, 
		   int tag) = 0;


  // Description:
  // This method receives a data object from a corresponding send. It blocks
  // until the receive is finished. 
  virtual int Receive(vtkDataObject* data, int remoteHandle, int tag);

  // Description:
  // This method receives a data array from a corresponding send. It blocks
  // until the receive is finished. 
  virtual int Receive(vtkDataArray* data, int remoteHandle, int tag);

  // Description:
  // Subclass have to supply these methods to receive various arrays of data.
  virtual int Receive(int* data, int length, int remoteHandle, 
		      int tag) = 0;
  virtual int Receive(unsigned long* data, int length, int remoteHandle,
		      int tag) = 0;
  virtual int Receive(unsigned char* data, int length, int remoteHandle, 
		      int tag) = 0;
  virtual int Receive(char* data, int length, int remoteHandle, 
		      int tag) = 0;
  virtual int Receive(float* data, int length, int remoteHandle, 
		      int tag) = 0;
  virtual int Receive(double* data, int length, int remoteHandle, 
		      int tag) = 0;
  virtual int Receive(vtkIdType* data, int length, int remoteHandle, 
		      int tag) = 0;

protected:

  void DeleteAndSetMarshalString(char *str, int strLength);

  // Write and read from marshal string
  // return 1 success, 0 fail
  int WriteObject(vtkDataObject *object);
  int ReadObject(vtkDataObject *object);
  
  int WriteDataSet(vtkDataSet *object);
  int ReadDataSet(vtkDataSet *object);

  int WriteImageData(vtkImageData *object);
  int ReadImageData(vtkImageData *object);

  int WriteDataArray(vtkDataArray *object);
  int ReadDataArray(vtkDataArray *object);

  vtkCommunicator();
  ~vtkCommunicator();
  vtkCommunicator(const vtkCommunicator&);
  void operator=(const vtkCommunicator&);

  char *MarshalString;
  int MarshalStringLength;
  // The data may not take up all of the string.
  int MarshalDataLength;

};

#endif // __vtkCommunicator_h


