/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInputPort.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInputPort.h"

#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutputPort.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkInputPort, "1.23");
vtkStandardNewMacro(vtkInputPort);

vtkCxxSetObjectMacro(vtkInputPort,Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkInputPort::vtkInputPort()
{
  this->RemoteProcessId = 0;
  this->Tag = 0;
  
  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  // State variables.
  this->DataTime = 0;

  this->DoUpdateInformation = 1;

  this->LastUpdatePiece = -1;
  this->LastUpdateNumberOfPieces = -1;
  this->LastUpdateGhostLevel = -1;

  this->LastUpdateExtent[0] = this->LastUpdateExtent[1] = 
    this->LastUpdateExtent[2] = this->LastUpdateExtent[3] = 
    this->LastUpdateExtent[4] = this->LastUpdateExtent[5] = 0;

  // from a pipeline perspective this has no inputs
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
// We need to have a "GetNetReferenceCount" to avoid memory leaks.
vtkInputPort::~vtkInputPort()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkInputPort::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RemoteProcessId: " << this->RemoteProcessId << endl;
  os << indent << "Tag: " << this->Tag << endl;
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "DataTime: " << this->DataTime << endl;
  os << indent << "DoUpdateInformation: " << this->DoUpdateInformation << endl;
}

int vtkInputPort::UpdateExtentIsOutsideOfTheExtent(vtkDataObject *output)
  {
  switch ( output->GetExtentType() )
    {
    case VTK_PIECES_EXTENT:
      if ( this->LastUpdatePiece != output->GetUpdatePiece() ||
           this->LastUpdateNumberOfPieces != output->GetUpdateNumberOfPieces() ||
           this->LastUpdateGhostLevel != output->GetUpdateGhostLevel())
        {
        return 1;
        }
      break;

    case VTK_3D_EXTENT:
      int extent[6];
      output->GetUpdateExtent(extent);
      if ( extent[0] < this->LastUpdateExtent[0] ||
           extent[1] > this->LastUpdateExtent[1] ||
           extent[2] < this->LastUpdateExtent[2] ||
           extent[3] > this->LastUpdateExtent[3] ||
           extent[4] < this->LastUpdateExtent[4] ||
           extent[5] > this->LastUpdateExtent[5] )
        {
        return 1;
        }
      break;

    // We should never have this case occur
    default:
      vtkErrorMacro( << "Internal error - invalid extent type!" );
      break;
    }
  return 0;

  }

unsigned long vtkInputPort::GetMTime()
{
  if (this->DoUpdateInformation && this->Controller)
    {
    vtkDataObject *output = 0;
    vtkInformation* outInfo = this->GetExecutive()->GetOutputInformation(0);
    if (outInfo)
      {
      output = outInfo->Get(vtkDataObject::DATA_OBJECT());
      }
    
    if (!output)
      {
      return this->Superclass::GetMTime();
      }

    // Trigger UpdateInformation in remotePort.
    // Up-stream port should have the same tag.
    this->Controller->TriggerRMI(this->RemoteProcessId, this->Tag);
  
    // Now receive the information
    int wholeInformation[7];
    unsigned long pmt = 0;
    this->Controller->Receive( wholeInformation, 7, 
                               this->RemoteProcessId,
                               vtkInputPort::INFORMATION_TRANSFER_TAG);

    this->Controller->Receive( &pmt, 1, 
                               this->RemoteProcessId,
                               vtkInputPort::INFORMATION_TRANSFER_TAG);
    int maxNumPieces = 0;
    this->Controller->Receive( &maxNumPieces, 1, 
                               this->RemoteProcessId,
                               vtkInputPort::INFORMATION_TRANSFER_TAG);

    // Save the upstream PMT for execute check (this may not be necessary)
    this->UpStreamMTime = pmt;

    // !!! Make sure that Update is called if data is released. !!!
    if (pmt > this->DataTime || output->GetDataReleased())
      {
      // Our data is out of data.  We will need a transfer.
      // This Modified call will ensure Update will get called.
      this->Modified();
      }
    }
  
  return this->Superclass::GetMTime();
}

int vtkInputPort::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector)
{  
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  unsigned long pmt = 0;
  
  if (!this->DoUpdateInformation)
    {
    return 1;
    }

  // Trigger UpdateInformation in remotePort.
  // Up-stream port should have the same tag.
  this->Controller->TriggerRMI(this->RemoteProcessId, this->Tag);
  
  // Now receive the information
  int wholeInformation[7];
  this->Controller->Receive( wholeInformation, 7, 
                             this->RemoteProcessId,
                             vtkInputPort::INFORMATION_TRANSFER_TAG);

  this->Controller->Receive( &pmt, 1, 
                             this->RemoteProcessId,
                             vtkInputPort::INFORMATION_TRANSFER_TAG);
  int maxNumPieces = 0;
  this->Controller->Receive( &maxNumPieces, 1, 
                             this->RemoteProcessId,
                             vtkInputPort::INFORMATION_TRANSFER_TAG);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               maxNumPieces);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               wholeInformation, 6);
    
  // Save the upstream PMT for execute check (this may not be necessary)
  this->UpStreamMTime = pmt;

  if (pmt > this->DataTime)
    {
    // Our data is out of data.  We will need a transfer.
    // This Modified call will ensure Update will get called.
    this->Modified();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkInputPort::RequestData(vtkInformation* vtkNotUsed(request),
                              vtkInformationVector** vtkNotUsed( inputVector),
                              vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  // Trigger Update in remotePort.
  // remotePort should have the same tag.
  this->Controller->TriggerRMI(this->RemoteProcessId, this->Tag+1);
  
  // Send the UpdateExtent request. The first 6 ints are the 3d extent, the
  // next to are the pieces extent (we don't know which type it is - just
  // send both)
  int extent[9];
  output->GetUpdateExtent( extent );
  extent[6] = output->GetUpdatePiece();
  extent[7] = output->GetUpdateNumberOfPieces();  
  extent[8] = output->GetUpdateGhostLevel();  
  this->Controller->Send( extent, 9, this->RemoteProcessId, 
                          vtkInputPort::UPDATE_EXTENT_TAG);

  if (this->UpStreamMTime <= this->DataTime && ! output->GetDataReleased() &&
      !this->UpdateExtentIsOutsideOfTheExtent(output) )
    { 
    // No, we do not need to update.
    return 1;
    }

  // we need the data so we send another request to get the data
  this->Controller->TriggerRMI(this->RemoteProcessId, this->Tag+3);
  
  // Well here is a bit of a hack.
  // Since the reader will overwrite whole extents, we need to save the whole
  // extent and reset it.
  int wholeExtent[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExtent);

  // receive the data
  this->Controller->Receive(output, this->RemoteProcessId,
                            vtkInputPort::DATA_TRANSFER_TAG);
  
  output->SetWholeExtent( wholeExtent );
  
  // Receive the data time
  this->Controller->Receive( &(this->DataTime), 1, this->RemoteProcessId,
                             vtkInputPort::NEW_DATA_TIME_TAG);
     
  this->LastUpdatePiece = output->GetUpdatePiece();
  this->LastUpdateNumberOfPieces = output->GetUpdateNumberOfPieces();
  this->LastUpdateGhostLevel = output->GetUpdateGhostLevel();
  
  this->SetLastUpdateExtent( output->GetUpdateExtent() );

  return 1;
}

int vtkInputPort::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  // invoke super first
  int retVal = this->Superclass::FillOutputPortInformation(port, info);
  
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  
  return retVal;
}

