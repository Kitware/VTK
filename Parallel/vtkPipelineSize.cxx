/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPipelineSize.cxx
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
#include "vtkPipelineSize.h"
#include "vtkDataObject.h"
#include "vtkSource.h"
#include "vtkObjectFactory.h"
#include "vtkDataReader.h"
#include "vtkConeSource.h"
#include "vtkPlaneSource.h"
#include "vtkLargeInteger.h"
#include "vtkPSphereSource.h"
#include "vtkPolyDataMapper.h"

//-------------------------------------------------------------------------
vtkPipelineSize* vtkPipelineSize::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPipelineSize");
  if(ret)
    {
    return (vtkPipelineSize*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPipelineSize;
}

unsigned long 
vtkPipelineSize::GetEstimatedSize(vtkDataObject *input)
{
  unsigned long sizes[3];
  unsigned long memorySize = 0;
  
  if (input->GetSource())
    {
    input->UpdateInformation();
    this->ComputeSourcePipelineSize(input->GetSource(),input, sizes );
    memorySize = sizes[2];
    } 
  
  return memorySize;
}

// The first size is the memory going downstream from here - which is all
// the memory coming in minus any data realeased. The second size is the
// size of the specified output (which can be used by the downstream 
// filter when determining how much data it might release). The final size
// is the maximum pipeline size encountered here and upstream from here.
void vtkPipelineSize::ComputeSourcePipelineSize(vtkSource *src, 
                                                vtkDataObject *output,
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
  this->GenericComputeSourcePipelineSize(src,output,size);
}

void vtkPipelineSize::GenericComputeSourcePipelineSize(vtkSource *src, 
                                                       vtkDataObject *output,
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
  int numberOfInputs = src->GetNumberOfInputs();
  if ( numberOfInputs > 0 )
    {
    inputSize = new unsigned long[numberOfInputs];
    }

  // Get the pipeline size propagated down each input. Keep track of max
  // pipeline size, how much memory will be required downstream from here,
  // the size of each input, and the memory required by this filter when
  // it executes.
  vtkDataObject **inputs = src->GetInputs();
  for (idx = 0; idx < numberOfInputs; ++idx)
    {
    if (inputs[idx])
      {
      // Get the upstream size of the pipeline, the estimated size of this
      // input, and the maximum size seen upstream from here.
      this->ComputeDataPipelineSize(inputs[idx], inputPipelineSize);

      // Save this input size to possibly be used when estimating output size
      inputSize[idx] = inputPipelineSize[1];

      // Is the max returned bigger than the max we've seen so far?
      if ( inputPipelineSize[2] > maxSize )
	{
	maxSize = inputPipelineSize[2];
	}
      
      // If we are going to release this input, then its size won't matter
      // downstream from here.
      if ( inputs[idx]->ShouldIReleaseData() )
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
    
    // The input was null, so it has no size
    else
      {
      inputSize[idx] = 0;
      }
    }
  
  // Now the we know the size of all input, compute the output size
  this->ComputeOutputMemorySize(src, output, inputSize, outputSize );

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
ComputeOutputMemorySize( vtkSource *src, vtkDataObject *output,
                         unsigned long *inputSize, unsigned long size[2] )
{
  vtkLargeInteger sz;
  
  // watch for special filters such as Glyph3D
  if (src->IsA("vtkGlyph3D"))
    {
    // the output size is the same as the source size * the number of points
    // we guess the number of points to be 1/16 of the input size in bytes
    if (src->GetNumberOfInputs() >= 2)
      {
      sz = inputSize[1];
      sz = sz * inputSize[0]*1024/16;
      size[0] = sz.CastToUnsignedLong();
      size[1] = size[0];
      return;
      }
    }
  
  this->GenericComputeOutputMemorySize(src, output, inputSize, size);
}



void vtkPipelineSize::
GenericComputeOutputMemorySize( vtkSource *src, vtkDataObject *output,
                                unsigned long *inputSize,
                                unsigned long size[2] )
{
  int idx;
  unsigned long tmp = 0;
  vtkLargeInteger sz = 0;
  
  
  size[0] = 0;
  size[1] = 0;

  // loop through all the outputs asking them how big they are given the
  // information that they have on their update extent. Keep track of 
  // the size of the specified output in size[0], and the sum of all
  // output size in size[1]. Ignore input sizes in this default implementation.
  vtkDataObject **outputs = src->GetOutputs();
  for (idx = 0; idx < src->GetNumberOfOutputs(); ++idx)
    {
    if (outputs[idx])
      {
      tmp = 0;
      if (outputs[idx]->IsA("vtkImageData"))
        {
        tmp = outputs[idx]->GetEstimatedMemorySize();
        }
      else
        {
        if (src->GetNumberOfInputs())
          {
          tmp = inputSize[0];
          }
        }
      if ( outputs[idx] == output )
        {
        size[0] = tmp;
        }
      }
    sz += tmp;
    }
  
  size[1] = sz.CastToUnsignedLong();
}

void vtkPipelineSize::
ComputeDataPipelineSize(vtkDataObject *input, unsigned long sizes[3])
{
  if (input->GetSource())
    {
    this->ComputeSourcePipelineSize(input->GetSource(),input, sizes );
    } 
  else
    {
    unsigned long size = input->GetActualMemorySize();
    sizes[0] = size;
    sizes[1] = size;
    sizes[2] = size;
    }
}


unsigned long vtkPipelineSize::GetNumberOfSubPieces(unsigned long memoryLimit, 
                                                    vtkPolyDataMapper *mapper)
{
  // find the right number of pieces
  if (!mapper->GetInput())
    {
    return 1;
    }
  
  vtkPolyData *input = mapper->GetInput();
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
    input->SetUpdateExtent(piece*subDivisions, numPieces*subDivisions);
    input->PropagateUpdateExtent();
    size = this->GetEstimatedSize(input);
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

