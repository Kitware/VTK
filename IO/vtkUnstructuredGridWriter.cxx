/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridWriter.cxx
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
#include "vtkUnstructuredGridWriter.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"

#include <stdio.h>

vtkCxxRevisionMacro(vtkUnstructuredGridWriter, "1.37");
vtkStandardNewMacro(vtkUnstructuredGridWriter);

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkUnstructuredGridWriter::SetInput(vtkUnstructuredGrid *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkUnstructuredGrid *vtkUnstructuredGridWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkUnstructuredGrid *)(this->Inputs[0]);
}

void vtkUnstructuredGridWriter::WriteData()
{
  ostream *fp;
  vtkUnstructuredGrid *input=this->GetInput();
  int *types, ncells, cellId;

  vtkDebugMacro(<<"Writing vtk unstructured grid data...");

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
  //
  // Write unstructured grid specific stuff
  //
  *fp << "DATASET UNSTRUCTURED_GRID\n"; 

  // Write data owned by the dataset
  if (!this->WriteDataSetData(fp, input))
    {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
    }

  if (!this->WritePoints(fp, input->GetPoints()))
    {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
    }
  if (!this->WriteCells(fp, input->GetCells(),"CELLS"))
    {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
    }
  
  //
  // Cell types are a little more work
  //
  ncells = input->GetCells()->GetNumberOfCells();
  types = new int[ncells];
  for (cellId=0; cellId < ncells; cellId++)
    {
    types[cellId] = input->GetCellType(cellId);
    }

  *fp << "CELL_TYPES " << ncells << "\n";
  if ( this->FileType == VTK_ASCII )
    {
    for (cellId=0; cellId<ncells; cellId++)
      {
      *fp << types[cellId] << "\n";
      }
    }
  else
    {
    // swap the bytes if necc
    vtkByteSwap::SwapWrite4BERange(types,ncells,fp);
    }
  *fp << "\n";
  delete [] types;

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

void vtkUnstructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
