/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataStreamer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkImageDataStreamer.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

// A helper class to keep track of a stack of extents 


class vtkImageDataStreamerExtentStack
{
public:
  vtkImageDataStreamerExtentStack()
  { this->StackTop = NULL; 
    this->StackStorageSize = 0;
    this->StackSize = 0;
    this->Stack = NULL; };

  ~vtkImageDataStreamerExtentStack()
  { if ( this->Stack ) { delete [] this->Stack; }; };

  void Push( int extent[6] );
  void Pop( int extent[6] );

  int  GetStackSize() { return this->StackSize; };

protected:
  int    *Stack;
  int    *StackTop;
  int    StackSize;
  int    StackStorageSize;
};

void vtkImageDataStreamerExtentStack::Push( int extent[6] )
{
  int *newStack, newSize;

  if ( this->StackSize >= this->StackStorageSize )
    {
    newSize = ( this->StackSize > 0 )?(this->StackSize*2):(100);
    newStack = new int[6*newSize];

    if ( this->Stack )
      {
      memcpy( newStack, this->Stack, 6*this->StackSize*sizeof(int) );
      delete [] this->Stack;
      }
    
    this->StackStorageSize = newSize;
    this->Stack = newStack;
    }

  memcpy( this->Stack + 6*this->StackSize, extent, 6*sizeof(int) );
  this->StackSize++;

}

void vtkImageDataStreamerExtentStack::Pop( int extent[6] )
{
  if ( this->StackSize <= 0 )
    {
    vtkGenericWarningMacro( << "The image data streamer stack is empty" );
    return;
    }

  this->StackSize--;
  memcpy( extent, this->Stack + 6*this->StackSize, 6*sizeof(int) );
}

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
  // Set a default memory limit of a gigabyte
  this->MemoryLimit = 1000000; 

  // Set a default split mode to be slabs
  this->SplitMode   = VTK_IMAGE_DATA_STREAMER_Z_SLAB_MODE;
}


