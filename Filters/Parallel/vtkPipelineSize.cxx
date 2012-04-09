/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPipelineSize.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPipelineSize.h"

#include "vtkAlgorithmOutput.h"
#include "vtkConeSource.h"
#include "vtkDataObject.h"
#include "vtkDataReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLargeInteger.h"
#include "vtkObjectFactory.h"
#include "vtkPSphereSource.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkDataSetAttributes.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPipelineSize);

unsigned long
vtkPipelineSize::GetEstimatedSize(vtkAlgorithm *input, int inputPort,
                                  int connection)
{
  unsigned long sizes[3];
  unsigned long memorySize = 0;


  if(vtkAlgorithmOutput* inInfo =
     input->GetInputConnection(inputPort, connection))
    {
    if (vtkAlgorithm* srcAlg =
        vtkAlgorithm::SafeDownCast(
          inInfo->GetProducer()))
      {
      this->ComputeSourcePipelineSize(srcAlg, inInfo->GetIndex(), sizes );
      memorySize = sizes[2];
      }
    }

  return memorySize;
}

// The first size is the memory going downstream from here - which is all
// the memory coming in minus any data realeased. The second size is the
// size of the specified output (which can be used by the downstream
// filter when determining how much data it might release). The final size
// is the maximum pipeline size encountered here and upstream from here.
void vtkPipelineSize::ComputeSourcePipelineSize(vtkAlgorithm *src,
                                                int outputPort,
                                                unsigned long size[3])
{
  // watch for special sources
  // handle vtkDataReader subclasses
  if (src->IsA("vtkDataReader"))
    {
    ifstream *ifs;
    vtkDataReader *rdr = vtkDataReader::SafeDownCast(src);
#ifdef _WIN32
    ifs = new ifstream(rdr->GetFileName(), ios::in | ios::binary);
#else
    ifs = new ifstream(rdr->GetFileName(), ios::in);
#endif
    if (!ifs->fail())
      {
      ifs->seekg(0,ios::end);
      int sz = ifs->tellg()/1024;
      size[0] = sz;
      size[1] = sz;
      size[2] = sz;
      return;
      }
    delete ifs;
    }

  // handle some simple sources
  vtkLargeInteger sz;
  if (src->IsA("vtkConeSource"))
    {
    vtkConeSource *s = vtkConeSource::SafeDownCast(src);
    sz = s->GetResolution();
    sz = sz * 32/1024;
    size[0] = sz.CastToUnsignedLong();
    size[1] = size[0];
    size[2] = size[0];
    return;
    }
  if (src->IsA("vtkPlaneSource"))
    {
    vtkPlaneSource *s = vtkPlaneSource::SafeDownCast(src);
    sz = s->GetXResolution();
    sz = sz * s->GetYResolution()*32/1024;
    size[0] = sz.CastToUnsignedLong();
    size[1] = size[0];
    size[2] = size[0];
    return;
    }
  if (src->IsA("vtkPSphereSource"))
    {
    vtkPSphereSource *s = vtkPSphereSource::SafeDownCast(src);
    size[0] = s->GetEstimatedMemorySize();
    size[1] = size[0];
    size[2] = size[0];
    return;
    }

  // otherwise use generic approach
  this->GenericComputeSourcePipelineSize(src,outputPort,size);
}

