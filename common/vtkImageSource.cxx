/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSource.cxx
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
#include "vtkImageSimpleCache.h"
#include "vtkImageSource.h"

#include <stdio.h>

//----------------------------------------------------------------------------
vtkImageSource::vtkImageSource()
{
  this->Output = NULL;

  this->StartMethod = NULL;
  this->StartMethodArgDelete = NULL;
  this->StartMethodArg = NULL;
  this->ProgressMethod = NULL;
  this->ProgressMethodArgDelete = NULL;
  this->ProgressMethodArg = NULL;
  this->EndMethod = NULL;
  this->EndMethodArgDelete = NULL;
  this->EndMethodArg = NULL;
  this->AbortExecute = 0;
  this->Progress = 0.0;
}


//----------------------------------------------------------------------------
// Description:
// Destructor: Delete the cache as well. (should caches by reference counted?)
vtkImageSource::~vtkImageSource()
{
  if (this->Output)
    {
    this->Output->Delete();
    this->Output = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkImageSource::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
 
  vtkObject::PrintSelf(os,indent);

  os << indent << "AbortExecute: " << (this->AbortExecute ? "On\n" : "Off\n");
  os << indent << "Progress: " << this->Progress << "\n";

  if (this->Output)
    {
    os << indent << "Cache:\n";
    this->Output->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "Cache: NULL \n";
    }
}
  


//----------------------------------------------------------------------------
// Description:
// This method can be used to intercept a generate call made to a cache.
// It allows a source to generate a larger region than was originally 
// specified.  The default method does not alter the specified region extent.
void vtkImageSource::InterceptCacheUpdate()
{
}


//----------------------------------------------------------------------------
// Description:
// This method can be called directly.
// It simply forwards the update to the cache.
void vtkImageSource::Update()
{
  // Make sure there is an output.
  this->CheckCache();

  this->Output->Update();
}

  
//----------------------------------------------------------------------------
// Description:
// This method is called by the cache.
void vtkImageSource::InternalUpdate(vtkImageData *data)
{
  if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);  
  this->Execute(data);
  if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);  
}

//----------------------------------------------------------------------------
// Description:
// This method updates the cache with the whole image extent.
void vtkImageSource::UpdateWholeExtent()
{
  this->CheckCache();
  this->GetOutput()->SetUpdateExtentToWholeExtent();
  this->GetOutput()->Update();
}

//----------------------------------------------------------------------------
// Description:
// This function can be defined in a subclass to generate the data
// for a region.
void vtkImageSource::Execute(vtkImageData *)
{
  vtkErrorMacro(<< "Execute(): Method not defined.");
}

//----------------------------------------------------------------------------
// Description:
// Returns the cache object of the source.  If one does not exist, a default
// is created.
vtkImageCache *vtkImageSource::GetCache()
{
  this->CheckCache();
  
  return this->Output;
}



//----------------------------------------------------------------------------
// Description:
// Returns an object which will generate data for Regions.
vtkImageCache *vtkImageSource::GetOutput()
{
  return this->GetCache();
}




//----------------------------------------------------------------------------
// Description:
// Returns the maximum mtime of this source and every object which effects
// this sources output. 
unsigned long vtkImageSource::GetPipelineMTime()
{
  return this->GetMTime();
}


//----------------------------------------------------------------------------
// Description:
// Use this method to specify a cache object for the filter.  
// If a cache has been set previously, it is deleted, and caches
// are not reference counted yet.  BE CAREFUL.
// The Source of the Cache is set as a side action.
void vtkImageSource::SetCache(vtkImageCache *cache)
{
  if (cache)
    {
    cache->ReleaseData();
    cache->SetSource(this);
    }
  
  if (this->Output)
    {
    this->Output->Delete();
    }

  this->Output = cache;
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// This method sets the value of the caches ReleaseDataFlag.  When this flag
// is set, the cache releases its data after every generate.  When a default
// cache is created, this flag is automatically set.
void vtkImageSource::SetReleaseDataFlag(int value)
{
  this->CheckCache();
  this->Output->SetReleaseDataFlag(value);
}


//----------------------------------------------------------------------------
// Description:
// This method gets the value of the caches ReleaseDataFlag.
int vtkImageSource::GetReleaseDataFlag()
{
  this->CheckCache();
  return this->Output->GetReleaseDataFlag();
}

//----------------------------------------------------------------------------
// Description:
// This private method creates a cache if one has not been set.
// ReleaseDataFlag is turned on.
void vtkImageSource::CheckCache()
{
  // create a default cache if one has not been set
  if ( ! this->Output)
    {
    this->Output = vtkImageSimpleCache::New();
    this->Output->ReleaseDataFlagOn();
    this->Output->SetSource(this);
    this->Modified();
    }
}



void vtkImageSource::UpdateProgress(float amount)
{
  this->Progress = amount;
  if ( this->ProgressMethod )
    {
    (*this->ProgressMethod)(this->ProgressMethodArg);
    }
}

//----------------------------------------------------------------------------
// Description:
// Specify function to be called before object executes.
void vtkImageSource::SetStartMethod(void (*f)(void *), void *arg)
{
  if ( f != this->StartMethod || arg != this->StartMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->StartMethodArg)&&(this->StartMethodArgDelete))
      {
      (*this->StartMethodArgDelete)(this->StartMethodArg);
      }
    this->StartMethod = f;
    this->StartMethodArg = arg;
    this->Modified();
    }
}

// Description:
// Specify function to be called to show progress of filter
void vtkImageSource::SetProgressMethod(void (*f)(void *), void *arg)
{
  if ( f != this->ProgressMethod || arg != this->ProgressMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->ProgressMethodArg)&&(this->ProgressMethodArgDelete))
      {
      (*this->ProgressMethodArgDelete)(this->ProgressMethodArg);
      }
    this->ProgressMethod = f;
    this->ProgressMethodArg = arg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Description:
// Specify function to be called after object executes.
void vtkImageSource::SetEndMethod(void (*f)(void *), void *arg)
{
  if ( f != this->EndMethod || arg != this->EndMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->EndMethodArg)&&(this->EndMethodArgDelete))
      {
      (*this->EndMethodArgDelete)(this->EndMethodArg);
      }
    this->EndMethod = f;
    this->EndMethodArg = arg;
    this->Modified();
    }
}


//----------------------------------------------------------------------------
// Description:
// Set the arg delete method. This is used to free user memory.
void vtkImageSource::SetStartMethodArgDelete(void (*f)(void *))
{
  if ( f != this->StartMethodArgDelete)
    {
    this->StartMethodArgDelete = f;
    this->Modified();
    }
}

// Description:
// Set the arg delete method. This is used to free user memory.
void vtkImageSource::SetProgressMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ProgressMethodArgDelete)
    {
    this->ProgressMethodArgDelete = f;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Description:
// Set the arg delete method. This is used to free user memory.
void vtkImageSource::SetEndMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EndMethodArgDelete)
    {
    this->EndMethodArgDelete = f;
    this->Modified();
    }
}










