/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAsynchronousBuffer.h
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
// .NAME vtkAsynchronousBuffer - For pipeline execution in multiple threads.
// .SECTION Description
// vtkAsynchronousBuffer will allow a non-blocking  update of a pipeline.
// When Blocking is off, the a call to Update returns immediately, and
// the Update continues in another thread. An abort mechanism has not
// been implemented.
// WARNING:  While the buffer is updating, the upstream pipeline cannot
// be modified or caused to update by the main thread.
// WARNING: This object is currently in development, and its API may
// change in the future (or the class may go away completely).

// .SECTION See Also
// vtkMutexLock vtkMultiThreader


#ifndef __vtkAsynchronousBuffer_h
#define __vtkAsynchronousBuffer_h

#include "vtkDataSetToDataSetFilter.h"
#include "vtkMultiThreader.h"
class vtkPortController;



class VTK_EXPORT vtkAsynchronousBuffer : public vtkDataSetToDataSetFilter
{
public:
  vtkTypeMacro(vtkAsynchronousBuffer,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkAsynchronousBuffer *New();

  // Description:
  // A flag to change the behavior of the standard "Update" call.  When blocking
  // is off, then this method starts an update on the input, and returns 
  // immediately.  If an additional call is made before an update is finished, 
  // it returns with no effect.  If it is called after a previous update has 
  // completed, it swaps the buffers and checks to see if another update needs 
  // to occur.
  vtkSetMacro(Blocking, int);
  vtkGetMacro(Blocking, int);
  vtkBooleanMacro(Blocking, int);

  // Description:
  // The behavior of this method depends on the "Blocking" flag.
  void InternalUpdate(vtkDataObject *output);

  // Description:
  // Methods required by the vtkPort superclass.
  void BlockingUpdate();
  void NonblockingUpdate();
  void PromoteData();
  int TestForFinished();
  void WaitForFinished();

  // Description:
  // We need a special UpdateInformation method because the PipelineMTime
  // is messed up by the asynchronous nature of the update.  The UpdateTime
  // of a down stream filter can not be compared to the modifiedTime
  // of an up stream object.
  void UpdateInformation(); 
  
  // Description:
  // If this value is 1 then, the buffer is in the middle of an
  // asynchronous update.
  vtkGetMacro(Finished, unsigned char); 
  
  // This ivar is public so thread function. can have access.
  // I do not think we need a lock on this variable.
  unsigned char Finished;
  unsigned char OutputConsumed;
  
protected:
  vtkAsynchronousBuffer();
  ~vtkAsynchronousBuffer();
  vtkAsynchronousBuffer(const vtkAsynchronousBuffer&);
  void operator=(const vtkAsynchronousBuffer&);

  int Blocking;
  vtkMultiThreader *Threader;
  int ThreadId;

  void BlockingUpdateInformation();
  void NonblockingUpdateInformation();
  void Execute();
};

#endif


