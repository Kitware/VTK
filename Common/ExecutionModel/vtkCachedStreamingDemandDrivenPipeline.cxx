/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCachedStreamingDemandDrivenPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCachedStreamingDemandDrivenPipeline.h"

#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkObjectFactory.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkCachedStreamingDemandDrivenPipeline);


//----------------------------------------------------------------------------
vtkCachedStreamingDemandDrivenPipeline
::vtkCachedStreamingDemandDrivenPipeline()
{
  this->CacheSize = 0;
  this->Data = NULL;
  this->Times = NULL;

  this->SetCacheSize(10);
}

//----------------------------------------------------------------------------
vtkCachedStreamingDemandDrivenPipeline
::~vtkCachedStreamingDemandDrivenPipeline()
{
  this->SetCacheSize(0);
}

//----------------------------------------------------------------------------
void vtkCachedStreamingDemandDrivenPipeline::SetCacheSize(int size)
{
  int idx;

  if (size == this->CacheSize)
    {
    return;
    }

  this->Modified();

  // free the old data
  for (idx = 0; idx < this->CacheSize; ++idx)
    {
    if (this->Data[idx])
      {
      this->Data[idx]->Delete();
      this->Data[idx] = NULL;
      }
    }
  delete [] this->Data;
  this->Data = NULL;
  delete [] this->Times;
  this->Times = NULL;

  this->CacheSize = size;
  if (size == 0)
    {
    return;
    }

  this->Data = new vtkDataObject* [size];
  this->Times = new unsigned long [size];

  for (idx = 0; idx < size; ++idx)
    {
    this->Data[idx] = NULL;
    this->Times[idx] = 0;
    }
}

//----------------------------------------------------------------------------
void vtkCachedStreamingDemandDrivenPipeline
::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CacheSize: " << this->CacheSize << "\n";
}

//----------------------------------------------------------------------------
int vtkCachedStreamingDemandDrivenPipeline::Update()
{
  return this->Superclass::Update();
}

//----------------------------------------------------------------------------
int vtkCachedStreamingDemandDrivenPipeline::Update(int port)
{
  if(!this->UpdateInformation())
    {
    return 0;
    }
  if(port >= 0 && port < this->Algorithm->GetNumberOfOutputPorts())
    {
    int retval = 1;
    // some streaming filters can request that the pipeline execute multiple
    // times for a single update
    do
      {
      retval =
        this->PropagateUpdateExtent(port) && this->UpdateData(port) && retval;
      }
    while (this->ContinueExecuting);
    return retval;
    }
  else
    {
    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkCachedStreamingDemandDrivenPipeline
::NeedToExecuteData(int outputPort,
                    vtkInformationVector** inInfoVec,
                    vtkInformationVector* outInfoVec)
{
  // If no port is specified, check all ports.  This behavior is
  // implemented by the superclass.
  if(outputPort < 0)
    {
    return this->Superclass::NeedToExecuteData(outputPort,
                                               inInfoVec, outInfoVec);
    }

  // Does the superclass want to execute? We must skip our direct superclass
  // because it looks at update extents but does not know about the cache
  if(this->vtkDemandDrivenPipeline::NeedToExecuteData(outputPort,
                                                      inInfoVec, outInfoVec))
    {
    return 1;
    }

  // Has the algorithm asked to be executed again?
  if(this->ContinueExecuting)
    {
    return 1;
    }

  // First look through the cached data to see if it is still valid.
  int i;
  unsigned long pmt = this->GetPipelineMTime();
  for (i = 0; i < this->CacheSize; ++i)
    {
    if (this->Data[i] && this->Times[i] < pmt)
      {
      this->Data[i]->Delete();
      this->Data[i] = NULL;
      this->Times[i] = 0;
      }
    }

  // We need to check the requested update extent.  Get the output
  // port information and data information.  We do not need to check
  // existence of values because it has already been verified by
  // VerifyOutputInformation.
  vtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
  vtkDataObject* dataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkInformation* dataInfo = dataObject->GetInformation();
  if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_PIECES_EXTENT)
    {
    int updatePiece = outInfo->Get(UPDATE_PIECE_NUMBER());
    int updateNumberOfPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
    int updateGhostLevel = outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());

    // check to see if any data in the cache fits this request
    for (i = 0; i < this->CacheSize; ++i)
      {
      if (this->Data[i])
        {
        dataInfo = this->Data[i]->GetInformation();

        // Check the unstructured extent.  If we do not have the requested
        // piece, we need to execute.
        int dataPiece = dataInfo->Get(vtkDataObject::DATA_PIECE_NUMBER());
        int dataNumberOfPieces =
          dataInfo->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
        int dataGhostLevel =
          dataInfo->Get(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
        if (dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) ==
            VTK_PIECES_EXTENT && dataPiece == updatePiece &&
            dataNumberOfPieces == updateNumberOfPieces &&
            dataGhostLevel == updateGhostLevel)
          {
          // we have a matching data we must copy it to our output, but for
          // now we don't support polydata
          return 1;
          }
        }
      }
    }
  else if (dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
    {
    // Check the structured extent.  If the update extent is outside
    // of the extent and not empty, we need to execute.
    int dataExtent[6];
    int updateExtent[6];
    outInfo->Get(UPDATE_EXTENT(), updateExtent);

    // check to see if any data in the cache fits this request
    for (i = 0; i < this->CacheSize; ++i)
      {
      if (this->Data[i])
        {
        dataInfo = this->Data[i]->GetInformation();
        dataInfo->Get(vtkDataObject::DATA_EXTENT(), dataExtent);
        if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) ==
           VTK_3D_EXTENT &&
           !(updateExtent[0] < dataExtent[0] ||
             updateExtent[1] > dataExtent[1] ||
             updateExtent[2] < dataExtent[2] ||
             updateExtent[3] > dataExtent[3] ||
             updateExtent[4] < dataExtent[4] ||
             updateExtent[5] > dataExtent[5]) &&
           (updateExtent[0] <= updateExtent[1] &&
            updateExtent[2] <= updateExtent[3] &&
            updateExtent[4] <= updateExtent[5]))
          {
          // we have a match
          // Pass this data to output.
          vtkImageData *id = vtkImageData::SafeDownCast(dataObject);
          vtkImageData *id2 = vtkImageData::SafeDownCast(this->Data[i]);
          if (id && id2)
            {
            id->SetExtent(dataExtent);
            id->GetPointData()->PassData(id2->GetPointData());
            // not sure if we need this
            dataObject->DataHasBeenGenerated();
            return 0;
            }
          }
        }
      }
    }

  // We do need to execute
  return 1;
}


