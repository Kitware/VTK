/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage3dDecomposedFilter.cxx
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
#include<math.h>
#include "vtkImage3dDecomposedFilter.hh"

//----------------------------------------------------------------------------
vtkImage3dDecomposedFilter::vtkImage3dDecomposedFilter()
{
  // create the filter chain 
  this->Filter0 = NULL;
  this->Filter1 = NULL;
  this->Filter2 = NULL;
}


//----------------------------------------------------------------------------
// Description:
// Destructor: Delete the sub filters.
vtkImage3dDecomposedFilter::~vtkImage3dDecomposedFilter()
{
  if (this->Filter0)
    {
    this->Filter0->Delete();
    }
  
  if (this->Filter1)
    {
    this->Filter1->Delete();
    }
  
  if (this->Filter2)
    {
    this->Filter2->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkImage3dDecomposedFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);
  if (this->Filter0)
    {
    os << indent << "Filter0: \n";
    this->Filter0->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "Filter0: NULL\n";
    }
  
  if (this->Filter1)
    {
    os << indent << "Filter1: \n";
    this->Filter1->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "Filter1: NULL\n";
    }
  
}




//----------------------------------------------------------------------------
// Description:
// Turn debugging output on. (in sub filters also)
void vtkImage3dDecomposedFilter::DebugOn()
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
  if (this->Filter2)
    {
    this->Filter2->DebugOn();
    }
}



//----------------------------------------------------------------------------
// Description:
// Pass modified message to sub filters.
void vtkImage3dDecomposedFilter::Modified()
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

  if (this->Filter2)
    {
    this->Filter2->Modified();
    }
}
  

//----------------------------------------------------------------------------
// Description:
// Set the Input of the filter.
void vtkImage3dDecomposedFilter::SetInput(vtkImageSource *input)
{
  this->Input = input;
  this->Modified();

  vtkDebugMacro(<< "SetInput: " << input->GetClassName()
		<< " (" << input << ")");

  if ( ! this->Filter0 || ! this->Filter1 || ! this->Filter2)
    {
    vtkErrorMacro(<< "SetInput: Sub filter not created yet.");
    return;
    }
  
  // set the input of the first sub filter 
  this->Filter0->SetInput(input);
  this->Filter1->SetInput(this->Filter0->GetOutput());
  this->Filter2->SetInput(this->Filter1->GetOutput());
}



//----------------------------------------------------------------------------
// Description:
// Set the plane of the smoothing.
void vtkImage3dDecomposedFilter::SetAxes3d(int axis0, int axis1, int axis2)
{
  vtkDebugMacro(<< "SetAxes: axis0 = " << axis0 << ", axis1 = " 
                << axis1 << ", axis2 = " << axis2);

  if ( ! this->Filter0 || ! this->Filter1 || ! this->Filter2)
    {
    vtkErrorMacro(<< "SetAxes3d: Sub filter not created yet.");
    return;
    }
  
  this->Filter0->SetAxis1d(axis0);
  this->Filter1->SetAxis1d(axis1);
  this->Filter2->SetAxis1d(axis2);
  this->Modified();
}




//----------------------------------------------------------------------------
// Description:
// This method sets the cache object of the filter.
// It justs feeds the request to the sub filter.
void vtkImage3dDecomposedFilter::SetCache(vtkImageCache *cache)
{
  vtkDebugMacro(<< "SetCache: (" << cache << ")");
  
  if ( ! this->Filter2)
    {
    vtkErrorMacro(<< "SetCache: Sub filter not created yet.");
    return;
    }
  
  this->Filter2->SetCache(cache);
}
  
//----------------------------------------------------------------------------
// Description:
// This method returns the cache to make a connection
// It justs feeds the request to the sub filter.
vtkImageSource *vtkImage3dDecomposedFilter::GetOutput()
{
  vtkImageSource *source;

  if ( ! this->Filter2)
    {
    vtkErrorMacro(<< "GetOutput: Sub filter not created yet.");
    return NULL;
    }
  
  source = this->Filter2->GetOutput();
  vtkDebugMacro(<< "GetOutput: returning source "
                << source->GetClassName() << " (" << source << ")");

  return source;
}
  

//----------------------------------------------------------------------------
// Description:
// This method returns the l;ast cache of the internal pipline.
vtkImageCache *vtkImage3dDecomposedFilter::GetCache()
{
  vtkImageCache *cache;

  if ( ! this->Filter2)
    {
    vtkErrorMacro(<< "GetCache: Sub filter not created yet.");
    return NULL;
    }
  
  cache = this->Filter2->GetCache();
  vtkDebugMacro(<< "GetOutput: returning cache "
                << cache->GetClassName() << " (" << cache << ")");

  return cache;
}
  


//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline before this filter.
// It propagates the message back.
unsigned long int vtkImage3dDecomposedFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;

  // This objects MTime
  time1 = this->GetMTime();

  if ( ! this->Filter2)
    {
    vtkWarningMacro(<< "GetPipelineMTime: Sub filter not created yet.");
    return NULL;
    }
  else
    {
    // Pipeline mtime
    time2 = this->Filter2->GetPipelineMTime();
    
    // Return the larger of the two
    if (time2 > time1)
      time1 = time2;
    }
  
  return time1;
}










