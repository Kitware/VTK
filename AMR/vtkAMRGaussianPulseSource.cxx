/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRGaussianPulseSource.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRGaussianPulseSource.h"
#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkAMRGaussianPulseSource);

//------------------------------------------------------------------------------
vtkAMRGaussianPulseSource::vtkAMRGaussianPulseSource()
{
  this->SetNumberOfInputPorts(0);

  this->RootSpacing[0] =
  this->RootSpacing[1] =
  this->RootSpacing[2] = 0.5;

  this->PulseOrigin[0] =
  this->PulseOrigin[1] =
  this->PulseOrigin[2] = 0.0;

  this->PulseWidth[0]  =
  this->PulseWidth[1]  =
  this->PulseWidth[2]  = 0.5;

  this->NumberOfLevels = 3;
  this->Dimension      = 3;
  this->RefinmentRatio = 2;
  this->PulseAmplitude = 0.0001;
}

//------------------------------------------------------------------------------
vtkAMRGaussianPulseSource::~vtkAMRGaussianPulseSource()
{

}

//------------------------------------------------------------------------------
void vtkAMRGaussianPulseSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
int vtkAMRGaussianPulseSource::RequestData(
    vtkInformation *request,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector )
{
  return 1;
}

