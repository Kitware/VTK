/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridWriter.cxx
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
#include "vtkStructuredGridWriter.h"

#include "vtkObjectFactory.h"
#include "vtkStructuredGrid.h"

vtkCxxRevisionMacro(vtkStructuredGridWriter, "1.32");
vtkStandardNewMacro(vtkStructuredGridWriter);

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkStructuredGridWriter::SetInput(vtkStructuredGrid *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkStructuredGrid *vtkStructuredGridWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkStructuredGrid *)(this->Inputs[0]);
}

void vtkStructuredGridWriter::WriteData()
{
  ostream *fp;
  vtkStructuredGrid *input= this->GetInput();
  int dim[3];

  vtkDebugMacro(<<"Writing vtk structured grid...");

  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
    {
      return;
    }

  // Write structured grid specific stuff
  //
  *fp << "DATASET STRUCTURED_GRID\n";

  // Write data owned by the dataset
  this->WriteDataSetData(fp, input);

  input->GetDimensions(dim);
  *fp << "DIMENSIONS " << dim[0] << " " << dim[1] << " " << dim[2] << "\n";

  this->WritePoints(fp, input->GetPoints());
  
  // If blanking, write that information out
  if ( input->GetBlanking() )
    {
    this->WriteBlanking(fp, input);
    }

  this->WriteCellData(fp, input);
  this->WritePointData(fp, input);

  this->CloseVTKFile(fp);
}

void vtkStructuredGridWriter::WriteBlanking(ostream *fp, vtkStructuredGrid *grid)
{
  vtkUnsignedCharArray *blanking=grid->GetPointVisibility();
  
  int numPts = grid->GetNumberOfPoints();
  *fp << "BLANKING " << numPts;
  WriteArray(fp, VTK_UNSIGNED_CHAR, blanking, " %s\n", numPts, 1);
}


void vtkStructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
