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

#include <stdlib.h>

vtkCxxRevisionMacro(vtkImageNoiseSource, "1.23");
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
  float *outPtr;
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  int *outExt;
  unsigned long count = 0;
  unsigned long target;
  
  if (data->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro("Execute: This source only outputs floats");
    }
  
  outExt = data->GetExtent();
  
  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1);
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  
  // Get increments to march through data 
  data->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  outPtr = (float *) data->GetScalarPointer(outExt[0],outExt[2],outExt[4]);
  
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; !this->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!(count%target))
        {
        this->UpdateProgress(count/(50.0*target));
        }
      count++;
      for (idxR = 0; idxR < rowLength; idxR++)
        {
        // Pixel operation
        *outPtr = this->Minimum +
          (this->Maximum - this->Minimum) * vtkMath::Random();
        outPtr++;
        }
      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }
}

void vtkImageNoiseSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Minimum: " << this->Minimum << "\n";
  os << indent << "Maximum: " << this->Maximum << "\n";
}

