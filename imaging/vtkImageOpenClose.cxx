/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOpenClose.cxx
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
#include "vtkImageOpenClose.h"

//----------------------------------------------------------------------------
vtkImageOpenClose::vtkImageOpenClose()
{
  // create the filter chain 
  this->Filter0 = vtkImageDilateErode::New();
  this->Filter1 = vtkImageDilateErode::New();
  this->SetOpenValue(0.0);
  this->SetCloseValue(255.0);

  // This dummy filter does not have an execute function, but
  // what is its dimensionality (that is not used)?
}


//----------------------------------------------------------------------------
// Description:
// Destructor: Delete the sub filters.
vtkImageOpenClose::~vtkImageOpenClose()
{
  if (this->Filter0)
    {
    this->Filter0->Delete();
    }
  
  if (this->Filter1)
    {
    this->Filter1->Delete();
    }
}


//----------------------------------------------------------------------------
void vtkImageOpenClose::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);
  os << indent << "Filter0: \n";
  this->Filter0->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Filter1: \n";
  this->Filter1->PrintSelf(os, indent.GetNextIndent());
}



//----------------------------------------------------------------------------
// Description:
// Turn debugging output on. (in sub filters also)
void vtkImageOpenClose::DebugOn()
{
  this->vtkObject::DebugOn();
  if (this->Filter0)
    {
    this->Filter0->DebugOn();
    }
  if (this->Filter1)
    {
    this->Filter1->DebugOn();
    }
}



//----------------------------------------------------------------------------
// Description:
// Pass modified message to sub filters.
void vtkImageOpenClose::Modified()
{
  this->vtkObject::Modified();
  if (this->Filter0)
    {
    this->Filter0->Modified();
    }
  
  if (this->Filter1)
    {
    this->Filter1->Modified();
    }
}




//----------------------------------------------------------------------------
// Description:
// This method sets the cache object of the filter.
// It justs feeds the request to the sub filter.
void vtkImageOpenClose::SetCache(vtkImageCache *cache)
{
  vtkDebugMacro(<< "SetCache: (" << cache << ")");
  
  if ( ! this->Filter1)
    {
    vtkErrorMacro(<< "SetCache: Sub filter not created yet.");
    return;
    }
  
  this->Filter1->SetCache(cache);
}
  
//----------------------------------------------------------------------------
// Description:
// This method returns the cache to make a connection
// It justs feeds the request to the sub filter.
vtkImageSource *vtkImageOpenClose::GetOutput()
{
  vtkImageSource *source;

  if ( ! this->Filter1)
    {
    vtkErrorMacro(<< "GetOutput: Sub filter not created yet.");
    return NULL;
    }
  
  source = this->Filter1->GetOutput();
  vtkDebugMacro(<< "GetOutput: returning source "
                << source->GetClassName() << " (" << source << ")");

  return source;
}
  

//----------------------------------------------------------------------------
// Description:
// This method returns the l;ast cache of the internal pipline.
vtkImageCache *vtkImageOpenClose::GetCache()
{
  vtkImageCache *cache;

  if ( ! this->Filter1)
    {
    vtkErrorMacro(<< "GetCache: Sub filter not created yet.");
    return NULL;
    }
  
  cache = this->Filter1->GetCache();
  vtkDebugMacro(<< "GetOutput: returning cache "
                << cache->GetClassName() << " (" << cache << ")");

  return cache;
}
  


//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline before this filter.
// It propagates the message back.
unsigned long int vtkImageOpenClose::GetPipelineMTime()
{
  unsigned long int time1, time2;

  // This objects MTime
  time1 = this->GetMTime();

  if ( ! this->Filter1)
    {
    vtkWarningMacro(<< "GetPipelineMTime: Sub filter not created yet.");
    return 0;
    }
  else
    {
    // Pipeline mtime
    time2 = this->Filter1->GetPipelineMTime();
    
    // Return the larger of the two
    if (time2 > time1)
      time1 = time2;
    }
  
  return time1;
}




