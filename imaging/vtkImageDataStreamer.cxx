/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataStreamer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkImageDataStreamer.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkImageDataStreamer* vtkImageDataStreamer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageDataStreamer");
  if(ret)
    {
    return (vtkImageDataStreamer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageDataStreamer;
}


//----------------------------------------------------------------------------
vtkImageDataStreamer::vtkImageDataStreamer()
{
  this->MemoryLimit = 1000; 
  this->SplitMode   = VTK_IMAGE_DATA_STREAMER_SLAB_MODE;
}


//----------------------------------------------------------------------------
void vtkImageDataStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);
  os << indent << "MemoryLimit (in kb): " << this->MemoryLimit << endl;
  os << indent << "SplitMode: ";
  if (this->SplitMode == VTK_IMAGE_DATA_STREAMER_BLOCK_MODE)
    {
    os << "Block\n";
    }
  else if (this->SplitMode == VTK_IMAGE_DATA_STREAMER_SLAB_MODE)
    {
    os << "Block\n";
    }
  else
    {
    os << "Unknown\n";
    }
}

//----------------------------------------------------------------------------

// We don't want to propagate this trigger request since we won't actually
// know our input update extents until UpdateData()
void vtkImageDataStreamer::TriggerAsynchronousUpdate()
{
}  

