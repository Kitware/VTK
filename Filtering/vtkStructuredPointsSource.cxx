/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredPointsSource.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStructuredPoints.h"

vtkCxxRevisionMacro(vtkStructuredPointsSource, "1.36");

//----------------------------------------------------------------------------
vtkStructuredPointsSource::vtkStructuredPointsSource()
{
  this->SetOutput(vtkStructuredPoints::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
void vtkStructuredPointsSource::SetOutput(vtkStructuredPoints *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkStructuredPoints *vtkStructuredPointsSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkStructuredPoints *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkStructuredPoints *vtkStructuredPointsSource::GetOutput(int idx)
{
  return (vtkStructuredPoints *) this->vtkSource::GetOutput(idx); 
}

//----------------------------------------------------------------------------
// Default method performs Update to get information.  Not all the old
// structured points sources compute information
void vtkStructuredPointsSource::ExecuteInformation()
{
  vtkStructuredPoints *output = this->GetOutput();
  vtkDataArray *scalars;

  output->UpdateData();
  scalars = output->GetPointData()->GetScalars();

  if (scalars)
    {
    output->SetScalarType(scalars->GetDataType());
    output->SetNumberOfScalarComponents(scalars->GetNumberOfComponents());
    }

  output->SetWholeExtent(output->GetExtent());
}

//----------------------------------------------------------------------------
void vtkStructuredPointsSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
