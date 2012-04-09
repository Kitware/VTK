/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridWriter.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkStandardNewMacro(vtkUnstructuredGridWriter);

void vtkUnstructuredGridWriter::WriteData()
{
  ostream *fp;
  vtkUnstructuredGrid *input= vtkUnstructuredGrid::SafeDownCast(
    this->GetInput());
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
  if ( input->GetCells() )
    {
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

int vtkUnstructuredGridWriter::FillInputPortInformation(int,
                                                        vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

vtkUnstructuredGrid* vtkUnstructuredGridWriter::GetInput()
{
  return vtkUnstructuredGrid::SafeDownCast(this->Superclass::GetInput());
}

vtkUnstructuredGrid* vtkUnstructuredGridWriter::GetInput(int port)
{
  return vtkUnstructuredGrid::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkUnstructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
