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
#include "vtkExtent.h"

//----------------------------------------------------------------------------
vtkUpStreamPort::vtkUpStreamPort()
{
  this->Tag = -1;
  
  // Controller keeps a reference to this object as well.
  this->Controller = 
    vtkMultiProcessController::RegisterAndGetGlobalController(this);
  
  this->PipelineFlag = 0;
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
// Remote method call to UpdateInformation and send the information downstream.
// This should be a friend.
void vtkUpStreamPortUpdateInformationCallBack(void *arg, int remoteProcessId)  
{
  vtkUpStreamPort *self = (vtkUpStreamPort*)arg;
  
  // Just call a method
  self->TriggerUpdateInformation(remoteProcessId);
}
//----------------------------------------------------------------------------
void vtkUpStreamPort::TriggerUpdateInformation(int remoteProcessId)
{
  vtkDataObject *input = this->GetInput();
  
  // Handle no input gracefully.
  if ( input != NULL )
    {
    input->UpdateInformation();
    }

  // Now just send the information downstream.
  // PipelineMTime is part of information, so downstream
  // port will make the time comparison, and call Update if necessary.
  this->Controller->Send( (vtkObject*)(input->GetDataInformation()), 
		  remoteProcessId, VTK_PORT_INFORMATION_TRANSFER_TAG);
}


//----------------------------------------------------------------------------
// Remote method call to Update and send data downstream.
// This should be a friend.
void vtkUpStreamPortUpdateCallBack(void *arg, int remoteProcessId)  
{
  vtkUpStreamPort *self = (vtkUpStreamPort*)arg;
  
  // Just call a method
  self->TriggerUpdate(remoteProcessId);
}
//----------------------------------------------------------------------------
void vtkUpStreamPort::TriggerUpdate(int remoteProcessId)
{
  unsigned long downDataTime;
  vtkDataObject *input = this->GetInput();
  
  // First get the update extent requested.
  this->Controller->Receive((vtkObject*)(input->GetGenericUpdateExtent()),
			    remoteProcessId, 
			    VTK_PORT_UPDATE_EXTENT_TAG);
  
  
  // Postpone the update if we want pipeline parallism.
  // Handle no input gracefully. (Not true: Later we will send a NULL input.)
  if (this->PipelineFlag == 0)
    {
    if ( input != NULL )
      {
      input->PreUpdate();
      input->InternalUpdate();
      }
    }

  // This is to time the marshaling and transfer of data.
  if ( this->StartMethod )
    {
    (*this->StartMethod)(this->StartMethodArg);
    }   
  
  // First transfer the new data.
  this->Controller->Send( input, remoteProcessId,
			  VTK_PORT_DATA_TRANSFER_TAG);

  if ( this->EndMethod )
    {
    (*this->EndMethod)(this->EndMethodArg);
    }

  // Postpone the update if we want pipeline parallism.
  // Handle no input gracefully. (Not true: Later we will send a NULL input.)
  if (this->PipelineFlag)
    {
    if ( input != NULL )
      {
      input->PreUpdate();
      input->InternalUpdate();
      }
    }

  // Since this time has to be local to downstream process
  // and we have no data, we have to create a time here.
  // (The output data usually does this.) 
  this->UpdateTime.Modified();
  
  // Since this UpStreamPort can have multiple DownStreamPorts
  // and the DownStreamPort makes the update-descision time comparison,
  // the DownStreamPort has to store this time.
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
// We need to create two RMIs when the tag is set.
// This means we must generate two tags form this ports tag.
// The ports tag should be even. 
// (I do not like this, but is there another solution?)
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
    this->Controller->RemoveRMI(vtkUpStreamPortUpdateInformationCallBack, 
                                (void *)this, this->Tag);
    this->Controller->RemoveRMI(vtkUpStreamPortUpdateCallBack, 
                                (void *)this, this->Tag + 1);
    }
  
  this->Tag = tag;
  this->Controller->AddRMI(vtkUpStreamPortUpdateInformationCallBack, (void *)this, tag);
  this->Controller->AddRMI(vtkUpStreamPortUpdateCallBack, 
                           (void *)this, tag+1);
}





