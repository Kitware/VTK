/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridWriter.cxx
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
#include "vtkRectilinearGridWriter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkRectilinearGridWriter, "1.18");
vtkStandardNewMacro(vtkRectilinearGridWriter);

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkRectilinearGridWriter::SetInput(vtkRectilinearGrid *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkRectilinearGrid *vtkRectilinearGridWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkRectilinearGrid *)(this->Inputs[0]);
}


void vtkRectilinearGridWriter::WriteData()
{
  ostream *fp;
  vtkRectilinearGrid *input = this->GetInput();
  int dim[3];

  vtkDebugMacro(<<"Writing vtk rectilinear grid...");

  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
    {
    return;
    }
  //
  // Write rectilinear grid specific stuff
  //
  *fp << "DATASET RECTILINEAR_GRID\n"; 

  // Write data owned by the dataset
  this->WriteDataSetData(fp, input);

  input->GetDimensions(dim);
  *fp << "DIMENSIONS " << dim[0] << " " << dim[1] << " " << dim[2] << "\n";

  this->WriteCoordinates(fp, input->GetXCoordinates(), 0);
  this->WriteCoordinates(fp, input->GetYCoordinates(), 1);
  this->WriteCoordinates(fp, input->GetZCoordinates(), 2);

  this->WriteCellData(fp, input);
  this->WritePointData(fp, input);

  this->CloseVTKFile(fp);
}

void vtkRectilinearGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
