/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkInputPort.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkInputPort.h"
#include "vtkOutputPort.h"
#include "vtkMultiProcessController.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkImageData.h"
#include "vtkDataInformation.h"
#include "vtkExtent.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkInputPort* vtkInputPort::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkInputPort");
  if(ret)
    {
    return (vtkInputPort*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInputPort;
}





//----------------------------------------------------------------------------
vtkInputPort::vtkInputPort()
{
  this->RemoteProcessId = 0;
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
vtkInputPort::~vtkInputPort()
{
  vtkMultiProcessController *tmp;
  
  // as a precaution.
  tmp = this->Controller;
  this->Controller = NULL;
  tmp->UnRegister(this);
}

//----------------------------------------------------------------------------
void vtkInputPort::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);
  os << indent << "RemoteProcessId: " << this->RemoteProcessId << endl;
  os << indent << "Tag: " << this->Tag << endl;
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "DataTime: " << this->DataTime << endl;
  os << indent << "TransferNeeded: " << this->TransferNeeded << endl;  
}

//----------------------------------------------------------------------------
// Maybe we can come up with a way to check the type of the upstream port's
// input here.  While we are at it, we could automatically generate a tag.
vtkPolyData *vtkInputPort::GetPolyDataOutput()
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
      vtkWarningMacro("vtkInputPort: Changing data type of output.");
      }
    }
  
  output = vtkPolyData::New();
  output->ReleaseData();
  this->vtkSource::SetNthOutput(0, output);
  return (vtkPolyData*)(output);
}


//----------------------------------------------------------------------------
// Maybe we can come up with a way to check the type of the upstream port's
// input here.  While we are at it, we could automatically generate a tag.
vtkUnstructuredGrid *vtkInputPort::GetUnstructuredGridOutput()
{
  vtkDataObject *output = NULL;
  
  // If there is already an output, I hope it is a vtkPolyData.
  if (this->Outputs)
    {
    output = this->Outputs[0];
    }
  if (output)
    {
    if (output->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
      {
      return (vtkUnstructuredGrid*)(output);
      }
    else
      {
      vtkWarningMacro("vtkInputPort: Changing data type of output.");
      }
    }
  
  output = vtkUnstructuredGrid::New();
  output->ReleaseData();
  this->vtkSource::SetNthOutput(0, output);
  return (vtkUnstructuredGrid*)(output);
}

//----------------------------------------------------------------------------
// Maybe we can come up with a way to check the type of the upstream port's
// input here.  While we are at it, we could automatically generate a tag.
vtkStructuredGrid *vtkInputPort::GetStructuredGridOutput()
{
  vtkDataObject *output = NULL;
  
  // If there is already an output, I hope it is a vtkPolyData.
  if (this->Outputs)
    {
    output = this->Outputs[0];
    }
  if (output)
    {
    if (output->GetDataObjectType() == VTK_STRUCTURED_GRID)
      {
      return (vtkStructuredGrid*)(output);
      }
    else
      {
      vtkWarningMacro("vtkInputPort: Changing data type of output.");
      }
    }
  
  output = vtkStructuredGrid::New();
  output->ReleaseData();
  this->vtkSource::SetNthOutput(0, output);
  return (vtkStructuredGrid*)(output);
}


//----------------------------------------------------------------------------
// Maybe we can come up with a way to check the type of the upstream port's
// input here.  While we are at it, we could automatically generate a tag.
vtkRectilinearGrid *vtkInputPort::GetRectilinearGridOutput()
{
  vtkDataObject *output = NULL;
  
  // If there is already an output, I hope it is a vtkPolyData.
  if (this->Outputs)
    {
    output = this->Outputs[0];
    }
  if (output)
    {
    if (output->GetDataObjectType() == VTK_RECTILINEAR_GRID)
      {
      return (vtkRectilinearGrid*)(output);
      }
    else
      {
      vtkWarningMacro("vtkInputPort: Changing data type of output.");
      }
    }
  
  output = vtkRectilinearGrid::New();
  output->ReleaseData();
  this->vtkSource::SetNthOutput(0, output);
  return (vtkRectilinearGrid*)(output);
}


//----------------------------------------------------------------------------
// Maybe we can come up with a way to check the type of the upstream port's
// input here.  While we are at it, we could automatically generate a tag.
vtkStructuredPoints *vtkInputPort::GetStructuredPointsOutput()
{
  vtkDataObject *output = NULL;
  
  // If there is already an output, I hope it is a vtkPolyData.
  if (this->Outputs)
    {
    output = this->Outputs[0];
    }
  if (output)
    {
    if (output->GetDataObjectType() == VTK_STRUCTURED_POINTS)
      {
      return (vtkStructuredPoints*)(output);
      }
    else
      {
      vtkWarningMacro("vtkInputPort: Changing data type of output.");
      }
    }
  
  output = vtkStructuredPoints::New();
  output->ReleaseData();
  this->vtkSource::SetNthOutput(0, output);
  return (vtkStructuredPoints*)(output);
}


//----------------------------------------------------------------------------
// Maybe we can come up with a way to check the type of the upstream port's
// input here.  While we are at it, we could automatically generate a tag.
vtkImageData *vtkInputPort::GetImageDataOutput()
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
      vtkWarningMacro("vtkInputPort: Changing data type of output.");
      }
    }
  
  output = vtkImageData::New();
  output->ReleaseData();
  this->vtkSource::SetNthOutput(0, output);
  return (vtkImageData*)(output);
}


