/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetSource.cxx
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
#include "vtkDataSetSource.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkDataSetSource, "1.10");

vtkDataSetSource::vtkDataSetSource()
{

}

//----------------------------------------------------------------------------
vtkDataSet *vtkDataSetSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkDataSetSource::SetOutput(vtkDataSet *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

