/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAsynchronousBuffer.h
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
// .NAME vtkAsynchronousBuffer - For pipeline execution in multiple threads.
// .SECTION Description
// vtkAsynchronousBuffer will allow a non-blocking  update of a pipeline.
// When Blocking is off, the a call to Update returns imediately, and
// the Update continues in another thread. An abort mechanism has not
// been implemented.
// WARNING:  While the buffer is updating, the upstream pipeline cannot
// be modified or caused to update by the main thread.
// WARNING: This object is currently in developement, and its API may
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
  const char *GetClassName() {return "vtkAsynchronousBuffer";};
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
  
  // This ivar is public so thread func. can have access.
  // I do not think we need a lock on this variable.
  unsigned char Finished;
  unsigned char OutputConsumed;
  
protected:
  vtkAsynchronousBuffer();
  ~vtkAsynchronousBuffer();
  vtkAsynchronousBuffer(const vtkAsynchronousBuffer&) {};
  void operator=(const vtkAsynchronousBuffer&) {};

  int Blocking;
  vtkMultiThreader *Threader;
  int ThreadId;

  void BlockingUpdateInformation();
  void NonblockingUpdateInformation();
  void Execute();
};

#endif


