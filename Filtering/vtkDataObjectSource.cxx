/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectSource.cxx
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
#include "vtkDataObjectSource.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkDataObjectSource, "1.11");
vtkStandardNewMacro(vtkDataObjectSource);

vtkDataObjectSource::vtkDataObjectSource()
{
  this->SetOutput(vtkDataObject::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}


//----------------------------------------------------------------------------
vtkDataObject *vtkDataObjectSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataObject *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkDataObjectSource::SetOutput(vtkDataObject *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

