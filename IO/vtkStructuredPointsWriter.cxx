/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsWriter.cxx
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
#include "vtkStructuredPointsWriter.h"

#include "vtkObjectFactory.h"
#include "vtkStructuredPoints.h"

vtkCxxRevisionMacro(vtkStructuredPointsWriter, "1.34");
vtkStandardNewMacro(vtkStructuredPointsWriter);

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkStructuredPointsWriter::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkImageData *vtkStructuredPointsWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}


void vtkStructuredPointsWriter::WriteData()
{
  ostream *fp;
  vtkImageData *input=this->GetInput();
  int dim[3];
  int *ext;
  float spacing[3], origin[3];

  vtkDebugMacro(<<"Writing vtk structured points...");

  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
      {
      return;
      }
  //
  // Write structured points specific stuff
  //
  *fp << "DATASET STRUCTURED_POINTS\n";

  // Write data owned by the dataset
  this->WriteDataSetData(fp, input);

  input->GetDimensions(dim);
  *fp << "DIMENSIONS " << dim[0] << " " << dim[1] << " " << dim[2] << "\n";

  input->GetSpacing(spacing);
  *fp << "SPACING " << spacing[0] << " " << spacing[1] << " " << spacing[2] << "\n";

  input->GetOrigin(origin);
  // Do the electric slide. Move origin to min corner of extent.
  // The alternative is to change the format to include an extent instead of dimensions.
  ext = input->GetExtent();
  origin[0] += ext[0] * spacing[0];
  origin[1] += ext[2] * spacing[1];
  origin[2] += ext[4] * spacing[2];
  *fp << "ORIGIN " << origin[0] << " " << origin[1] << " " << origin[2] << "\n";

  this->WriteCellData(fp, input);
  this->WritePointData(fp, input);

  this->CloseVTKFile(fp);
}

void vtkStructuredPointsWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
