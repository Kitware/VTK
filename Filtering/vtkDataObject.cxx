/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataObject.h"

#include "vtkAlgorithmOutput.h"
#include "vtkExtentTranslator.h"
#include "vtkFieldData.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"
#include "vtkTrivialProducer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationStringKey.h"

vtkCxxRevisionMacro(vtkDataObject, "1.2.2.8");
vtkStandardNewMacro(vtkDataObject);

vtkCxxSetObjectMacro(vtkDataObject,Information,vtkInformation);
vtkCxxSetObjectMacro(vtkDataObject,FieldData,vtkFieldData);

vtkInformationKeyMacro(vtkDataObject, DATA_TYPE_NAME, String);
vtkInformationKeyMacro(vtkDataObject, DATA_OBJECT, DataObject);
vtkInformationKeyMacro(vtkDataObject, DATA_EXTENT_TYPE, Integer);
vtkInformationKeyMacro(vtkDataObject, DATA_PIECE_NUMBER, Integer);
vtkInformationKeyMacro(vtkDataObject, DATA_NUMBER_OF_PIECES, Integer);
vtkInformationKeyMacro(vtkDataObject, DATA_NUMBER_OF_GHOST_LEVELS, Integer);
vtkInformationKeyMacro(vtkDataObject, SCALAR_TYPE, Integer);
vtkInformationKeyMacro(vtkDataObject, SCALAR_NUMBER_OF_COMPONENTS, Integer);
vtkInformationKeyMacro(vtkDataObject, FIELD_ARRAY_TYPE, Integer);
vtkInformationKeyMacro(vtkDataObject, FIELD_ASSOCIATION, Integer);
vtkInformationKeyMacro(vtkDataObject, FIELD_ATTRIBUTE_TYPE, Integer);
vtkInformationKeyMacro(vtkDataObject, FIELD_NAME, String);
vtkInformationKeyMacro(vtkDataObject, FIELD_NUMBER_OF_COMPONENTS, Integer);
vtkInformationKeyMacro(vtkDataObject, FIELD_NUMBER_OF_TUPLES, Integer);
vtkInformationKeyMacro(vtkDataObject, FIELD_OPERATION, Integer);
vtkInformationKeyRestrictedMacro(vtkDataObject, DATA_EXTENT, IntegerVector, 6);

// Initialize static member that controls global data release 
// after use by filter
static int vtkDataObjectGlobalReleaseDataFlag = 0;

//----------------------------------------------------------------------------
vtkDataObject::vtkDataObject()
{
  this->Source = NULL;
  this->PipelineInformation = 0;

  this->Information = vtkInformation::New();

  // We have to assume that if a user is creating the data on their own,
  // then they will fill it with valid data.
  this->DataReleased = 0;

  this->FieldData = NULL;
  vtkFieldData *fd = vtkFieldData::New();
  this->SetFieldData(fd);
  fd->Delete();

  this->PipelineMTime = 0;

  this->RequestExactExtent = 0;
  
  this->Locality = 0.0;

  this->LastUpdateExtentWasOutsideOfTheExtent = 0;

  this->NumberOfConsumers = 0;
  this->Consumers = 0;
  this->GarbageCollecting = 0;
}

//----------------------------------------------------------------------------
vtkDataObject::~vtkDataObject()
{
  this->SetPipelineInformation(0);
  this->SetInformation(0);
  this->SetFieldData(NULL);

  delete [] this->Consumers;
}

