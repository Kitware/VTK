/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridSource.cxx
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
#include "vtkStructuredGridSource.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkStructuredGridSource, "1.24");
vtkStandardNewMacro(vtkStructuredGridSource);

//----------------------------------------------------------------------------
vtkStructuredGridSource::vtkStructuredGridSource()
{
  this->vtkSource::SetNthOutput(0, vtkStructuredGrid::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkStructuredGrid *vtkStructuredGridSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkStructuredGrid *)(this->Outputs[0]);
}


//----------------------------------------------------------------------------
void vtkStructuredGridSource::SetOutput(vtkStructuredGrid *output)
{
  this->vtkSource::SetNthOutput(0, output);
}


