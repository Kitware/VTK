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

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#endif

vtkCxxRevisionMacro(vtkStructuredGridWriter, "1.35");
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
    if (fp)
      {
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
      }
    return;
    }

  // Write structured grid specific stuff
  //
  *fp << "DATASET STRUCTURED_GRID\n";

  // Write data owned by the dataset
  if (!this->WriteDataSetData(fp, input))
    {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
    }

  input->GetDimensions(dim);
  *fp << "DIMENSIONS " << dim[0] << " " << dim[1] << " " << dim[2] << "\n";

  if (!this->WritePoints(fp, input->GetPoints()))
    {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
    }
  
  // If blanking, write that information out
  if ( input->GetBlanking() )
    {
    if (!this->WriteBlanking(fp, input))
      {
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
      return;
      }
    }

  if (!this->WriteCellData(fp, input))
    {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
    }
  if (!this->WritePointData(fp, input))
    {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
    }

  this->CloseVTKFile(fp);
}

int vtkStructuredGridWriter::WriteBlanking(ostream *fp, vtkStructuredGrid *grid)
{
  vtkUnsignedCharArray *blanking=grid->GetPointVisibility();
  
  int numPts = grid->GetNumberOfPoints();
  *fp << "BLANKING " << numPts;
  return this->WriteArray(fp, VTK_UNSIGNED_CHAR, blanking, " %s\n", numPts, 1);
}


void vtkStructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
