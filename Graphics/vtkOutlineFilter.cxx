/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineFilter.cxx
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
#include "vtkOutlineFilter.h"
#include "vtkOutlineSource.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkOutlineFilter, "1.29");
vtkStandardNewMacro(vtkOutlineFilter);

vtkOutlineFilter::vtkOutlineFilter ()
{
  this->OutlineSource = vtkOutlineSource::New();
}

vtkOutlineFilter::~vtkOutlineFilter ()
{
  if (this->OutlineSource != NULL)
    {
    this->OutlineSource->Delete ();
    this->OutlineSource = NULL;
    }
}

void vtkOutlineFilter::Execute()
{
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<< "Creating dataset outline");

  //
  // Let OutlineSource do all the work
  //

  this->OutlineSource->SetBounds(this->GetInput()->GetBounds());
  this->OutlineSource->Update();

  output->CopyStructure(this->OutlineSource->GetOutput());

}


void vtkOutlineFilter::ExecuteInformation()
{
  
  vtkDebugMacro(<< "Creating dataset outline");

  //
  // Let OutlineSource do all the work
  //
  
  this->vtkSource::ExecuteInformation();

  this->OutlineSource->UpdateInformation();
}
