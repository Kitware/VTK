/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStaticCache.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkImageStaticCache.h"

//----------------------------------------------------------------------------
vtkImageStaticCache::vtkImageStaticCache()
{
  this->CachedData = NULL;
}


//----------------------------------------------------------------------------
vtkImageStaticCache::~vtkImageStaticCache()
{
  if (this->CachedData)
    {
    this->CachedData->Delete();
    this->CachedData = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkImageStaticCache::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageCache::PrintSelf(os,indent);

  if ( ! this->CachedData)
    {
    os << indent << "CachedData: None";
    }
  else
    {
    os << indent << "CachedData: \n";
    this->CachedData->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
// Description:
// This Method deletes any data in cache. For a static cache the data cannot be
// released except by deleteing the instance or providing a new CachedData
void vtkImageStaticCache::ReleaseData()
{
}

vtkImageData *vtkImageStaticCache::UpdateAndReturnData()
{
  return this->CachedData;
}


// Description:
// This method updates the region specified by "UpdateExtent".  
void vtkImageStaticCache::Update()
{
  // Make sure image information is upto date
  this->UpdateImageInformation();
  
  this->ClipUpdateExtentWithWholeExtent();
}



//----------------------------------------------------------------------------
// Description:
// return the un filled data of the UpdateExtent in this cache.
vtkImageData *vtkImageStaticCache::GetData()
{
  if (! this->CachedData)
    {
    return NULL;
    }
  
  return this->CachedData;
}

//----------------------------------------------------------------------------
// Description:
// This method updates the instance variables "WholeExtent", "Spacing", 
// "Origin", "Bounds" etc.
// It needs to be separate from "Update" because the image information
// may be needed to compute the required UpdateExtent of the input
// (see "vtkImageFilter").
void vtkImageStaticCache::UpdateImageInformation()
{
  if (!this->CachedData)
    {
    vtkWarningMacro("No data currently in static cache!");
    return;
    }
  
  this->SetWholeExtent(this->CachedData->GetExtent());
  this->SetOrigin(this->CachedData->GetOrigin());
  this->SetSpacing(this->CachedData->GetSpacing());
}

//----------------------------------------------------------------------------
// Description:
// Make this a separate method to avoid another GetPipelineMTime call.
unsigned long vtkImageStaticCache::GetPipelineMTime()
{
  if (!this->CachedData) return 0;
  return this->CachedData->GetMTime();
}