//----------------------------------------------------------------------------
int vtkInputPort::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector** vtkNotUsed( inputVector ), 
  vtkInformationVector* outputVector)
{
  // if the controller has not been set yet then we have a problem
  if (!this->Controller)
    {
    vtkErrorMacro("Attempt to use input port withotu a controller!");
    return 0;
    }
  
  // Trigger 
  // Up-stream port should have the same tag.
  this->Controller->TriggerRMI(this->RemoteProcessId, this->Tag+2);
  
  // Now receive the information
  int dataType = 0;
  this->Controller->Receive( &dataType, 1, 
                             this->RemoteProcessId,
                             vtkInputPort::DATA_TYPE_TAG);
  
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  
  if (!output || output->GetDataObjectType() != dataType) 
    {
    output = 0;
    switch (dataType)
      {
      case VTK_POLY_DATA:
        output = vtkPolyData::New();
        break;
      case VTK_STRUCTURED_GRID:
        output = vtkStructuredGrid::New();
        break;        
      case VTK_RECTILINEAR_GRID:
        output = vtkRectilinearGrid::New();
        break;
      case VTK_UNSTRUCTURED_GRID:
        output = vtkUnstructuredGrid::New();
        break;
      case VTK_IMAGE_DATA:
        output = vtkImageData::New();
        break;
      case VTK_STRUCTURED_POINTS:
        vtkErrorMacro("vtkStructuredPoints are being deprecated. Please use "
                      "vtkImageData instead");
        break;
      }
    if (output)
      {
      output->SetPipelineInformation(info);
      output->Delete();
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      }
    }
  return 1;
}
