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
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageSource.h"

#include <stdio.h>

//----------------------------------------------------------------------------
vtkImageSource::vtkImageSource()
{
  this->Output = NULL;
  this->NumberOfExecutionAxes = -1; // default to 4?
  this->ExecutionAxes[0] = VTK_IMAGE_X_AXIS;
  this->ExecutionAxes[1] = VTK_IMAGE_Y_AXIS;
  this->ExecutionAxes[2] = VTK_IMAGE_Z_AXIS;
  this->ExecutionAxes[3] = VTK_IMAGE_TIME_AXIS;
  this->ExecutionAxes[4] = VTK_IMAGE_COMPONENT_AXIS;
  
  this->StartMethod = NULL;
  this->StartMethodArgDelete = NULL;
  this->StartMethodArg = NULL;
  this->EndMethod = NULL;
  this->EndMethodArgDelete = NULL;
  this->EndMethodArg = NULL;
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

  os << indent << "NumberOfExecutionAxes: " 
     << this->NumberOfExecutionAxes << "\n";
  if (this->NumberOfExecutionAxes > 0)
    {
    os << indent << "ExecutionAxes: (" 
       << vtkImageAxisNameMacro(this->ExecutionAxes[0]);
    for (idx = 1; idx < this->NumberOfExecutionAxes; ++idx)
      {
      os << ", " << vtkImageAxisNameMacro(this->ExecutionAxes[idx]);
      }
    os << ")\n";
    }

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
void vtkImageSource::InterceptCacheUpdate(vtkImageCache *cache)
{
  cache = cache;
}


//----------------------------------------------------------------------------
// Description:
// This method can be called by the cache or directly.
void vtkImageSource::Update()
{
  vtkImageRegion *region;

  // Make sure there is an output.
  this->CheckCache();
  // Duplicated here because user can call this update directly.
  this->UpdateImageInformation();
  this->Output->ClipUpdateExtentWithWholeExtent();
  
  // Make sure the subclss has defined the NumberOfExecutionAxes.
  // It is needed to terminate recursion.
  if (this->NumberOfExecutionAxes < 0)
    {
    vtkErrorMacro(<< "Subclass has not set NumberOfExecutionAxes");
    return;
    }
  region = this->GetOutput()->GetScalarRegion();
  region->SetAxes(VTK_IMAGE_DIMENSIONS, this->ExecutionAxes);
  
  if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);  
  // Call a recursive method that will loop over the extra axes.
  this->RecursiveLoopUpdate(VTK_IMAGE_DIMENSIONS, region);
  if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);  

  region->Delete();
}

  
//----------------------------------------------------------------------------
// Description:
// This is a recursive method that loops over "extra" axes.
// The recursion stops when the dimensionality of the regions
// are equal to "NumberOfExecutionAxes"
void 
vtkImageSource::RecursiveLoopUpdate(int dim, vtkImageRegion *region)
{
  // Terminate recursion?
  if (dim == this->NumberOfExecutionAxes)
    {
    this->Execute(region);
    return;
    }
  else
    {
    int coordinate;
    int min, max;
    int axis = this->ExecutionAxes[dim - 1];
    
    region->GetAxisExtent(axis, min, max);
    for (coordinate = min; coordinate <= max; ++coordinate)
      {
      // colapse one dimension.
      region->SetAxisExtent(axis, coordinate, coordinate);
      // Continue recursion.
      this->vtkImageSource::RecursiveLoopUpdate(dim - 1, region);
      }
    // restore original extent
    region->SetAxisExtent(axis, min, max);  
    }
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
void vtkImageSource::Execute(vtkImageRegion *region)
{
  region = region;
  vtkErrorMacro(<< "Execute(region): Method not defined.");
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
    if (cache)
      {
      cache->SetScalarType(this->Output->GetScalarType());
      }
    vtkDebugMacro("SetCache: Delete the cache I have. Note: The application "
		  << "must make sure that nothing references this cache.");
    this->Output->Delete();
    }

  this->Output = cache;
  this->Modified();
}

//----------------------------------------------------------------------------
// Also contains logic to determine the superclass loop axes.
void vtkImageSource::SetExecutionAxes(int dim, int *axes)
{
  int allAxes[VTK_IMAGE_DIMENSIONS];
  int idx1, idx2;
  int modified = 0;

  // We do not check number for modifiation, because it may be different.
  this->NumberOfExecutionAxes = dim;
  
  // Copy axes
  for (idx1 = 0; idx1 < dim; ++idx1)
    {
    if (this->ExecutionAxes[idx1] != axes[idx1])
      {
      modified = 1;
      }
    allAxes[idx1] = axes[idx1];
    }
  
  // choose the rest of the axes
  // look through original axes to find ones not taken.
  // By this point scalars, vectors, .. would be separe, 
  // so consider components
  for (idx1 = 0; idx1 < 5 && dim < 5; ++idx1)
    {
    for (idx2 = 0; idx2 < dim; ++idx2)
      {
      if (allAxes[idx2] == this->ExecutionAxes[idx1])
	{
	break;
	}
      }
    if (idx2 == dim)
      {
      allAxes[dim] = this->ExecutionAxes[idx1];
      ++dim;
      }
    }
  
  // Sanity check
  if (dim != VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "SetExecutionAxes: Could not complete unspecified axes.");
    return;
    }
  
  // Actuall set the axes.
  for (idx1 = 0; idx1 < VTK_IMAGE_DIMENSIONS; ++idx1)
    {
    this->ExecutionAxes[idx1] = allAxes[idx1];
    }
  
  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImageSource::SetExecutionAxes(int axis)
{
  int axes[1];
  axes[0] = axis;
  this->SetExecutionAxes(1, axes);
}
//----------------------------------------------------------------------------
void vtkImageSource::SetExecutionAxes(int axis0, int axis1)
{
  int axes[2];
  axes[0] = axis0;
  axes[1] = axis1;
  this->SetExecutionAxes(2, axes);
}
//----------------------------------------------------------------------------
void vtkImageSource::SetExecutionAxes(int axis0, int axis1, int axis2)
{
  int axes[3];
  axes[0] = axis0;
  axes[1] = axis1;
  axes[2] = axis2;
  this->SetExecutionAxes(3, axes);
}
//----------------------------------------------------------------------------
void vtkImageSource::SetExecutionAxes(int axis0, int axis1, 
					    int axis2, int axis3)
{
  int axes[4];
  axes[0] = axis0;  axes[1] = axis1;
  axes[2] = axis2;  axes[3] = axis3;
  this->SetExecutionAxes(4, axes);
}





//----------------------------------------------------------------------------
void vtkImageSource::GetExecutionAxes(int dim, int *axes)
{
  int idx;

  // Copy axes
  for (idx = 0; idx < dim; ++idx)
    {
    axes[idx] = this->ExecutionAxes[idx];
    }
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
// This method sets the value of the caches ScalarType.
void vtkImageSource::SetOutputScalarType(int value)
{
  this->CheckCache();
  this->Output->SetScalarType(value);
}

//----------------------------------------------------------------------------
// Description:
// This method returns the caches ScalarType.
int vtkImageSource::GetOutputScalarType()
{
  this->CheckCache();
  return this->Output->GetScalarType();
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
    this->Output = vtkImageCache::New();
    this->Output->ReleaseDataFlagOn();
    this->Output->SetSource(this);
    this->Modified();
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