//----------------------------------------------------------------------------
void vtkImageDataStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);
  os << indent << "MemoryLimit (in kb): " << this->MemoryLimit << endl;
  os << indent << "SplitMode: ";
  if (this->SplitMode == VTK_IMAGE_DATA_STREAMER_BLOCK_MODE)
    {
    os << "Block\n";
    }
  else if (this->SplitMode == VTK_IMAGE_DATA_STREAMER_X_SLAB_MODE)
    {
    os << "X Slab\n";
    }
  else if (this->SplitMode == VTK_IMAGE_DATA_STREAMER_Y_SLAB_MODE)
    {
    os << "Y Slab\n";
    }
  else if (this->SplitMode == VTK_IMAGE_DATA_STREAMER_Z_SLAB_MODE)
    {
    os << "Z Slab\n";
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
  unsigned long  inputMemorySize, newSize;
  double         splitSize[3];
  vtkImageData   *input = this->GetInput();
  vtkImageData   *output = (vtkImageData*)out;
  int            outExt[6], currExt[6], newExt[6];
  int            i, best;
  int            unreachableLimit;

  vtkImageDataStreamerExtentStack extentStack;

  // prevent chasing our tail
  if (this->Updating)
    {
    return;
    }

  // Initialize the output
  output->Initialize();

  // If there is a start method, call it
  if ( this->StartMethod )
    {
    (*this->StartMethod)(this->StartMethodArg);
    }

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
  // We do need to break it up - keep breaking up the extent according
  // to the split method until the memory limit is achieved. Attempt to
  // detect the case where the memory limit cannot be achieved and avoid
  // unproductive splitting
  else 
    {
    output->GetUpdateExtent(outExt);
    output->SetExtent(outExt);
    output->AllocateScalars();

    // Push this extent on our stack to get started
    extentStack.Push( outExt );

    // Keep processing until our stack is empty
    while ( extentStack.GetStackSize() > 0 )
      {
      
      // Get the next extent
      extentStack.Pop( currExt );

      // How much memory does this require?
      input->SetUpdateExtent( currExt );
      input->PropagateUpdateExtent();
      newSize = input->GetEstimatedPipelineMemorySize();

      unreachableLimit = 0;

      // Are we over the limit? If so, we need to figure out which way to 
      // split it and push the two halves on the stack. If we can't seem
      // to get closer to the limit, stop splitting and process this extent
      // even though it doesn't meet the limit.
      if ( newSize > this->MemoryLimit )
	{
	// Compute the new sizes for the first half of each split
	for ( i = 0; i < 3; i++ )
	  {
	  splitSize[i] = -1;

	  if ( currExt[i*2+1] - currExt[i*2] > 0 &&
	     ( this->SplitMode == VTK_IMAGE_DATA_STREAMER_BLOCK_MODE ||
	       this->SplitMode == i ) )
	    {
	    memcpy( newExt, currExt, 6*sizeof(int) );
	    newExt[i*2+1] = (currExt[i*2] + currExt[i*2+1]) / 2;
	  
	    // How much memory does this require?
	    input->SetUpdateExtent( newExt );
	    input->PropagateUpdateExtent();
	    splitSize[i] = input->GetEstimatedPipelineMemorySize();

	    // Are we getting closer to the limit by a reasonable amount?
	    splitSize[i] = (splitSize[i] > newSize*0.9)?(-1):(splitSize[i]);
	    }
	  }
	
	// Pick the best split and push it on the stack	
	if ( splitSize[0] != -1 &&
	     ( splitSize[0] <= splitSize[1] || splitSize[1] == -1 ) &&
	     ( splitSize[0] <= splitSize[2] || splitSize[2] == -1 ) )
	  {
	  // The X split is best
	  best = 0;
	  }
	else if ( splitSize[1] != -1 &&
	     ( splitSize[1] <= splitSize[0] || splitSize[0] == -1 ) &&
	     ( splitSize[1] <= splitSize[2] || splitSize[2] == -1 ) )
	  {
	  // The Y split is best
	  best = 1;
	  }
	else if ( splitSize[2] != -1 &&
	     ( splitSize[2] <= splitSize[0] || splitSize[0] == -1 ) &&
	     ( splitSize[2] <= splitSize[1] || splitSize[1] == -1 ) )
	  {
	  // The Z split is best
	  best = 2;
	  }
	else
	  {
	  // No split will work - our limit is probably unreachable
	  unreachableLimit = 1;
	  
	  // We need to propagate this extent again since we will
	  // rely on the fact that it has been propagated later on
	  input->SetUpdateExtent( currExt );
	  input->PropagateUpdateExtent();

	  // There is no best split axis
	  best = -1;
	  }

	// If our limit seems reachable, do the split and push the
	// two halves on the stack
	if ( best >= 0 )
	  {
	  // Do a quick check - if we are in block mode, and two or
	  // three of the split sizes are the same, pick the one
	  // with the bigger extent (otherwise X will always split)
	  if ( this->SplitMode ==  VTK_IMAGE_DATA_STREAMER_BLOCK_MODE )
	    {
	    // Check one of the other axes
	    i = (best+1)%3;
	    if ( splitSize[best] == splitSize[i] &&
		 currExt[i*2+1] - currExt[i*2] > 
		 currExt[best*2+1] - currExt[best*2] )
	      {
	      best = i;
	      }

	    // Check the final axis
	    i = (i+1)%3;
	    if ( splitSize[best] == splitSize[i] &&
		 currExt[i*2+1] - currExt[i*2] > 
		 currExt[best*2+1] - currExt[best*2] )
	      {
	      best = i;
	      }
	    }

	  // Here's the first half
	  memcpy( newExt, currExt, 6*sizeof(int) );
	  newExt[best*2+1] = (currExt[best*2] + currExt[best*2+1]) / 2;	  
	  extentStack.Push( newExt );

	  // Here's the second half
	  newExt[best*2] = newExt[best*2+1] + 1;
	  newExt[best*2+1] = currExt[best*2+1];
	  extentStack.Push( newExt );
	  }

	}      
      
      // Is it under our limit or can we not achieve the limit? 
      // If so, do the update
      if ( newSize <= this->MemoryLimit || unreachableLimit )
	{
	input->TriggerAsynchronousUpdate();
	input->UpdateData();
	output->CopyAndCastFrom(input, currExt);
	}
      }
    }

  output->DataHasBeenGenerated();

  // Call the end method, if there is one
  if ( this->EndMethod )
    {
    (*this->EndMethod)(this->EndMethodArg);
    }

  // Information gets invalidated as soon as Update is called,
  // so validate it again here.
  this->InformationTime.Modified();  
}





