/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericOutlineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericOutlineFilter.h"

#include "vtkGenericDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkGenericOutlineFilter, "1.1");
vtkStandardNewMacro(vtkGenericOutlineFilter);

vtkGenericOutlineFilter::vtkGenericOutlineFilter ()
{
  this->OutlineSource = vtkOutlineSource::New();
}

vtkGenericOutlineFilter::~vtkGenericOutlineFilter ()
{
  if (this->OutlineSource != NULL)
    {
    this->OutlineSource->Delete ();
    this->OutlineSource = NULL;
    }
}

void vtkGenericOutlineFilter::Execute()
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


void vtkGenericOutlineFilter::ExecuteInformation()
{
  
  vtkDebugMacro(<< "Creating dataset outline");

  //
  // Let OutlineSource do all the work
  //
  
  this->vtkSource::ExecuteInformation();

  this->OutlineSource->UpdateInformation();
}
