/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineCornerFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOutlineCornerFilter.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineCornerSource.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkOutlineCornerFilter, "1.9");
vtkStandardNewMacro(vtkOutlineCornerFilter);

vtkOutlineCornerFilter::vtkOutlineCornerFilter ()
{
  this->CornerFactor = 0.2;
  this->OutlineCornerSource = vtkOutlineCornerSource::New();
}

vtkOutlineCornerFilter::~vtkOutlineCornerFilter ()
{
  if (this->OutlineCornerSource != NULL)
    {
    this->OutlineCornerSource->Delete ();
    this->OutlineCornerSource = NULL;
    }
}

void vtkOutlineCornerFilter::Execute()
{
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<< "Creating dataset outline");

  //
  // Let OutlineCornerSource do all the work
  //
  this->OutlineCornerSource->SetBounds(this->GetInput()->GetBounds());
  this->OutlineCornerSource->SetCornerFactor(this->GetCornerFactor());
  this->OutlineCornerSource->Update();

  output->CopyStructure(this->OutlineCornerSource->GetOutput());

}


void vtkOutlineCornerFilter::ExecuteInformation()
{
  vtkDebugMacro(<< "Creating dataset outline");

  //
  // Let OutlineCornerSource do all the work
  //
  
  this->vtkSource::ExecuteInformation();

  this->OutlineCornerSource->UpdateInformation();
}


void vtkOutlineCornerFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CornerFactor: " << this->CornerFactor << "\n";
}
