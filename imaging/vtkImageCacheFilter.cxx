/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCacheFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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

#include "vtkImageCacheFilter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageCacheFilter* vtkImageCacheFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageCacheFilter");
  if(ret)
    {
    return (vtkImageCacheFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageCacheFilter;
}




//----------------------------------------------------------------------------
vtkImageCacheFilter::vtkImageCacheFilter()
{
  this->CacheSize = 0;
  this->Data = NULL;
  this->Times = NULL;
  
  this->SetCacheSize(10);
}

//----------------------------------------------------------------------------
vtkImageCacheFilter::~vtkImageCacheFilter()
{
  this->SetCacheSize(0);
}


//----------------------------------------------------------------------------
void vtkImageCacheFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx, *ext;
  vtkIndent i2 = indent.GetNextIndent();
  
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "CacheSize: " << this->CacheSize << endl;
  os << indent << "Caches: \n";
  for (idx = 0; idx < this->CacheSize; ++idx)
    {
    if (this->Data[idx])
      {
      ext = this->Data[idx]->GetExtent();
      os << i2 << idx << ": (" << this->Times[idx] 
	 << ") " << ext[0] << ", " << ext[1] << ", " << ext[2] << ", " 
	 << ext[3] << ", " << ext[4] << ", " << ext[5] << endl;
      }
    }
}

void vtkImageCacheFilter::SetCacheSize(int size)
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
  if (this->Data)
    {
    delete [] this->Data;
    this->Data = NULL;
    }
  if (this->Times)
    {
    delete [] this->Times;
    this->Times = NULL;
    }
  
  this->CacheSize = size;
  if (size == 0)
    {
    return;
    }
  
  this->Data = new vtkImageData* [size];
  this->Times = new unsigned long [size];

  for (idx = 0; idx < size; ++idx)
    {
    this->Data[idx] = NULL;
    this->Times[idx] = 0;
    }
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
void vtkImageCacheFilter::UpdateData(vtkDataObject *outObject)
{
  unsigned long pmt;
  int *uExt, *ext;
  vtkImageData *outData = (vtkImageData *)(outObject);
  vtkImageData *inData = this->GetInput();
  int i;
  int flag = 0;

  if (!inData)
    {
    vtkErrorMacro(<< "Input not set.");
    return;
    }

  uExt = outData->GetUpdateExtent();

  // First look through the cached data to see if it is still valid.
  pmt = inData->GetPipelineMTime();
  for (i = 0; i < this->CacheSize; ++i)
    {
    if (this->Data[i] && this->Times[i] < pmt)
      {
      this->Data[i]->Delete();
      this->Times[i] = 0;
      }
    }

  // Look for data that contains UpdateExtent.
  for (i = 0; i < this->CacheSize; ++i)
    {
    if (this->Data[i])
      {
      ext = this->Data[i]->GetExtent();
      if (uExt[0] >= ext[0] && uExt[1] <= ext[1] &&
	  uExt[2] >= ext[2] && uExt[3] <= ext[3] &&
	  uExt[4] >= ext[4] && uExt[5] <= ext[5])
	{
	vtkDebugMacro("Found Cached Data to meet request" );
	
	// Pass this data to output.
	outData->SetExtent(ext);
	outData->GetPointData()->PassData(this->Data[i]->GetPointData());
	outData->DataHasBeenGenerated();
	flag = 1;
	}
      }
    }


  if (flag == 0)
    {
    unsigned long bestTime = VTK_LARGE_INTEGER;
    int bestIdx = 0;

    // we need to update.
    inData->SetUpdateExtent(uExt);
    inData->PropagateUpdateExtent();
    inData->UpdateData();
    
    if (inData->GetDataReleased())
      { // special case  
      return;
      }

    vtkDebugMacro("Generating Data to meet request" );
    
    outData->SetExtent(inData->GetExtent());
    outData->GetPointData()->PassData(inData->GetPointData());
    outData->DataHasBeenGenerated();
    
    // Save the image in cache.
    // Find a spot to put the data.
    for (i = 0; i < this->CacheSize; ++i)
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
    if (this->Data[bestIdx] == NULL)
      {
      this->Data[bestIdx] = vtkImageData::New();
      }
    this->Data[bestIdx]->ReleaseData();
    this->Data[bestIdx]->SetScalarType(inData->GetScalarType());
    this->Data[bestIdx]->SetExtent(inData->GetExtent());
    this->Data[bestIdx]->SetNumberOfScalarComponents(inData->GetNumberOfScalarComponents());
    this->Data[bestIdx]->GetPointData()->SetScalars(inData->GetPointData()->GetScalars());
    this->Times[bestIdx] = inData->GetUpdateTime();
    
    // release input data
    if (this->GetInput()->ShouldIReleaseData())
      {
      this->GetInput()->ReleaseData();
      }
    }
}