//----------------------------------------------------------------------------
void vtkDataObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Source )
    {
    os << indent << "Source: " << this->Source << "\n";
    }
  else
    {
    os << indent << "Source: (none)\n";
    }

  if ( this->Information )
    {
    os << indent << "Information: " << this->Information << "\n";
    }
  else
    {
    os << indent << "Information: (none)\n";
    }

  os << indent << "Data Released: "
     << (this->DataReleased ? "True\n" : "False\n");
  os << indent << "Global Release Data: "
     << (vtkDataObjectGlobalReleaseDataFlag ? "On\n" : "Off\n");

  os << indent << "PipelineMTime: " << this->PipelineMTime << endl;
  os << indent << "UpdateTime: " << this->UpdateTime << endl;

  if(vtkInformation* pInfo = this->GetPipelineInformation())
    {
    os << indent << "Release Data: "
       << (this->GetReleaseDataFlag() ? "On\n" : "Off\n");
    if(pInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED()))
      {
      os << indent << "UpdateExtent: Initialized\n";
      }
    else
      {
      os << indent << "UpdateExtent: Not Initialized\n";
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
      {
      int updateExtent[6] = {0,-1,0,-1,0,-1};
      this->GetUpdateExtent(updateExtent);
      os << indent << "UpdateExtent: " << updateExtent[0] << ", "
         << updateExtent[1] << ", " << updateExtent[2] << ", "
         << updateExtent[3] << ", " << updateExtent[4] << ", "
         << updateExtent[5] << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
      {
      os << indent << "Update Number Of Pieces: "
         << pInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES())
         << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
      {
      os << indent << "Update Piece: "
         << pInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER())
         << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()))
      {
      os << indent << "Update Ghost Level: "
         << pInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS())
         << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
      {
      int wholeExtent[6] = {0,-1,0,-1,0,-1};
      this->GetWholeExtent(wholeExtent);
      os << indent << "WholeExtent: " << wholeExtent[0] << ", "
         << wholeExtent[1] << ", " << wholeExtent[2] << ", "
         << wholeExtent[3] << ", " << wholeExtent[4] << ", "
         << wholeExtent[5] << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()))
      {
      os << indent << "MaximumNumberOfPieces: "
         << pInfo->Get(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES())
         << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR()))
      {
      os << indent << "ExtentTranslator: ("
         << pInfo->Get(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR())
         << ")\n";
      }
    }

  if (this->RequestExactExtent)
    {
    os << indent << "RequestExactExtent: On\n ";
    }
  else
    {
    os << indent << "RequestExactExtent: Off\n ";
    }

  os << indent << "Field Data:\n";
  this->FieldData->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Locality: " << this->Locality << endl;
  os << indent << "NumberOfConsumers: " << this->NumberOfConsumers << endl;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetPipelineInformation(vtkInformation* newInfo)
{
  vtkInformation* oldInfo = this->PipelineInformation;
  if(newInfo != oldInfo)
    {
    if(newInfo)
      {
      // Reference the new information.
      newInfo->Register(this);

      // Detach the output that used to be held by the new information.
      if(vtkDataObject* oldData = newInfo->Get(vtkDataObject::DATA_OBJECT()))
        {
        oldData->SetPipelineInformation(0);
        }

      // Tell the new information about this object.
      newInfo->Set(vtkDataObject::DATA_OBJECT(), this);
      }

    // Save the pointer to the new information.
    this->PipelineInformation = newInfo;

    if(oldInfo)
      {
      // Remove the old information's reference to us.
      oldInfo->Set(vtkDataObject::DATA_OBJECT(), 0);

      // Remove our reference to the old information.
      oldInfo->UnRegister(this);
      }

    // Set the Source ivar for backwards compatibility.
    this->Source = 0;
    if(vtkExecutive* executive = this->GetExecutive())
      {
      if(vtkAlgorithmOutput* producerPort = executive->GetProducerPort(this))
        {
        this->Source = vtkSource::SafeDownCast(producerPort->GetProducer());
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkDataObject::GetProducerPort()
{
  // Make sure there is an executive.
  if(!this->GetExecutive())
    {
    vtkTrivialProducer* tp = vtkTrivialProducer::New();
    tp->SetOutput(this);
    tp->Delete();
    }

  // Get the port from the executive.
  return this->GetExecutive()->GetProducerPort(this);
}

//----------------------------------------------------------------------------
// Determine the modified time of this object
unsigned long int vtkDataObject::GetMTime()
{
  unsigned long result;

  result = vtkObject::GetMTime();
  if ( this->FieldData )
    {
    unsigned long mtime = this->FieldData->GetMTime();
    result = ( mtime > result ? mtime : result);
    }
  
  return result;
}

//----------------------------------------------------------------------------
void vtkDataObject::Initialize()
{
  this->FieldData->Initialize();
  this->Modified();
}

void vtkDataObject::AddConsumer(vtkObject *c)
{
  // make sure it isn't already there
  if (this->IsConsumer(c))
    {
    return;
    }
  // add it to the list, reallocate memory
  vtkObject **tmp = this->Consumers;
  this->NumberOfConsumers++;
  this->Consumers = new vtkObject* [this->NumberOfConsumers];
  for (int i = 0; i < (this->NumberOfConsumers-1); i++)
    {
    this->Consumers[i] = tmp[i];
    }
  this->Consumers[this->NumberOfConsumers-1] = c;
  // free old memory
  delete [] tmp;
}

void vtkDataObject::RemoveConsumer(vtkObject *c)
{
  // make sure it is already there
  if (!this->IsConsumer(c))
    {
    return;
    }
  // remove it from the list, reallocate memory
  vtkObject **tmp = this->Consumers;
  this->NumberOfConsumers--;
  this->Consumers = new vtkObject* [this->NumberOfConsumers];
  int cnt = 0;
  int i;
  for (i = 0; i <= this->NumberOfConsumers; i++)
    {
    if (tmp[i] != c)
      {
      this->Consumers[cnt] = tmp[i];
      cnt++;
      }
    }
  // free old memory
  delete [] tmp;
}

int vtkDataObject::IsConsumer(vtkObject *c)
{
  int i;
  for (i = 0; i < this->NumberOfConsumers; i++)
    {
    if (this->Consumers[i] == c)
      {
      return 1;
      }
    }
  return 0;
}

vtkObject *vtkDataObject::GetConsumer(int i)
{
  if (i >= this->NumberOfConsumers)
    {
    return 0;
    }
  return this->Consumers[i];
}

//----------------------------------------------------------------------------
void vtkDataObject::SetGlobalReleaseDataFlag(int val)
{
  if (val == vtkDataObjectGlobalReleaseDataFlag)
    {
    return;
    }
  vtkDataObjectGlobalReleaseDataFlag = val;
}

//----------------------------------------------------------------------------

void vtkDataObject::DataHasBeenGenerated()
{
  this->DataReleased = 0;
  this->UpdateTime.Modified();

  // This is here so that the data can be easlily marked as up to date.
  // It is used specifically when the filter vtkQuadricClustering
  // is executed manually with the append methods.
  this->Information->Set(DATA_PIECE_NUMBER(), this->GetUpdatePiece());
  this->Information->Set(DATA_NUMBER_OF_PIECES(), this->GetUpdateNumberOfPieces());
  this->Information->Set(DATA_NUMBER_OF_GHOST_LEVELS(), this->GetUpdateGhostLevel());
}

//----------------------------------------------------------------------------
int vtkDataObject::GetGlobalReleaseDataFlag()
{
  return vtkDataObjectGlobalReleaseDataFlag;
}

//----------------------------------------------------------------------------
void vtkDataObject::ReleaseData()
{
  this->Initialize();
  this->DataReleased = 1;
}

//----------------------------------------------------------------------------
int vtkDataObject::ShouldIReleaseData()
{
  if ( vtkDataObjectGlobalReleaseDataFlag || this->GetReleaseDataFlag() )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::SetRequestExactExtent( int flag )
{
  this->RequestExactExtent = flag;
}

//----------------------------------------------------------------------------
void vtkDataObject::CopyPipelineInformation(vtkInformation* oldPInfo,
                                            vtkInformation* newPInfo)
{
  newPInfo->CopyEntry(oldPInfo, vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  newPInfo->CopyEntry(oldPInfo, vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES());
  newPInfo->CopyEntry(oldPInfo, vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED());
  newPInfo->CopyEntry(oldPInfo, vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  newPInfo->CopyEntry(oldPInfo, vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  newPInfo->CopyEntry(oldPInfo, vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  newPInfo->CopyEntry(oldPInfo, vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
void vtkDataObject::SetSource(vtkSource* newSource)
{
  vtkDebugMacro( << this->GetClassName() << " ("
                 << this << "): setting Source to " << newSource );
  if(newSource)
    {
    // Find the output index on the source producing this data object.
    int index = newSource->GetOutputIndex(this);
    if(index >= 0)
      {
      newSource->GetExecutive()->SetOutputData(newSource, index, this);
      }
    else
      {
      vtkErrorMacro("SetSource cannot find the output index of this "
                    "data object from the source.");
      this->SetPipelineInformation(0);
      }
    }
  else
    {
    this->SetPipelineInformation(0);
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::UnRegister(vtkObjectBase *o)
{
  int check = (this->GetReferenceCount() > 1);
  this->Superclass::UnRegister(o);
  if(check && !this->GarbageCollecting)
    {
    vtkGarbageCollector::Check(this);
    }
}

//----------------------------------------------------------------------------
unsigned long vtkDataObject::GetUpdateTime()
{
  return this->UpdateTime.GetMTime();
}

unsigned long vtkDataObject::GetEstimatedMemorySize()
{
  // This should be implemented in a subclass. If not, default to
  // estimating that no memory is used.
  return 0;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkDataObject::GetExecutive()
{
  if(this->PipelineInformation)
    {
    return this->PipelineInformation->Get(vtkExecutive::EXECUTIVE());
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkDataObject::GetPortNumber()
{
  if(this->PipelineInformation)
    {
    return this->PipelineInformation->Get(vtkExecutive::PORT_NUMBER());
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkStreamingDemandDrivenPipeline* vtkDataObject::TrySDDP(const char* method)
{
  // Make sure there is an executive.
  if(!this->GetExecutive())
    {
    vtkTrivialProducer* tp = vtkTrivialProducer::New();
    tp->SetOutput(this);
    tp->Delete();
    }

  // Try downcasting the executive to the proper type.
  if(SDDP* sddp = SDDP::SafeDownCast(this->GetExecutive()))
    {
    return sddp;
    }
  else if(method)
    {
    vtkErrorMacro("Method " << method << " cannot be called unless the "
                  "data object is managed by a "
                  "vtkStreamingDemandDrivenPipeline.");
    }
  return 0;
}

//----------------------------------------------------------------------------

unsigned long vtkDataObject::GetActualMemorySize()
{
  return this->FieldData->GetActualMemorySize();
}

//----------------------------------------------------------------------------

void vtkDataObject::CopyInformation( vtkDataObject *data )
{
  if ( this->GetExtentType() == VTK_3D_EXTENT &&
       data->GetExtentType() == VTK_3D_EXTENT )
    {
    this->SetWholeExtent(data->GetWholeExtent());
    }
  else
    {
    this->SetMaximumNumberOfPieces(data->GetMaximumNumberOfPieces());
    }
  this->SetExtentTranslator(data->GetExtentTranslator());
}

//----------------------------------------------------------------------------
void vtkDataObject::ShallowCopy(vtkDataObject *src)
{
  if (!src)
    {
    vtkWarningMacro("Attempted to ShallowCopy from null.");
    return;
    }

  this->InternalDataObjectCopy(src);

  if (!src->FieldData)
    {
    this->SetFieldData(0);
    }
  else
    {
    if (this->FieldData)
      {
      this->FieldData->ShallowCopy(src->FieldData);
      }
    else
      {
      vtkFieldData* fd = vtkFieldData::New();
      fd->ShallowCopy(src->FieldData);
      this->SetFieldData(fd);
      fd->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::DeepCopy(vtkDataObject *src)
{
  vtkFieldData *srcFieldData = src->GetFieldData();
  
  this->InternalDataObjectCopy(src);

  if (srcFieldData)
    {
    vtkFieldData *newFieldData = vtkFieldData::New();
    newFieldData->DeepCopy(srcFieldData);
    this->SetFieldData(newFieldData);
    newFieldData->Delete();
    }
  else
    {
    this->SetFieldData(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::InternalDataObjectCopy(vtkDataObject *src)
{
  // If the input data object has pipeline information and this object
  // does not, setup a trivial producer so that this object will have
  // pipeline information into which to copy values.
  if(src->GetPipelineInformation() && !this->GetPipelineInformation())
    {
    this->GetProducerPort();
    }

  this->DataReleased = src->DataReleased;
  if(src->Information->Has(DATA_EXTENT()))
    {
    this->Information->Set(DATA_EXTENT(),
                           src->Information->Get(DATA_EXTENT()),
                           6);
    }
  if(src->Information->Has(DATA_PIECE_NUMBER()))
    {
    this->Information->Set(DATA_PIECE_NUMBER(),
                           src->Information->Get(DATA_PIECE_NUMBER()));
    }
  if(src->Information->Has(DATA_NUMBER_OF_PIECES()))
    {
    this->Information->Set(DATA_NUMBER_OF_PIECES(),
                           src->Information->Get(DATA_NUMBER_OF_PIECES()));
    }
  if(src->Information->Has(DATA_NUMBER_OF_GHOST_LEVELS()))
    {
    this->Information->Set(DATA_NUMBER_OF_GHOST_LEVELS(),
                           src->Information->Get(DATA_NUMBER_OF_GHOST_LEVELS()));
    }
  vtkInformation* thatPInfo = src->GetPipelineInformation();
  vtkInformation* thisPInfo = this->GetPipelineInformation();
  if(thisPInfo && thatPInfo)
    {
    // copy the pipeline info if it is available
    if(thisPInfo)
      {
      thisPInfo->CopyEntry(thatPInfo, SDDP::WHOLE_EXTENT());
      thisPInfo->CopyEntry(thatPInfo, SDDP::MAXIMUM_NUMBER_OF_PIECES());
      thisPInfo->CopyEntry(thatPInfo, vtkDemandDrivenPipeline::RELEASE_DATA());
      }
    }
  // This also caused a pipeline problem.
  // An input pipelineMTime was copied to output.  Pipeline did not execute...
  // We do not copy MTime of object, so why should we copy these.
  //this->PipelineMTime = src->PipelineMTime;
  //this->UpdateTime = src->UpdateTime;
  //this->Locality = src->Locality;
}

//----------------------------------------------------------------------------
// This should be a pure virutal method.
void vtkDataObject::Crop()
{
}

//----------------------------------------------------------------------------
void vtkDataObject::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  if(this->Information)
    {
    collector->ReportReference(this->Information, "Information");
    }
  if(this->PipelineInformation)
    {
    collector->ReportReference(this->PipelineInformation, "PipelineInformation");
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::GarbageCollectionStarting()
{
  this->GarbageCollecting = 1;
  this->Superclass::GarbageCollectionStarting();
}

//----------------------------------------------------------------------------
void vtkDataObject::RemoveReferences()
{
  if(this->PipelineInformation)
    {
    this->PipelineInformation->UnRegister(this);
    this->PipelineInformation = 0;
    }
  if(this->Information)
    {
    this->Information->UnRegister(this);
    this->Information = 0;
    }
  this->Superclass::RemoveReferences();
}

//----------------------------------------------------------------------------
void vtkDataObject::Update()
{
  if(SDDP* sddp = this->TrySDDP("Update"))
    {
    sddp->Update(this->GetPortNumber());
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::UpdateInformation()
{
  if(SDDP* sddp = this->TrySDDP("UpdateInformation"))
    {
    sddp->UpdateInformation();
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::PropagateUpdateExtent()
{
  if(SDDP* sddp = this->TrySDDP("PropagateUpdateExtent"))
    {
    sddp->PropagateUpdateExtent(this->GetPortNumber());
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::TriggerAsynchronousUpdate()
{
}

//----------------------------------------------------------------------------
void vtkDataObject::UpdateData()
{
  if(SDDP* sddp = this->TrySDDP("UpdateData"))
    {
    sddp->UpdateData(this->GetPortNumber());
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdateExtentToWholeExtent()
{
  if(SDDP* sddp = this->TrySDDP("SetUpdateExtentToWholeExtent"))
    {
    if(sddp->SetUpdateExtentToWholeExtent(this->GetPortNumber()))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::SetMaximumNumberOfPieces(int n)
{
  if(SDDP* sddp = this->TrySDDP("SetMaximumNumberOfPieces"))
    {
    if(sddp->SetMaximumNumberOfPieces(this->GetPortNumber(), n))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int vtkDataObject::GetMaximumNumberOfPieces()
{
  if(SDDP* sddp = this->TrySDDP("GetMaximumNumberOfPieces"))
    {
    return sddp->GetMaximumNumberOfPieces(this->GetPortNumber());
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetWholeExtent(int x0, int x1, int y0, int y1,
                                   int z0, int z1)
{
  int extent[6] = {x0, x1, y0, y1, z0, z1};
  this->SetWholeExtent(extent);
}

//----------------------------------------------------------------------------
void vtkDataObject::SetWholeExtent(int extent[6])
{
  if(SDDP* sddp = this->TrySDDP("SetWholeExtent"))
    {
    if(sddp->SetWholeExtent(this->GetPortNumber(), extent))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int* vtkDataObject::GetWholeExtent()
{
  if(SDDP* sddp = this->TrySDDP("GetWholeExtent"))
    {
    return sddp->GetWholeExtent(this->GetPortNumber());
    }
  else
    {
    static int extent[6] = {0,-1,0,-1,0,-1};
    return extent;
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::GetWholeExtent(int& x0, int& x1, int& y0, int& y1,
                                   int& z0, int& z1)
{
  int extent[6];
  this->GetWholeExtent(extent);
  x0 = extent[0];
  x1 = extent[1];
  y0 = extent[2];
  y1 = extent[3];
  z0 = extent[4];
  z1 = extent[5];
}

//----------------------------------------------------------------------------
void vtkDataObject::GetWholeExtent(int extent[6])
{
  if(SDDP* sddp = this->TrySDDP("GetWholeExtent"))
    {
    sddp->GetWholeExtent(this->GetPortNumber(), extent);
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdateExtent(int x0, int x1, int y0, int y1,
                                   int z0, int z1)
{
  int extent[6] = {x0, x1, y0, y1, z0, z1};
  this->SetUpdateExtent(extent);
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdateExtent(int extent[6])
{
  if(SDDP* sddp = this->TrySDDP("SetUpdateExtent"))
    {
    if(sddp->SetUpdateExtent(this->GetPortNumber(), extent))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int* vtkDataObject::GetUpdateExtent()
{
  if(SDDP* sddp = this->TrySDDP("GetUpdateExtent"))
    {
    return sddp->GetUpdateExtent(this->GetPortNumber());
    }
  else
    {
    static int extent[6] = {0,-1,0,-1,0,-1};
    return extent;
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::GetUpdateExtent(int& x0, int& x1, int& y0, int& y1,
                                   int& z0, int& z1)
{
  int extent[6];
  this->GetUpdateExtent(extent);
  x0 = extent[0];
  x1 = extent[1];
  y0 = extent[2];
  y1 = extent[3];
  z0 = extent[4];
  z1 = extent[5];
}

//----------------------------------------------------------------------------
void vtkDataObject::GetUpdateExtent(int extent[6])
{
  if(SDDP* sddp = this->TrySDDP("GetUpdateExtent"))
    {
    sddp->GetUpdateExtent(this->GetPortNumber(), extent);
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdatePiece(int piece)
{
  if(SDDP* sddp = this->TrySDDP("SetUpdatePiece"))
    {
    if(sddp->SetUpdatePiece(this->GetPortNumber(), piece))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int vtkDataObject::GetUpdatePiece()
{
  if(SDDP* sddp = this->TrySDDP("GetUpdatePiece"))
    {
    return sddp->GetUpdatePiece(this->GetPortNumber());
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdateNumberOfPieces(int n)
{
  if(SDDP* sddp = this->TrySDDP("SetUpdateNumberOfPieces"))
    {
    if(sddp->SetUpdateNumberOfPieces(this->GetPortNumber(), n))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int vtkDataObject::GetUpdateNumberOfPieces()
{
  if(SDDP* sddp = this->TrySDDP("GetUpdateNumberOfPieces"))
    {
    return sddp->GetUpdateNumberOfPieces(this->GetPortNumber());
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdateGhostLevel(int level)
{
  if(SDDP* sddp = this->TrySDDP("SetUpdateGhostLevel"))
    {
    if(sddp->SetUpdateGhostLevel(this->GetPortNumber(), level))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int vtkDataObject::GetUpdateGhostLevel()
{
  if(SDDP* sddp = this->TrySDDP("GetUpdateGhostLevel"))
    {
    return sddp->GetUpdateGhostLevel(this->GetPortNumber());
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetExtentTranslator(vtkExtentTranslator* translator)
{
  if(SDDP* sddp = this->TrySDDP("SetExtentTranslator"))
    {
    if(sddp->SetExtentTranslator(this->GetPortNumber(), translator))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
vtkExtentTranslator* vtkDataObject::GetExtentTranslator()
{
  if(SDDP* sddp = this->TrySDDP("GetExtentTranslator"))
    {
    return sddp->GetExtentTranslator(this->GetPortNumber());
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetReleaseDataFlag(int value)
{
  if(SDDP* sddp = this->TrySDDP("SetReleaseDataFlag"))
    {
    if(sddp->SetReleaseDataFlag(this->GetPortNumber(), value))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int vtkDataObject::GetReleaseDataFlag()
{
  if(SDDP* sddp = this->TrySDDP("GetReleaseDataFlag"))
    {
    return sddp->GetReleaseDataFlag(this->GetPortNumber());
    }
  return 0;
}