//----------------------------------------------------------------------------
// The only tricky thing here is the translation of the PipelineMTime
// into a value meaningful to this process.
void vtkInputPort::UpdateInformation()
{
  vtkDataObject *output;
  unsigned long pmt;
  
  if (this->Outputs == NULL || this->Outputs[0] == NULL)
    {
    vtkErrorMacro("No output.");
    return;
    }

  output = this->Outputs[0];
  
  // Trigger UpdateInformation in remotePort.
  // Up-stream port should have the same tag.
  this->Controller->TriggerRMI(this->RemoteProcessId, this->Tag);
  
  // Now receive the information
  this->Controller->Receive(output->GetDataInformation(), 
                            this->RemoteProcessId,
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
void vtkInputPort::PreUpdate(vtkDataObject *output)
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
  // we want task parallelism with multiple input filters, 
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

  // Trigger Update in remotePort.
  // remotePort should have the same tag.
  this->Controller->TriggerRMI(this->RemoteProcessId, this->Tag+1);

  // Send the UpdateExtent request.
  this->Controller->Send((vtkObject*)(output->GetGenericUpdateExtent()),
			 this->RemoteProcessId, VTK_PORT_UPDATE_EXTENT_TAG);

  // This is for pipeline parallism.
  // The Upstream port may or may not promote its data (execute).
  // It needs the data time of our output to compare to the mtime
  // of its input to determine if it should send the data (execute).
  this->Controller->Send( &(this->DataTime), 1, this->RemoteProcessId,
			  VTK_PORT_NEW_DATA_TIME_TAG);
  
  // This automatically causes to remotePort to send the data.
  // Tell the update method to receive the data.
  this->TransferNeeded = 1;
}


//----------------------------------------------------------------------------
void vtkInputPort::InternalUpdate(vtkDataObject *output)
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

  // Well here is a bit of a hack.
  // Since the reader will overwrite whole extents, we need to save the whole
  // extent and reset it.
  vtkDataInformation *info = output->GetDataInformation()->MakeObject();
  info->Copy(output->GetDataInformation());  
  // receive the data

  this->Controller->Receive(output, this->RemoteProcessId,
			    VTK_PORT_DATA_TRANSFER_TAG);

  output->GetDataInformation()->Copy(info);
  info->Delete();

  if ( this->EndMethod )
    {
    (*this->EndMethod)(this->EndMethodArg);
    }

  // Receive the data time
  this->Controller->Receive( &(this->DataTime), 1, this->RemoteProcessId,
			    VTK_PORT_NEW_DATA_TIME_TAG);
     
  this->TransferNeeded = 0;
}






















