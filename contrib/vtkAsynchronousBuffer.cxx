/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAsynchronousBuffer.cxx
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
//#include <unistd.h>
#include "vtkAsynchronousBuffer.h"
#include "vtkDataInformation.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkAsynchronousBuffer* vtkAsynchronousBuffer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkAsynchronousBuffer");
  if(ret)
    {
    return (vtkAsynchronousBuffer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkAsynchronousBuffer;
}




//----------------------------------------------------------------------------
vtkAsynchronousBuffer::vtkAsynchronousBuffer()
{
  this->Blocking = 1;
  this->Threader = vtkMultiThreader::New();
  this->ThreadId = -1;
  this->Finished = 1;
  this->OutputConsumed = 1;
}

//----------------------------------------------------------------------------
vtkAsynchronousBuffer::~vtkAsynchronousBuffer()
{
  this->Threader->Delete();
  this->Threader = NULL;
}


//----------------------------------------------------------------------------
// the asynchrous update function
VTK_THREAD_RETURN_TYPE vtkAsynchronousBufferUpdate( void *arg )
{
  ThreadInfoStruct *info = (ThreadInfoStruct*)arg;
  vtkAsynchronousBuffer *self = (vtkAsynchronousBuffer*)(info->UserData);

  self->GetInput()->Update();
  self->Finished = 1;

  return VTK_THREAD_RETURN_VALUE;
}



//----------------------------------------------------------------------------
// Access to Finished should have a mutex lock around it.
void vtkAsynchronousBuffer::UpdateInformation()
{
  if (this->Blocking)
    {
    this->BlockingUpdateInformation();
    }
  else
    {
    this->NonblockingUpdateInformation();
    }
}

  
//----------------------------------------------------------------------------
// Access to Finished should have a mutex lock around it.
void vtkAsynchronousBuffer::NonblockingUpdateInformation()
{
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  unsigned long t1, t2;
  
  // just some error checking
  if (input == NULL)
    {
    vtkErrorMacro("No Input");
    return;
    }
  if (output == NULL)
    {
    vtkErrorMacro("No Output");
    return;
    }
  
  // Avoid accessing input if another thread is running.
  // I assume values from last UpdateInformation is sufficient.
  if ( ! this->Finished)
    {
    vtkDebugMacro("Still Updating");
    return;
    }
  
  // Test to see if we can promote data (copy form input to output)
  // If last promotion has not been consumed.
  // (The existance of a thread is used to detect promotable data.)
  if (this->OutputConsumed && this->ThreadId != -1)
    {
    // Promote the data (copy input to output)
    this->PromoteData();
    this->OutputConsumed = 0;
    // clean up thread.
    vtkDebugMacro("Promoting data to output");
    this->Threader->TerminateThread(this->ThreadId);
    this->ThreadId = -1;
    // This modified will cause update to be called ...
    // (not really necessary, because output has been modified by promotion).
    this->Modified();
    }
  
  // Test to see if we should start an asynchronous update.
  // Only start an update if we are not updating already, 
  // and there is no promotable data.
  if (this->ThreadId == -1)
    {
    // check the update time of the input directly to pre determine
    // whether input will generate new data.
    input->UpdateInformation();
    if (input->GetPipelineMTime() > input->GetUpdateTime())
      {
      // spawn a thread and start an update.
      // what if an abort occurs?!! ... 
      vtkDebugMacro("Spawn an update");
      this->Finished = 0;
      this->ThreadId = 
	this->Threader->SpawnThread(vtkAsynchronousBufferUpdate, (void *)this);
      }
    }
  
  // Do the typical update information stuff (as if we were a simple source).
  output->GetDataInformation()->SetLocality(1.0);
  t1 = this->GetMTime();
  t2 = output->GetMTime();
  if (t2 > t1)
    {
    t1 = t2;
    }
  output->SetPipelineMTime(t1);
  // Is it up to date? Really? Oh well.
  output->SetEstimatedWholeMemorySize(input->GetEstimatedWholeMemorySize());
  // Copy data specific information
  output->CopyInformation(input);
}
  
  
//----------------------------------------------------------------------------
// Access to Finished should have a mutex lock around it.
void vtkAsynchronousBuffer::BlockingUpdateInformation()
{
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  
  // just some error checking
  if (input == NULL)
    {
    vtkErrorMacro("No Input");
    return;
    }
  if (output == NULL)
    {
    vtkErrorMacro("No Output");
    return;
    }
  
  // make sure we are not already updating asynchronously.
  this->WaitForFinished();

  // Now we can look downstream for pipeline mtime.
  this->vtkSource::UpdateInformation();

  // Copy data specific information
  output->CopyInformation(input);
}


//----------------------------------------------------------------------------
void vtkAsynchronousBuffer::InternalUpdate(vtkDataObject *vtkNotUsed(output))
{
  if (this->Blocking)
    {
    this->BlockingUpdate();
    }
  else
    {
    this->NonblockingUpdate();
    }
} 

//----------------------------------------------------------------------------
void vtkAsynchronousBuffer::NonblockingUpdate()
{
  // Every thing has been done in UpdateInformation
  // Maybe we should leave the promotion to the update, but that might
  // cause a delay initiating the next asynchronous update.
  
  this->OutputConsumed = 1;
}

//----------------------------------------------------------------------------
// To make sure the data gets to output (for initialization)
// I am not positive this still works
void vtkAsynchronousBuffer::BlockingUpdate()
{
  if (this->GetInput() == NULL)
    {
    return;
    }

  // check to see if we are already updating (loop)
  if (this->Updating)
    {
    return;
    }

  // make sure we are not already updating asynchronously.
  this->WaitForFinished();

  // delete thread from last update (clean up)
  if (this->ThreadId != -1)
    {
    this->Threader->TerminateThread(this->ThreadId);
    this->ThreadId = -1;
    }

  // Do we need to update our input?
  if (this->GetInput()->GetUpdateTime() < this->GetInput()->GetPipelineMTime())
    {
    this->GetInput()->Update();
    }

  // This executes the copy if the input is more recent than the output
  this->PromoteData();
}



//----------------------------------------------------------------------------
// This may need a mutex lock.  I assume not because the spawned thread
// only set the variable to 1 at the end.  Any value of zero is ok,
// and any partial non zero value is ok too.
int vtkAsynchronousBuffer::TestForFinished()
{
  return (int)(this->Finished);
}


//----------------------------------------------------------------------------
void vtkAsynchronousBuffer::WaitForFinished()
{
  while ( ! this->Finished)
    {
    // vtkTimerLog::Sleep(10);
    }  
}



//----------------------------------------------------------------------------
void vtkAsynchronousBuffer::PromoteData()
{
  if ( this->StartMethod ) 
    {
    (*this->StartMethod)(this->StartMethodArg);
    }
  this->Progress = 0.0;
  this->GetOutput()->Initialize(); //clear output
  this->Execute();
  if ( !this->AbortExecute ) 
    {
    this->UpdateProgress(1.0);
    }
  if ( this->EndMethod ) 
    {
    (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->GetInput()->ShouldIReleaseData() )
    { 
    this->GetInput()->ReleaseData();
    }
}

//----------------------------------------------------------------------------
void vtkAsynchronousBuffer::Execute()
{
  this->GetOutput()->CopyStructure((vtkDataSet *)this->GetInput());

  this->GetOutput()->GetPointData()->PassData(
                 ((vtkDataSet *)this->GetInput())->GetPointData());
  this->GetOutput()->GetCellData()->PassData(
                 ((vtkDataSet *)this->GetInput())->GetCellData());
}


//----------------------------------------------------------------------------
void vtkAsynchronousBuffer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  if (this->Blocking)
    {
    os << indent << "BlockingOn\n";
    }
  else
    {
    os << indent << "BlockingOff\n";
    }
  os << "Finished: " << this->Finished << endl;
  
  os << indent << "ThreadId: " << this->ThreadId << "\n";
}

