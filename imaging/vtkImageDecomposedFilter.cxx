/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDecomposedFilter.cxx
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
#include<math.h>
#include "vtkImageDecomposedFilter.h"

//----------------------------------------------------------------------------
vtkImageDecomposedFilter::vtkImageDecomposedFilter()
{
  int idx;

  this->Dimensionality = 0;
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Filters[idx] = NULL;
    }
}


//----------------------------------------------------------------------------
// Description:
// Destructor: Delete the sub filters.
vtkImageDecomposedFilter::~vtkImageDecomposedFilter()
{
  int idx;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (this->Filters[idx])
      {
      this->Filters[idx]->Delete();
      this->Filters[idx] = NULL;
      }
    }
}



//----------------------------------------------------------------------------
void vtkImageDecomposedFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageFilter::PrintSelf(os,indent);
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    if (this->Filters[idx])
      {
      os << indent << "Filter" << idx << ":\n";
      this->Filters[idx]->PrintSelf(os, indent.GetNextIndent());
      }
    else
      {
      os << indent << "Filter" << idx << ": NULL\n";
      }
    }
  
}




//----------------------------------------------------------------------------
// Description:
// Turn debugging output on. (in sub filters also)
void vtkImageDecomposedFilter::DebugOn()
{
  int idx;
  
  this->vtkObject::DebugOn();
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    if (this->Filters[idx])
      {
      this->Filters[idx]->DebugOn();
      }
    }
}



//----------------------------------------------------------------------------
// Description:
// Pass modified message to sub filters.
void vtkImageDecomposedFilter::Modified()
{
  int idx;
  
  this->vtkObject::Modified();
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    if (this->Filters[idx])
      {
      this->Filters[idx]->Modified();
      }
    }
}
  

//----------------------------------------------------------------------------
// Description:
// Set the Input of the filter.
void vtkImageDecomposedFilter::SetInput(vtkImageSource *input)
{
  this->Input = input;
  this->Modified();
  vtkDebugMacro(<< "SetInput: " << input->GetClassName()
		<< " (" << input << ")");

  if (this->Filters[0])
    {
    this->SetInternalInput(input);
    }
}


//----------------------------------------------------------------------------
// Description:
// Set the Input of the sub pipeline.
void vtkImageDecomposedFilter::SetInternalInput(vtkImageSource *input)
{
  int idx;
  
  vtkDebugMacro(<< "SetInternalInput: " << input->GetClassName()
		<< " (" << input << ")");

  if ( ! this->Filters[0])
    {
    vtkDebugMacro("SetInternalInput: sub filters do not exists.");
    return;
    }
  
  this->Filters[0]->SetInput(input);
  
  // Connect all the filters
  // This is conditional on having the input because
  // the OutputScalarTypes are computed when the pipeline is connected.
  for (idx = 1; idx < this->Dimensionality; ++idx)
    {
    if ( ! this->Filters[idx])
      {
      vtkErrorMacro(<< "SetInput: cannot find filter " << idx);
      return;
      }
    this->Filters[idx]->SetInput(this->Filters[idx-1]->GetOutput());
    }
}


//----------------------------------------------------------------------------
// Description:
// Each sub filter gets the same limit.
void vtkImageDecomposedFilter::SetInputMemoryLimit(long limit)
{
  int idx;
  
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    if ( ! this->Filters[idx])
      {
      vtkErrorMacro(<< "SetInputMemoryLimit: Sub filter not created yet. "
		    << "Subclasses SetDimensionality did not work");
      return;
      }
    this->Filters[idx]->SetInputMemoryLimit(limit);
    }
  
  this->Modified();
}










//----------------------------------------------------------------------------
// Description:
// Set the plane of the smoothing.
void vtkImageDecomposedFilter::SetAxes(int num, int *axes)
{
  int idx;
  
  this->vtkImageFilter::SetAxes(num, axes);
  for (idx = 0; idx < num; ++idx)
    {
    if (this->Filters[idx])
      {
      this->Filters[idx]->SetAxes(axes[idx]);
      }
    }
  this->Modified();
}


//----------------------------------------------------------------------------
// Description:
// This method sets the cache object of the filter.
// It justs feeds the request to the sub filter.
void vtkImageDecomposedFilter::SetCache(vtkImageCache *cache)
{
  vtkDebugMacro(<< "SetCache: (" << cache << ")");
  
  if ( ! this->Filters[this->Dimensionality - 1])
    {
    vtkErrorMacro(<< "SetCache: Sub filter not created yet. "
		  << "SetDimensionality first");
    return;
    }
  
  this->Filters[this->Dimensionality - 1]->SetCache(cache);
}
  
//----------------------------------------------------------------------------
// Description:
// This method tells the last filter to save or release its output.
void vtkImageDecomposedFilter::SetReleaseDataFlag(int flag)
{
  if ( ! this->Filters[this->Dimensionality - 1])
    {
    vtkErrorMacro(<< "SetReleaseDataFlag: Sub filter not created yet. "
		  << "SetDimensionality first.");
    return;
    }
  
  this->Filters[this->Dimensionality - 1]->SetReleaseDataFlag(flag);
}
  


//----------------------------------------------------------------------------
// Description:
// This method returns the cache to make a connection
// It justs feeds the request to the sub filter.
vtkImageCache *vtkImageDecomposedFilter::GetOutput()
{
  vtkImageCache *source;

  if ( ! this->Filters[this->Dimensionality - 1])
    {
    vtkErrorMacro(<< "GetOutput: Sub filter not created yet. "
		  << "SetDimensionality first");
    return NULL;
    }
  
  source = this->Filters[this->Dimensionality - 1]->GetOutput();

  vtkDebugMacro(<< "GetOutput: returning source "
                << source->GetClassName() << " (" << source << ")");

  return source;
}
  

//----------------------------------------------------------------------------
// Description:
// This method returns the l;ast cache of the internal pipline.
vtkImageCache *vtkImageDecomposedFilter::GetCache()
{
  return (vtkImageCache *)(this->GetOutput());
}
  


//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline before this filter.
// It propagates the message back.
unsigned long int vtkImageDecomposedFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;

  // This objects MTime
  time1 = this->GetMTime();

  if ( ! this->Filters[this->Dimensionality - 1])
    {
    vtkWarningMacro(<< "GetPipelineMTime: Sub filter not created yet. "
		    << "SetDimensionality first");
    }
  else
    {
    time2 = this->Filters[this->Dimensionality - 1]->GetPipelineMTime();
    // Return the larger of the two
    if (time2 > time1)
      time1 = time2;
    }
  
  return time1;
}