//----------------------------------------------------------------------------
int vtkCachedStreamingDemandDrivenPipeline
::ExecuteData(vtkInformation* request,
              vtkInformationVector** inInfoVec,
              vtkInformationVector* outInfoVec)
{
  // only works for one in one out algorithms
  if (request->Get(FROM_OUTPUT_PORT()) != 0)
    {
    vtkErrorMacro("vtkCachedStreamingDemandDrivenPipeline can only be used for algorithms with one output and one input");
    return 0;
    }

  // first do the ususal thing
  int result = this->Superclass::ExecuteData(request, inInfoVec, outInfoVec);

  // then save the newly generated data
  unsigned long bestTime = VTK_INT_MAX;
  int bestIdx = 0;

  // Save the image in cache.
  // Find a spot to put the data.
  for (int i = 0; i < this->CacheSize; ++i)
    {
    if (this->Data[i] == NULL)
      {
      bestIdx = i;
      bestTime = 0;
      break;
      }
    if (this->Times[i] < bestTime)
      {
      bestIdx = i;
      bestTime = this->Times[i];
      }
    }

  vtkInformation* outInfo = outInfoVec->GetInformationObject(0);
  vtkDataObject* dataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (this->Data[bestIdx] == NULL)
    {
    this->Data[bestIdx] = dataObject->NewInstance();
    }
  this->Data[bestIdx]->ReleaseData();

  vtkImageData *id = vtkImageData::SafeDownCast(dataObject);
  if (id)
    {
    vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);
    vtkImageData *input =
      vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    id->SetExtent(input->GetExtent());
    id->GetPointData()->PassData(input->GetPointData());
    id->DataHasBeenGenerated();
    }

  vtkImageData *id2 = vtkImageData::SafeDownCast(this->Data[bestIdx]);
  if (id && id2)
    {
    id2->SetExtent(id->GetExtent());
    id2->GetPointData()->SetScalars(
      id->GetPointData()->GetScalars());
    }

  this->Times[bestIdx] = dataObject->GetUpdateTime();

  return result;
}
