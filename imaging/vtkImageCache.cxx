/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCache.cxx
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

#include "vtkImageCache.h"
#include "vtkExtent.h"

//----------------------------------------------------------------------------
vtkImageCache::vtkImageCache()
{
  int idx;

  for (idx = 0; idx < VTK_CACHE_NUMBER; ++idx)
    {
    this->Data[idx] = NULL;
    this->Times[idx] = 0;
    }
}

//----------------------------------------------------------------------------
vtkImageCache::~vtkImageCache()
{
  int idx;

  for (idx = 0; idx < VTK_CACHE_NUMBER; ++idx)
    {
    if (this->Data[idx])
      {
      this->Data[idx]->Delete();
      }
    }
}


//----------------------------------------------------------------------------
void vtkImageCache::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  vtkIndent *i2 = i.GetNextIndent();
  
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "Caches: \n";
  for (idx = 0; idx < VTK_CACHE_NUMBER; ++idx)
    {
    }
}
  
//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
void vtkImageCache::InternalUpdate(vtkDataObject *outObject)
{
  unsigned long pmt;
  int *uExt, *ext;
  vtkImageData *outData = (vtkImageData *)(outObject);
  vtkImageData *inData = this->GetInput();
  int i;
  int flag = 0;

  cerr << "UpdateCache: \n";
  
  uExt = outData->GetUpdateExtent();

  // First look through the cached data to see if it is still valid.
  pmt = inData->GetPipelineMTime();
  for (i = 0; i < VTK_CACHE_NUMBER; ++i)
    {
    if (this->Data[i] && this->Times[i] < pmt)
      {
      this->Data[i]->Delete();
      this->Times[i] = 0;
      }
    }

  // Look for data that contains UpdateExtent.
  for (i = 0; i < VTK_CACHE_NUMBER; ++i)
    {
    if (this->Data[i])
      {
      ext = this->Data[i]->GetExtent();
      if (uExt[0] >= ext[0] && uExt[1] <= ext[1] &&
	  uExt[2] >= ext[2] && uExt[3] <= ext[3] &&
	  uExt[4] >= ext[4] && uExt[5] <= ext[5])
	{
	cerr << "Found Cached Data to meet request" << *(outData->GetGenericUpdateExtent()) << endl;
	// Pass this data to output.
	outData->SetExtent(ext);
	outData->GetPointData()->PassData(this->Data[i]->GetPointData());
	flag = 1;
	}
      }
    }


  if (flag == 0)
    {
    unsigned long bestTime = VTK_LARGE_INTEGER;
    int bestIdx;

    // we need to update.
    inData->SetUpdateExtent(uExt);
    inData->PreUpdate();
    inData->InternalUpdate();

    cerr << "Generating Data to meet request" << *(outData->GetGenericUpdateExtent()) << endl;

    outData->SetExtent(inData->GetExtent());
    outData->GetPointData()->PassData(inData->GetPointData());

    // Save the image in cache.
    // Find a spot to put the data.
    for (i = 0; i < VTK_CACHE_NUMBER; ++i)
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



