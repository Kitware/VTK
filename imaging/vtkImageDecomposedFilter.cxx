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
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageDecomposedFilter.h"

//----------------------------------------------------------------------------
vtkImageDecomposedFilter::vtkImageDecomposedFilter()
{
  int idx;

  for (idx = 0; idx < 4; ++idx)
    {
    this->Filters[idx] = NULL;
    }
}


//----------------------------------------------------------------------------
// Description:
// Destructor: Delete the sub filters.
vtkImageDecomposedFilter::~vtkImageDecomposedFilter()
{
  this->DeleteFilters();
}



//----------------------------------------------------------------------------
void vtkImageDecomposedFilter::PrintSelf(ostream& os, vtkIndent indent) {
  int idx;
  
  vtkImageFilter::PrintSelf(os,indent);

  for (idx = 0; idx < 4; ++idx)
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
void vtkImageDecomposedFilter::DeleteFilters()
{
  int idx;
  vtkImageFilter *lastFilter = this->Filters[this->NumberOfFilteredAxes];
  
  // Don't delete the cache if it is ours.
  if (lastFilter && (lastFilter->GetOutput() == this->GetOutput()))
    {
    lastFilter->SetCache(NULL);
    this->GetOutput()->SetSource(this);
    }
  
  for (idx = 0; idx < 4; ++idx)
    {
    if (this->Filters[idx])
      {
      this->Filters[idx]->Delete();
      this->Filters[idx] = NULL;
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
  for (idx = 0; idx < 4; ++idx)
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
  for (idx = 0; idx < 4; ++idx)
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
void vtkImageDecomposedFilter::SetInput(vtkImageCache *input)
{
  if (this->Input == input)
    {
    return;
    }
  
  this->Input = input;
  this->Modified();
  vtkDebugMacro(<< "SetInput: " << input->GetClassName()
     << " (" << input << ")");

  this->SetInternalInput(input);
}



//----------------------------------------------------------------------------
// Description:
// By specifying which axes are filtered, you are really just
// setting the Bypass flag of the four (one for each axis) filters.
void vtkImageDecomposedFilter::SetFilteredAxes(int num, int *axes)
{
  int idxFilter, idxAxis, flag;
  
  this->vtkImageFilter::SetFilteredAxes(num, axes);
  for (idxFilter = 0; idxFilter < 4; ++idxFilter)
    {
    // should this filter be on?
    flag = 1;
    for (idxAxis = 0; idxAxis < num; ++idxAxis)
      {
      if (axes[idxAxis] == idxFilter)
	{
	flag = 0;
	}
      }
    this->Filters[idxFilter]->SetBypass(flag);
    }
}



//----------------------------------------------------------------------------
// Description:
// Called after the filters have been created by the subclass.
// This method sets some generic ivars, and connect the filters
// together.
void vtkImageDecomposedFilter::InitializeFilters()
{
  int idx;
  
  // Set ivars
  for (idx = 0; idx < 4; ++idx)
    {
    if ( ! this->Filters[idx])
      {
      vtkErrorMacro("Filters not created");
      return;
      }
    this->Filters[idx]->SetInputMemoryLimit(this->GetInputMemoryLimit());
    }
  
  // set the output of the last filter. 
  if (this->GetOutput())
    {
    this->Filters[3]->SetCache(this->GetOutput());
    this->GetOutput()->SetSource(this->Filters[3]);
    }
}

  
  
//----------------------------------------------------------------------------
// Description:
// Set the Input of the sub pipeline.
void vtkImageDecomposedFilter::SetInternalInput(vtkImageCache *input)
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
  for (idx = 1; idx < 4; ++idx)
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
  
  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
    {
    if (this->Filters[idx])
      {
      this->Filters[idx]->SetInputMemoryLimit(limit);
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
  
  if (this->Filters[this->NumberOfFilteredAxes - 1])
    {
    this->Filters[this->NumberOfFilteredAxes - 1]->SetCache(cache);
    }
  this->vtkImageFilter::SetCache(cache);
}
  
//----------------------------------------------------------------------------
// Description:
// Causes the filter to execute, and put its results in cache.
void vtkImageDecomposedFilter::Update()
{
  vtkImageCache *cache;
  
  if (this->NumberOfFilteredAxes == 0)
    {
    vtkErrorMacro("Update: NumberOfFilteredAxes not set.");
    return;
    }
  if (this->Filters[this->NumberOfFilteredAxes - 1] == NULL)
    {
    vtkErrorMacro("Update: Last filter not created");
    return;
    }
  
  cache = this->GetOutput();
  if (cache == NULL)
    {
    vtkErrorMacro("Update: NumberOfFilteredAxes not set.");
    return;
    }
  
  cache->Update();
}


//----------------------------------------------------------------------------
// Description:
// Specify function to be called before object executes.
void vtkImageDecomposedFilter::SetStartMethod(void (*f)(void *), void *arg)
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
    if (this->Filters[0])
      {
      this->Filters[0]->SetStartMethod(f, arg);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Description:
// Specify function to be called after object executes.
void vtkImageDecomposedFilter::SetEndMethod(void (*f)(void *), void *arg)
{
  if ( f != this->EndMethod || arg != this->EndMethodArg )
    {
    // delete the current arg if there is one and a delete method
    if ((this->EndMethodArg)&&(this->EndMethodArgDelete))
      {
      (*this->EndMethodArgDelete)(this->EndMethodArg);
      }
    this->EndMethod = f;
    this->EndMethodArg = arg;
    if (this->Filters[this->NumberOfFilteredAxes - 1])
      {
      this->Filters[this->NumberOfFilteredAxes - 1]->SetEndMethod(f, arg);
      }
    this->Modified();
    }
}