//----------------------------------------------------------------------------
void vtkImageDataStreamer::UpdateData(vtkDataObject *out)
{
  int            idx;
  unsigned long  inputMemorySize, newSize;
  vtkImageData   *input = this->GetInput();
  vtkImageData   *output = (vtkImageData*)out;
  int            outExt[6], firstExt[6], currentExt[6];
  int            blockSize[3], numBlocks[3], splitAxis;
  int            i, j, k, tmp;

  // prevent chasing our tail
  if (this->Updating)
    {
    return;
    }

  // Initialize the output
  output->Initialize();

  // Try to behave gracefully with no input.
  if (input == NULL)
    {
    output->SetExtent(output->GetUpdateExtent());
    output->AllocateScalars();
    output->DataHasBeenGenerated();
    return;
    }
  
  // If we don't stream, how much memory would we use?
  inputMemorySize = input->GetEstimatedPipelineMemorySize();

  // Fast path for when streaming is not necessary
  if ( inputMemorySize <= this->MemoryLimit )
    {
    input->SetUpdateExtent(output->GetUpdateExtent());
    this->Updating = 1;
    input->TriggerAsynchronousUpdate();
    input->UpdateData();
    this->Updating = 0;
    output->SetExtent(input->GetExtent());
    output->GetPointData()->PassData(input->GetPointData());
    }
  // We do need to break it up - use one of two methods (slab or block)
  // For now, just assume all slabs / blocks will require the same max
  // pipeline memory. So we determine only one slab / block size
  else 
    {
    output->GetUpdateExtent(outExt);
    output->SetExtent(outExt);
    output->AllocateScalars();

    // This is the extent we will keep splitting
    memcpy( firstExt, outExt, 6*sizeof(int) );

    blockSize[0] = firstExt[1] - firstExt[0] + 1;
    blockSize[1] = firstExt[3] - firstExt[2] + 1;
    blockSize[2] = firstExt[5] - firstExt[4] + 1;

    // Keep splitting until we meet the memory limit or can't split any
    // further (slab size is 1 or all block sides are 1). Each split is in 
    // half. If a split results in less than a 10% decrease in memory size,
    // then consider it not worth it and stop splitting even though we
    // have not reached the limit yet. 

    while ( inputMemorySize > this->MemoryLimit &&
	    ( this->SplitMode == VTK_IMAGE_DATA_STREAMER_BLOCK_MODE &&
	      ( blockSize[0] > 1 || 
		blockSize[1] > 1 || 
		blockSize[2] > 1 ) ) ||
	    ( this->SplitMode == VTK_IMAGE_DATA_STREAMER_SLAB_MODE &&
	      blockSize[2] > 1 ) )
      {
      if ( this->SplitMode == VTK_IMAGE_DATA_STREAMER_SLAB_MODE )
	{
	splitAxis = 2;
	}
      else if ( blockSize[0] >= blockSize[1] && 
		blockSize[0] >= blockSize[2] )
	{
	splitAxis = 0;
	}
      else if ( blockSize[1] >= blockSize[0] && 
		blockSize[1] >= blockSize[2] )
	{
	splitAxis = 1;
	}
      else
	{
	splitAxis = 2;
	}

      tmp = firstExt[splitAxis*2 + 1];
      firstExt[splitAxis*2 + 1] = firstExt[splitAxis*2] + 
	(firstExt[splitAxis*2 + 1] - firstExt[splitAxis*2]) / 2;
      
      input->SetUpdateExtent( firstExt );
      input->PropagateUpdateExtent();
      newSize = input->GetEstimatedPipelineMemorySize();
      if ( ((float)inputMemorySize * 0.9) < newSize )
	{
	firstExt[splitAxis*2 + 1] = tmp;
	break;
	}
      inputMemorySize = newSize;
      blockSize[splitAxis] = 
	firstExt[splitAxis*2 + 1] - firstExt[splitAxis*2] + 1;
      }

    memcpy( currentExt, outExt, 6*sizeof(int) );

    // Now we know the size of a block, just do all the blocks
    // numBlocks is the number of whole blocks we need to do along
    // each axis
    numBlocks[0] = (outExt[1] - outExt[0] + 1) / blockSize[0];
    numBlocks[1] = (outExt[3] - outExt[2] + 1) / blockSize[1];
    numBlocks[2] = (outExt[5] - outExt[4] + 1) / blockSize[2];
    
    // increment by one if there is a partial block left in a direction
    numBlocks[0] += ( (outExt[1] - outExt[0] + 1) % blockSize[0] != 0 );
    numBlocks[1] += ( (outExt[3] - outExt[2] + 1) % blockSize[1] != 0 );
    numBlocks[2] += ( (outExt[5] - outExt[4] + 1) % blockSize[2] != 0 );

    vtkDebugMacro( << "Our output extent is " << outExt[0] << " " <<
      outExt[1] << " " << outExt[2] << " " << outExt[3] << " " <<
      outExt[4] << " " << outExt[5] );

    vtkDebugMacro( << "Our block size is " << blockSize[0] << " " <<
      blockSize[1] << " " << blockSize[2] );

    vtkDebugMacro( << "We will update data " << numBlocks[0] << " by " <<
      numBlocks[1] << " by " << numBlocks[2] << " times" );

    for ( k = 0; k < numBlocks[2]; k++ )
      {
      currentExt[4] = outExt[4] + k * blockSize[2];
      currentExt[5] = currentExt[4] + blockSize[2] - 1;
      // fix if this is a last partial slab
      if ( currentExt[5] > outExt[5] )
	{
	currentExt[5] = outExt[5];
	}
      
      for ( j = 0; j < numBlocks[1]; j++ )
	{
	currentExt[2] = outExt[2] + j * blockSize[1];
	currentExt[3] = currentExt[2] + blockSize[1] - 1;
	// fix if this is a last partial slab
	if ( currentExt[3] > outExt[3] )
	  {
	  currentExt[3] = outExt[3];
	  }

	for ( i = 0; i < numBlocks[0]; i++ )
	  {
	  currentExt[0] = outExt[0] + i * blockSize[0];
	  currentExt[1] = currentExt[0] + blockSize[0] - 1;
	  // fix if this is a last partial slab
	  if ( currentExt[1] > outExt[1] )
	    {
	    currentExt[1] = outExt[1];
	    }	  
	  
	  input->SetUpdateExtent( currentExt );
	  input->PropagateUpdateExtent();
	  input->TriggerAsynchronousUpdate();
	  input->UpdateData();
	  output->CopyAndCastFrom(input, currentExt);
	  }
	}
      }
    }

  output->DataHasBeenGenerated();

  // If there is a start method, call it
  if ( this->StartMethod )
    {
    (*this->StartMethod)(this->StartMethodArg);
    }

  // Call the end method, if there is one
  if ( this->EndMethod )
    {
    (*this->EndMethod)(this->EndMethodArg);
    }

  // Information gets invalidated as soon as Update is called,
  // so validate it again here.
  this->InformationTime.Modified();  
}





