/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOpenClose3D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageOpenClose3D.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageOpenClose3D, "1.21");
vtkStandardNewMacro(vtkImageOpenClose3D);

//----------------------------------------------------------------------------
// functions to convert progress calls.
void vtkImageOpenClose3DUpdateProgress0(void *arg)
{
  vtkImageOpenClose3D *self = (vtkImageOpenClose3D *)(arg);
  // fprintf(stderr, "progress0: %f\n",(0.5 * self->GetFilter0()->GetProgress()));
  self->UpdateProgress(0.5 * self->GetFilter0()->GetProgress());
}

void vtkImageOpenClose3DUpdateProgress1(void *arg)
{
  vtkImageOpenClose3D *self = (vtkImageOpenClose3D *)(arg);
  self->UpdateProgress(0.5 * self->GetFilter1()->GetProgress() + 0.5);
}


//----------------------------------------------------------------------------
vtkImageOpenClose3D::vtkImageOpenClose3D()
{
  // create the filter chain 
  this->Filter0 = vtkImageDilateErode3D::New();
  this->Filter0->SetProgressMethod(vtkImageOpenClose3DUpdateProgress0,
                                   (void *)this);
  this->Filter1 = vtkImageDilateErode3D::New();
  this->Filter1->SetProgressMethod(vtkImageOpenClose3DUpdateProgress1, 
                                   (void *)this); 
  this->SetOpenValue(0.0);
  this->SetCloseValue(255.0);

  // This dummy filter does not have an execute function.
}


//----------------------------------------------------------------------------
// Destructor: Delete the sub filters.
vtkImageOpenClose3D::~vtkImageOpenClose3D()
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
void vtkImageOpenClose3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Filter0: \n";
  this->Filter0->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Filter1: \n";
  this->Filter1->PrintSelf(os, indent.GetNextIndent());
}



//----------------------------------------------------------------------------
// Turn debugging output on. (in sub filters also)
void vtkImageOpenClose3D::DebugOn()
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
void vtkImageOpenClose3D::DebugOff()
{
  this->vtkObject::DebugOff();
  if (this->Filter0)
    {
    this->Filter0->DebugOff();
    }
  if (this->Filter1)
    {
    this->Filter1->DebugOff();
    }
}



//----------------------------------------------------------------------------
// Pass modified message to sub filters.
void vtkImageOpenClose3D::Modified()
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
// This method returns the cache to make a connection
// It justs feeds the request to the sub filter.
vtkImageData *vtkImageOpenClose3D::GetOutput()
{
  vtkImageData *source;

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
// This method considers the sub filters MTimes when computing this objects
// MTime
unsigned long int vtkImageOpenClose3D::GetMTime()
{
  unsigned long int t1, t2;
  
  t1 = this->vtkImageToImageFilter::GetMTime();
  if (this->Filter0)
    {
    t2 = this->Filter0->GetMTime();
    if (t2 > t1)
      {
      t1 = t2;
      }
    }
  if (this->Filter1)
    {
    t2 = this->Filter1->GetMTime();
    if (t2 > t1)
      {
      t1 = t2;
      }
    }
  
  return t1;
}

  






//----------------------------------------------------------------------------
// Set the Input of the filter.
void vtkImageOpenClose3D::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);

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
// Selects the size of gaps or objects removed.
void vtkImageOpenClose3D::SetKernelSize(int size0, int size1, int size2)
{
  if ( ! this->Filter0 || ! this->Filter1)
    {
    vtkErrorMacro(<< "SetKernelSize: Sub filter not created yet.");
    return;
    }
  
  this->Filter0->SetKernelSize(size0, size1, size2);
  this->Filter1->SetKernelSize(size0, size1, size2);
  // Sub filters take care of modified.
}

  

//----------------------------------------------------------------------------
// Determines the value that will closed.
// Close value is first dilated, and then eroded
void vtkImageOpenClose3D::SetCloseValue(float value)
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
float vtkImageOpenClose3D::GetCloseValue()
{
  if ( ! this->Filter0)
    {
    vtkErrorMacro(<< "GetCloseValue: Sub filter not created yet.");
    return 0.0;
    }
  
  return this->Filter0->GetDilateValue();
}

  


//----------------------------------------------------------------------------
// Determines the value that will opened.  
// Open value is first eroded, and then dilated.
void vtkImageOpenClose3D::SetOpenValue(float value)
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
float vtkImageOpenClose3D::GetOpenValue()
{
  if ( ! this->Filter0)
    {
    vtkErrorMacro(<< "GetOpenValue: Sub filter not created yet.");
    return 0.0;
    }
  
  return this->Filter0->GetErodeValue();
}



