//----------------------------------------------------------------------------
// Description:
// Set the Input of the filter.
void vtkImageOpenClose::SetInput(vtkImageSource *input)
{
  this->Input = input;
  this->Modified();

  vtkDebugMacro(<< "SetInput: " << input->GetClassName()
		<< " (" << input << ")");

  if ( ! this->Filter0 || ! this->Filter1)
    {
    vtkErrorMacro(<< "SetInput: Sub filter not created yet.");
    return;
    }
  
  // set the input of the first sub filter 
  this->Filter0->SetInput(input);
  this->Filter1->SetInput(this->Filter0->GetOutput());
}



//----------------------------------------------------------------------------
// Description:
// Set the plane of the smoothing.
void vtkImageOpenClose::SetAxes(int axis0, int axis1, int axis2)
{
  vtkDebugMacro(<< "SetAxes: axis0 = " << axis0 << ", axis1 = " << axis1
                << ", axis2 = " << axis2);

  if ( ! this->Filter0 || ! this->Filter1)
    {
    vtkErrorMacro(<< "SetAxes: Sub filter not created yet.");
    return;
    }
  
  this->Filter0->SetAxes(axis0, axis1, axis2);
  this->Filter1->SetAxes(axis0, axis1, axis2);
  this->Modified();
}



//----------------------------------------------------------------------------
// Description:
// Selects the size of gaps or objects removed. All axis get the same 
// size creating a sphere footprint.
void vtkImageOpenClose::SetKernelSize(int size)
{
  this->Modified();
  
  if ( ! this->Filter0 || ! this->Filter1)
    {
    vtkErrorMacro(<< "SetKernelSize: Sub filter not created yet.");
    return;
    }
  
  this->Filter0->SetKernelSize(size);
  this->Filter1->SetKernelSize(size);
}

//----------------------------------------------------------------------------
// Description:
// Selects the size of gaps or objects removed.
void vtkImageOpenClose::SetKernelSize(int size0, int size1, int size2)
{
  this->Modified();
  
  if ( ! this->Filter0 || ! this->Filter1)
    {
    vtkErrorMacro(<< "SetKernelSize: Sub filter not created yet.");
    return;
    }
  
  this->Filter0->SetKernelSize(size0, size1, size2);
  this->Filter1->SetKernelSize(size0, size1, size2);
}

  

//----------------------------------------------------------------------------
// Description:
// Determines the value that will closed.
// Close value is first dilated, and then eroded
void vtkImageOpenClose::SetCloseValue(float value)
{
  if ( ! this->Filter0 || ! this->Filter1)
    {
    vtkErrorMacro(<< "SetCloseValue: Sub filter not created yet.");
    return;
    }
  
  this->Filter0->SetDilateValue(value);
  this->Filter1->SetErodeValue(value);
  
}

//----------------------------------------------------------------------------
float vtkImageOpenClose::GetCloseValue()
{
  if ( ! this->Filter0)
    {
    vtkErrorMacro(<< "GetCloseValue: Sub filter not created yet.");
    return 0.0;
    }
  
  return this->Filter0->GetDilateValue();
}

  


//----------------------------------------------------------------------------
// Description:
// Determines the value that will opened.  
// Open value is first eroded, and then dilated.
void vtkImageOpenClose::SetOpenValue(float value)
{
  if ( ! this->Filter0 || ! this->Filter1)
    {
    vtkErrorMacro(<< "SetOpenValue: Sub filter not created yet.");
    return;
    }
  
  this->Filter0->SetErodeValue(value);
  this->Filter1->SetDilateValue(value);
}


//----------------------------------------------------------------------------
float vtkImageOpenClose::GetOpenValue()
{
  if ( ! this->Filter0)
    {
    vtkErrorMacro(<< "GetOpenValue: Sub filter not created yet.");
    return 0.0;
    }
  
  return this->Filter0->GetErodeValue();
}

  


