void vtkPipelineSize::GenericComputeSourcePipelineSize(vtkAlgorithm *src,
                                                       int outputPort,
                                                       unsigned long size[3])
{
  unsigned long outputSize[2];
  unsigned long inputPipelineSize[3];
  vtkLargeInteger mySize = 0;
  unsigned long maxSize = 0;
  vtkLargeInteger goingDownstreamSize = 0;
  unsigned long *inputSize = NULL;
  int idx;

  // We need some space to store the input sizes if there are any inputs
  int numberOfInputs = src->GetTotalNumberOfInputConnections();
  if ( numberOfInputs > 0 )
    {
    inputSize = new unsigned long[numberOfInputs];
    }

  // Get the pipeline size propagated down each input. Keep track of max
  // pipeline size, how much memory will be required downstream from here,
  // the size of each input, and the memory required by this filter when
  // it executes.
  int port = 0;
  int conn = 0;
  for (idx = 0; idx < numberOfInputs; ++idx)
    {
    src->ConvertTotalInputToPortConnection(idx,port,conn);
    inputSize[idx] = 0;
    if(vtkAlgorithmOutput* inInfo = src->GetInputConnection(port, conn))
      {
      if (vtkAlgorithm* srcAlg =
          vtkAlgorithm::SafeDownCast(inInfo->GetProducer()))
        {
        // Get the upstream size of the pipeline, the estimated size of this
        // input, and the maximum size seen upstream from here.
        this->ComputeSourcePipelineSize(srcAlg, inInfo->GetIndex(),
                                        inputPipelineSize);

        // Save this input size to possibly be used when estimating output size
        inputSize[idx] = inputPipelineSize[1];

        // Is the max returned bigger than the max we've seen so far?
        if ( inputPipelineSize[2] > maxSize )
          {
          maxSize = inputPipelineSize[2];
          }

        // If we are going to release this input, then its size won't matter
        // downstream from here.
        vtkDemandDrivenPipeline *ddp =
          vtkDemandDrivenPipeline::SafeDownCast(srcAlg->GetExecutive());
        if (ddp &&
            ddp->GetOutputInformation(inInfo->GetIndex())
            ->Get(vtkDemandDrivenPipeline::RELEASE_DATA()))
          {
          goingDownstreamSize = goingDownstreamSize + inputPipelineSize[0] -
            inputPipelineSize[1];
          }
        else
          {
          goingDownstreamSize = goingDownstreamSize + inputPipelineSize[0];
          }

        // During execution this filter will need all the input data
        mySize += inputPipelineSize[0];
        }
      }
    }

  // Now the we know the size of all input, compute the output size
  this->ComputeOutputMemorySize(src, outputPort, inputSize, outputSize );

  // This filter will produce all output so it needs all that memory.
  // Also, all this data will flow downstream to the next source (if it is
  // the requested output) or will still exist with no chance of being
  // released (if it is the non-requested output)
  mySize += outputSize[1];
  goingDownstreamSize += outputSize[1];

  // Is the state of the pipeline during this filter's execution the
  // largest that it has been so far?
  if ( mySize.CastToUnsignedLong() > maxSize )
    {
    maxSize = mySize.CastToUnsignedLong();
    }

  // The first size is the memory going downstream from here - which is all
  // the memory coming in minus any data realeased. The second size is the
  // size of the specified output (which can be used by the downstream
  // filter when determining how much data it might release). The final size
  // is the maximum pipeline size encountered here and upstream from here.
  size[0] = goingDownstreamSize.CastToUnsignedLong();
  size[1] = outputSize[0];
  size[2] = maxSize;

  // Delete the space we may have created
  if ( inputSize )
    {
    delete [] inputSize;
    }
}

void vtkPipelineSize::
ComputeOutputMemorySize( vtkAlgorithm *src, int outputPort,
                         unsigned long *inputSize, unsigned long size[2] )
{
  vtkLargeInteger sz;

  // watch for special filters such as Glyph3D
  if (src->IsA("vtkGlyph3D"))
    {
    // the output size is the same as the source size * the number of points
    // we guess the number of points to be 1/16 of the input size in bytes
    if (src->GetTotalNumberOfInputConnections() >= 2)
      {
      sz = inputSize[1];
      sz = sz * inputSize[0]*1024/16;
      size[0] = sz.CastToUnsignedLong();
      size[1] = size[0];
      return;
      }
    }

  this->GenericComputeOutputMemorySize(src, outputPort, inputSize, size);
}



