/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkDownStreamPort.cxx
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
#include "vtkDownStreamPort.h"
#include "vtkUpStreamPort.h"
#include "vtkMultiProcessController.h"
#include "vtkPolyData.h"
#include "vtkImageData.h"
#include "vtkDataInformation.h"
#include "vtkExtent.h"


//----------------------------------------------------------------------------
vtkDownStreamPort::vtkDownStreamPort()
{
  this->UpStreamProcessId = 0;
  this->Tag = 0;
  
  // Controller keeps a reference to this object as well.
  this->Controller = 
    vtkMultiProcessController::RegisterAndGetGlobalController(this);

  // State variables.
  this->TransferNeeded = 0;
  this->DataTime = 0;
}

//----------------------------------------------------------------------------
// We need to have a "GetNetReferenceCount" to avoid memory leaks.
vtkDownStreamPort::~vtkDownStreamPort()
{
  vtkMultiProcessController *tmp;
  
  // as a precaution.
  tmp = this->Controller;
  this->Controller = NULL;
  tmp->UnRegister(this);
}

//----------------------------------------------------------------------------
void vtkDownStreamPort::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);
  os << indent << "UpStreamProcessId: " << this->UpStreamProcessId << endl;
  os << indent << "Tag: " << this->Tag << endl;
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "DataTime: " << this->DataTime << endl;
  os << indent << "TransferNeeded: " << this->TransferNeeded << endl;  
}

//----------------------------------------------------------------------------
// Maybe we can come up with a way to check the type of the upstream port's
// input here.  While we are at it, we could automatically generate a tag.
vtkPolyData *vtkDownStreamPort::GetPolyDataOutput()
{
  vtkDataObject *output = NULL;
  
  // If there is already an output, I hope it is a vtkPolyData.
  if (this->Outputs)
    {
    output = this->Outputs[0];
    }
  if (output)
    {
    if (output->GetDataObjectType() == VTK_POLY_DATA)
      {
      return (vtkPolyData*)(output);
      }
    else
      {
      vtkWarningMacro("vtkDownStreamPort: Changing data type of output.");
      }
    }
  
  output = vtkPolyData::New();
  this->vtkSource::SetOutput(0, output);
  return (vtkPolyData*)(output);
}


//----------------------------------------------------------------------------
// Maybe we can come up with a way to check the type of the upstream port's
// input here.  While we are at it, we could automatically generate a tag.
vtkImageData *vtkDownStreamPort::GetImageDataOutput()
{
  vtkDataObject *output = NULL;
  
  // If there is already an output, I hope it is a vtkImageData.
  if (this->Outputs)
    {
    output = this->Outputs[0];
    }
  if (output)
    {
    if (output->GetDataObjectType() == VTK_IMAGE_DATA)
      {
      return (vtkImageData*)(output);
      }
    else
      {
      vtkWarningMacro("vtkDownStreamPort: Changing data type of output.");
      }
    }
  
  output = vtkImageData::New();
  this->vtkSource::SetOutput(0, output);
  return (vtkImageData*)(output);
}


//----------------------------------------------------------------------------
// The only tricky thing here is the translation of the PipelineMTime
// into a value meaningful to this process.
void vtkDownStreamPort::UpdateInformation()
{
  vtkDataObject *output;
  unsigned long pmt;
  
  if (this->Outputs == NULL || this->Outputs[0] == NULL)
    {
    vtkErrorMacro("No output.");
    return;
    }

  output = this->Outputs[0];
  
  // Trigger UpdateInformation in UpStreamPort.
  // Up-stream port should have the same tag.
  this->Controller->TriggerRMI(this->UpStreamProcessId, this->Tag);

  // Now receive the information
  this->Controller->Receive(output->GetDataInformation(), 
                            this->UpStreamProcessId,
                            VTK_PORT_INFORMATION_TRANSFER_TAG);

  // Convert Pipeline MTime into a value meaningful in this process.
  pmt = output->GetDataInformation()->GetPipelineMTime();

  // Save the upstream PMT for execute check (this may not be necessary)
  this->UpStreamMTime = pmt;

  // !!! Make sure that Update is called if data is released. !!!
  if (pmt > this->DataTime || output->GetDataReleased())
    {
    // Our data is out of data.  We will need a transfer.
    // This Modified call will ensure Update will get called.
    this->Modified();
    }
  output->SetPipelineMTime(this->GetMTime());
  // Locality has to be changed too.
  output->GetDataInformation()->SetLocality(1.0);
}


//----------------------------------------------------------------------------
// The only tricky thing here is the translation of the PipelineMTime
// into a value meaningful to this process.
void vtkDownStreamPort::PreUpdate(vtkDataObject *output)
{
  // This should be cleared by this point.
  // UpdateInformation and Update calls need to be made in pairs.
  if (this->TransferNeeded)
    {
    vtkWarningMacro("Transfer should have been received.");
    return;
    }
  
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // This would normally be done in the Update method, but since
  // we want task parallelism whith multiple input filters, 
  // it needs to be here.

  // Do we need to update?
  // !!! I am uneasy about the Released check.  Although a new update extent
  // will cause the data to be released,  released data does not imply 
  // Update will be called !!!!
  if (this->UpStreamMTime <= this->DataTime && ! output->GetDataReleased())
    { 
    // No, we do not need to update.
    return;
    }

  // Trigger Update in UpStreamPort.
  // UpStreamPort should have the same tag.
  this->Controller->TriggerRMI(this->UpStreamProcessId, this->Tag+1);

  // Send the UpdateExtent request.
  this->Controller->Send((vtkObject*)(output->GetGenericUpdateExtent()),
			 this->UpStreamProcessId, VTK_PORT_UPDATE_EXTENT_TAG);

  // This automatically causes to UpStreamPort to send the data.
  // Tell the update method to receive the data.
  this->TransferNeeded = 1;
}


//----------------------------------------------------------------------------
void vtkDownStreamPort::InternalUpdate(vtkDataObject *output)
{

  if ( ! this->TransferNeeded)
    {
    // If something unexpected happened, let me know.
    vtkWarningMacro("InternalUpdate was called when no data was needed.");
    return;
    }
  
  if ( this->StartMethod )
    {
    (*this->StartMethod)(this->StartMethodArg);
    }    
  // receive the data
  this->Controller->Receive(output, this->UpStreamProcessId,
			    VTK_PORT_DATA_TRANSFER_TAG);
  if ( this->EndMethod )
    {
    (*this->EndMethod)(this->EndMethodArg);
    }

  // Receive the data time
  this->Controller->Receive( &(this->DataTime), 1, this->UpStreamProcessId,
			    VTK_PORT_NEW_DATA_TIME_TAG);
     
      
  this->TransferNeeded = 0;
}






















