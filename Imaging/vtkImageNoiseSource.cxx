/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNoiseSource.cxx
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
#include "vtkImageNoiseSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkImageProgressIterator.h"

vtkCxxRevisionMacro(vtkImageNoiseSource, "1.24");
vtkStandardNewMacro(vtkImageNoiseSource);

//----------------------------------------------------------------------------
vtkImageNoiseSource::vtkImageNoiseSource()
{
  this->Minimum = 0.0;
  this->Maximum = 10.0;
  this->WholeExtent[0] = 0;  this->WholeExtent[1] = 255;
  this->WholeExtent[2] = 0;  this->WholeExtent[3] = 255;
  this->WholeExtent[4] = 0;  this->WholeExtent[5] = 0;
}


//----------------------------------------------------------------------------
void vtkImageNoiseSource::SetWholeExtent(int xMin, int xMax, 
                                         int yMin, int yMax,
                                         int zMin, int zMax)
{
  int modified = 0;
  
  if (this->WholeExtent[0] != xMin)
    {
    modified = 1;
    this->WholeExtent[0] = xMin ;
    }
  if (this->WholeExtent[1] != xMax)
    {
    modified = 1;
    this->WholeExtent[1] = xMax ;
    }
  if (this->WholeExtent[2] != yMin)
    {
    modified = 1;
    this->WholeExtent[2] = yMin ;
    }
  if (this->WholeExtent[3] != yMax)
    {
    modified = 1;
    this->WholeExtent[3] = yMax ;
    }
  if (this->WholeExtent[4] != zMin)
    {
    modified = 1;
    this->WholeExtent[4] = zMin ;
    }
  if (this->WholeExtent[5] != zMax)
    {
    modified = 1;
    this->WholeExtent[5] = zMax ;
    }
  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImageNoiseSource::ExecuteInformation()
{
  vtkImageData *output = this->GetOutput();
  
  output->SetWholeExtent(this->WholeExtent);
  output->SetScalarType(VTK_FLOAT);
  output->SetNumberOfScalarComponents(1);
}

void vtkImageNoiseSource::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data = this->AllocateOutputData(output);
  
  if (data->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro("Execute: This source only outputs floats");
    }
  
  vtkImageProgressIterator<float> outIt(data, data->GetExtent(), this, 0);

  // Loop through ouput pixels
  while (!outIt.IsAtEnd())
    {
    float* outSI = outIt.BeginSpan();
    float* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
      {
      // now process the components
      *outSI = this->Minimum +
        (this->Maximum - this->Minimum) * vtkMath::Random();
      outSI++;
      }
    outIt.NextSpan();
    }
}

void vtkImageNoiseSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Minimum: " << this->Minimum << "\n";
  os << indent << "Maximum: " << this->Maximum << "\n";
}

