/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMemoryLimitImageDataStreamer.cxx
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

#include "vtkMemoryLimitImageDataStreamer.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkPipelineSize.h"

//----------------------------------------------------------------------------
vtkMemoryLimitImageDataStreamer* vtkMemoryLimitImageDataStreamer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMemoryLimitImageDataStreamer");
  if(ret)
    {
    return (vtkMemoryLimitImageDataStreamer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMemoryLimitImageDataStreamer;
}


//----------------------------------------------------------------------------
vtkMemoryLimitImageDataStreamer::vtkMemoryLimitImageDataStreamer()
{
  // Set a default memory limit of 50 Megabytes
  this->MemoryLimit = 50000; 
}


//----------------------------------------------------------------------------
void vtkMemoryLimitImageDataStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageDataStreamer::PrintSelf(os,indent);

  os << indent << "MemoryLimit (in kb): " << this->MemoryLimit << endl;
}


//----------------------------------------------------------------------------
void vtkMemoryLimitImageDataStreamer::UpdateData(vtkDataObject *out)
{
  // find the right number of pieces
  if (!this->GetInput())
    {
    return;
    }
  
  vtkImageData *input = this->GetInput();
  vtkExtentTranslator *translator = this->GetExtentTranslator();
  translator->SetWholeExtent(out->GetUpdateExtent());

  vtkPipelineSize *sizer = vtkPipelineSize::New();
  this->NumberOfStreamDivisions = 1;
  unsigned long oldSize, size = 0;
  float ratio;
  translator->SetPiece(0);

  // watch for the limiting case where the size is the maximum size
  // represented by an unsigned long. In that case we do not want to do the
  // ratio test. We actual test for size < 0.5 of the max unsigned long which
  // would indicate that oldSize is about at max unsigned long.
  unsigned long maxSize;
  maxSize = (((unsigned long)0x1) << (8*sizeof(unsigned long) - 1));
  
  // we also have to watch how many pieces we are creating. Since
  // NumberOfStreamDivisions is an int, it cannot be more that say 2^31
  // (which is a bit much anyhow) so we also stop if the number of pieces is
  // too large.
  int count = 0;
  
  // double the number of pieces until the size fits in memory
  // or the reduction in size falls to 20%
  do 
    {
    oldSize = size;
    translator->SetNumberOfPieces(this->NumberOfStreamDivisions);
    translator->PieceToExtentByPoints();
    input->SetUpdateExtent(translator->GetExtent());
    input->PropagateUpdateExtent();
    size = sizer->GetEstimatedSize(this->GetInput());
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
    this->NumberOfStreamDivisions = this->NumberOfStreamDivisions*2;
    count++;
    }
  while (size > this->MemoryLimit && 
         (size < maxSize && ratio < 0.8) && count < 29);
  
  // undo the last *2
  this->NumberOfStreamDivisions = this->NumberOfStreamDivisions/2;
  
  // now call the superclass
  this->vtkImageDataStreamer::UpdateData(out);

  sizer->Delete();
}
