/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkUpStreamPort.cxx
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
#include "vtkUpStreamPort.h"
#include "vtkDownStreamPort.h"
#include "vtkMultiProcessController.h"
#include "vtkPolyData.h"


//----------------------------------------------------------------------------
vtkUpStreamPort::vtkUpStreamPort()
{
  this->Tag = -1;
  
  // Controller keeps a reference to this object as well.
  this->Controller = 
    vtkMultiProcessController::RegisterAndGetGlobalController(this);
}

//----------------------------------------------------------------------------
// We need to have a "GetNetReferenceCount" to avoid memory leaks.
vtkUpStreamPort::~vtkUpStreamPort()
{
  vtkMultiProcessController *tmp;
  
  // as a precaution set ivar to NULL before deleting.
  tmp = this->Controller;
  this->Controller = NULL;
  tmp->UnRegister(this);
}

//----------------------------------------------------------------------------
void vtkUpStreamPort::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProcessObject::PrintSelf(os,indent);
  os << indent << "Tag: " << this->Tag << endl;
  os << indent << "Controller: (" << this->Controller << ")\n";
}


//----------------------------------------------------------------------------
// Remote method call to start an update.
// Actually call by UpdateInformation.
void vtkUpStreamPortUpdateCallBack(void *arg, int remoteProcessId)  
{
  vtkUpStreamPort *self = (vtkUpStreamPort*)arg;
  
  // Just call a method
  self->Trigger(remoteProcessId);
}

  
//----------------------------------------------------------------------------
void vtkUpStreamPort::Trigger(int remoteProcessId)
{
  vtkDataObject *input = this->GetInput();
  unsigned long pmt, downDataTime;
  int transferNeeded;
  
  // First get the update extent requested.
  this->Controller->Receive((vtkObject*)(input->GetGenericUpdateExtent()),
			    remoteProcessId, 
			    VTK_PORT_UPDATE_EXTENT_TAG);
  
  // Second, receive the previous "TransferTime" from the down stream port.
  this->Controller->Receive( &downDataTime, 1, remoteProcessId, 
			     VTK_PORT_DOWN_DATA_TIME_TAG);
  
  // Handle no input gracefully.
  if ( input == NULL )
    {
    vtkErrorMacro("Input is not set.");
    transferNeeded = 0;
    this->Controller->Send( &transferNeeded, 1, remoteProcessId,
			    VTK_PORT_TRANSFER_NEEDED_TAG);
    return;
    }

  // Get PipelineMTime.
  input->UpdateInformation();
  pmt = input->GetPipelineMTime();
    
  // See if the down stream port needs new data.
  transferNeeded = (pmt > downDataTime);
  
  // Send message back whether to expect data.
  this->Controller->Send( &transferNeeded, 1, remoteProcessId,
			  VTK_PORT_TRANSFER_NEEDED_TAG);
  
  if ( ! transferNeeded)
    {
    return;
    }
  
  // Now, it is a little unusual to have an update during an
  // UpdateInformation call.  However, it is the only way to
  // get task parallelism initiated in parallel without defining
  // a non-blocking update in vtkSource.  One note: A second 
  // call to UpdateInformation could block while this Update
  // is finishing.  This should not be a problem now that there
  // is only one UpdateInformation per Update 
  // (thanks to InternalUpdate). Ah! But if we have more than
  // one port in a serially linked pipeline, each one adds another 
  // call to UpdateInformation.  I am assume this will not cause 
  // a problem.  STREAMING (BABY)!
  
  // Do we need to update?
  if (pmt > this->UpdateTime)
    {
    input->InternalUpdate();
    this->UpdateTime.Modified();
    }
  
  // The data transfer is received in the down stream ports Update method.
  // First transfer the new data.
  this->Controller->Send( input, remoteProcessId,
			  VTK_PORT_DATA_TRANSFER_TAG);
  // Last, send its time for the down stream port to store.
  downDataTime = this->UpdateTime.GetMTime();
  this->Controller->Send( &downDataTime, 1, remoteProcessId,
			  VTK_PORT_NEW_DATA_TIME_TAG);
}





//----------------------------------------------------------------------------
void vtkUpStreamPort::SetInput(vtkPolyData *input)
{
  this->vtkProcessObject::SetInput(0, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkUpStreamPort::GetInput()
{
  if (this->Inputs == NULL)
    {
    return NULL;
    }
  return this->Inputs[0];
}


//----------------------------------------------------------------------------
// We need to create an RMI when the tag is set.
void vtkUpStreamPort::SetTag(int tag)
{
  if (this->Tag == tag)
    {
    return;
    }
  
  this->Modified();
  
  // remove old RMI.
  if (this->Tag != -1)
    {
    this->Controller->RemoveRMI(vtkUpStreamPortUpdateCallBack, (void *)this, this->Tag);
    }
  
  this->Tag = tag;
  this->Controller->AddRMI(vtkUpStreamPortUpdateCallBack, (void *)this, tag);
}