void vtkPipelineSize::
GenericComputeOutputMemorySize( vtkAlgorithm *src, int outputPort,
                                unsigned long * vtkNotUsed( inputSize ),
                                unsigned long size[2] )
{
  int idx;
  vtkLargeInteger tmp = 0;
  vtkLargeInteger sz = 0;

  vtkDemandDrivenPipeline *ddp =
    vtkDemandDrivenPipeline::SafeDownCast(src->GetExecutive());

  size[0] = 0;
  size[1] = 0;

  // loop through all the outputs asking them how big they are given the
  // information that they have on their update extent. Keep track of
  // the size of the specified output in size[0], and the sum of all
  // output size in size[1]. Ignore input sizes in this default implementation.
  for (idx = 0; idx < src->GetNumberOfOutputPorts(); ++idx)
    {
    vtkInformation *outInfo = ddp->GetOutputInformation(idx);
    if (outInfo)
      {
      tmp = 0;
      vtkInformation *dataInfo =
        outInfo->Get(vtkDataObject::DATA_OBJECT())->GetInformation();
      if (dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) ==
          VTK_PIECES_EXTENT)
        {
        // TODO: need something here
        tmp = 1;
        }
      if (dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
        {
        int uExt[6];
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),uExt);
        tmp = 4;

        vtkInformation *scalarInfo = vtkDataObject::GetActiveFieldInformation(outInfo,
          vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
        int numComp = 1;
        if (scalarInfo)
          {
          tmp = vtkDataArray::GetDataTypeSize(
            scalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE()));
          if (scalarInfo->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
            {
            numComp = scalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
            }
          }
        tmp *= numComp;
        for (idx = 0; idx < 3; ++idx)
          {
          tmp = tmp*(uExt[idx*2+1] - uExt[idx*2] + 1);
          }
        tmp /= 1024;
        }
      if (idx == outputPort )
        {
        size[0] = tmp.CastToUnsignedLong();
        }
      }
    sz += tmp;
    }

  size[1] = sz.CastToUnsignedLong();
}


unsigned long vtkPipelineSize::GetNumberOfSubPieces(unsigned long memoryLimit,
                                                    vtkPolyDataMapper *mapper)
{
  // find the right number of pieces
  if (!mapper->GetInput())
    {
    return 1;
    }

  unsigned long subDivisions = 1;
  unsigned long numPieces = mapper->GetNumberOfPieces();
  unsigned long piece = mapper->GetPiece();
  unsigned long oldSize, size = 0;
  float ratio;

  // watch for the limiting case where the size is the maximum size
  // represented by an unsigned long. In that case we do not want to do the
  // ratio test. We actual test for size < 0.5 of the max unsigned long which
  // would indicate that oldSize is about at max unsigned long.
  unsigned long maxSize;
  maxSize = (((unsigned long)0x1) << (8*sizeof(unsigned long) - 1));

  // we also have to watch how many pieces we are creating. Since
  // NumberOfStreamDivisions is an int, it cannot be more that say 2^31
  // (which is a bit much anyhow) so we also stop if the number of pieces is
  // too large. Here we start off with the current number of pieces.
  int count = (int) (log(static_cast<float>(numPieces))/log(static_cast<float>(2)));

  // double the number of pieces until the size fits in memory
  // or the reduction in size falls to 20%
  do
    {
    oldSize = size;
    vtkStreamingDemandDrivenPipeline::SetUpdateExtent(
      mapper->GetInputInformation(), piece*subDivisions, numPieces*subDivisions, 0);
    mapper->GetInputAlgorithm()->PropagateUpdateExtent();
    size = this->GetEstimatedSize(mapper,0,0);
    // watch for the first time through
    if (!oldSize)
      {
      ratio = 0.5;
      }
    // otherwise the normal ratio calculation
    else
      {
      ratio = size/(float)oldSize;
      }
    subDivisions = subDivisions*2;
    count++;
    }
  while (size > memoryLimit &&
         (size > maxSize || ratio < 0.8) && count < 29);

  // undo the last *2
  subDivisions = subDivisions/2;

  return subDivisions;
}

void vtkPipelineSize::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
